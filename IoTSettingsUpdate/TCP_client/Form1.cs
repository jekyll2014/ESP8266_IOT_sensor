using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

using TextLoggerHelper;

namespace IoTSettingsUpdate
{
    public partial class Form1 : Form
    {
        private readonly Config<IoTSettingsUpdateSettings> _appConfig = new Config<IoTSettingsUpdateSettings>("appsettings.json");

        private TcpClient _clientSocket;
        private NetworkStream _serverStream;
        private string _ipAddress;
        private int _ipPort;
        private string _comName;
        private int _comSpeed;
        private int _replyTimeout = 10000;
        private readonly List<char> _stringsDivider = new List<char> { '\r', '\n' };

        private StringBuilder _unparsedData = new StringBuilder();

        //private int _maxLogLength = 4096;
        private bool _autoScroll;
        private int _selStart = 0;

        private readonly DataTable _configData = new DataTable();

        private readonly string[] _columnNames =
            {
            "Command",
            "Default value",
            "Custom value",
            "Parameter description",
            "ParameterName",
            "ErrorReplyString"
        };

        private enum Columns
        {
            Command,
            DefaultValue,
            CustomValue,
            ParameterDescription,
            ParameterName,
            ErrorReplyString
        }

        private enum DataDirection
        {
            Received,
            Sent,
            Error,
            Note
        }

        private readonly Dictionary<byte, string> _directions = new Dictionary<byte, string>()
        {
            {(byte)DataDirection.Received, "<<"},
            {(byte)DataDirection.Sent,">>"},
            {(byte)DataDirection.Error,"!!"},
            {(byte)DataDirection.Note,"**"}
        };

        private TextLogger _logger;

        private Config<CommandSettings> _espSettings;

        private readonly List<AwaitedReply> _awaitedReplies = new List<AwaitedReply>();
        private volatile bool _sending;

        private class AwaitedReply
        {
            public string ParameterPattern = "";
            public string IncorrectReplyPattern = "";
            public bool found;
            public bool incorrect;
        }

        #region Data acquisition

        private async void Button_sendAll_Click(object sender, EventArgs e)
        {
            if (_sending)
            {
                _sending = false;

                return;
            }

            var currentCell = dataGridView_config.CurrentCell.ColumnIndex;

            if (currentCell != (int)Columns.CustomValue
                && currentCell != (int)Columns.DefaultValue)
                return;

            _sending = true;
            //dataGridView_config.Enabled = false;
            button_send.Enabled = false;
            button_sendAll.Text = "Stop";
            button_sendCommand.Enabled = false;
            button_getConfig.Enabled = false;
            button_getStatus.Enabled = false;
            button_getSensor.Enabled = false;
            button_setTime.Enabled = false;
            button_reset.Enabled = false;
            button_help.Enabled = false;
            comboBox_portname1.Enabled = true;

            for (var currentRow = 0; currentRow < _configData.Rows.Count; currentRow++)
            {
                await SendParameter(currentRow, currentCell);
                Thread.Sleep(200);
                Application.DoEvents();

                if (!_sending)
                    break;
            }

            _sending = false;
            //dataGridView_config.Enabled = true;
            button_send.Enabled = true;
            button_sendAll.Text = "Send all";
            button_sendCommand.Enabled = true;
            button_getConfig.Enabled = true;
            button_getStatus.Enabled = true;
            button_getSensor.Enabled = true;
            button_setTime.Enabled = true;
            button_reset.Enabled = true;
            button_help.Enabled = true;
            comboBox_portname1.Enabled = true;
        }

        private async Task<bool> SendParameter(int currentRow, int currentCell)
        {
            var par = _configData.Rows[currentRow].ItemArray[currentCell].ToString();

            if (string.IsNullOrEmpty(par))
                par = " ";
            var tmpStr = _configData.Rows[currentRow].ItemArray[(int)Columns.Command] + "=" + par;

            var outStream = new List<byte>();
            outStream.AddRange(Encoding.ASCII.GetBytes(tmpStr));

            var paramName = _configData.Rows[currentRow].ItemArray[(int)Columns.ParameterName].ToString();
            var incorrectSample = _configData.Rows[currentRow].ItemArray[(int)Columns.ErrorReplyString].ToString();

            SendData(outStream.ToArray(), checkBox_addCrLf.Checked);

            if (string.IsNullOrEmpty(paramName))
                return true;

            dataGridView_config.Rows[currentRow].Cells[currentCell].Style.BackColor = Color.Yellow;

            var res = await WaitForReply(paramName, incorrectSample, _replyTimeout);
            if (res)
            {
                dataGridView_config.Rows[currentRow].Cells[currentCell].Style.BackColor = Color.GreenYellow;
            }
            else
            {
                dataGridView_config.Rows[currentRow].Cells[currentCell].Style.BackColor = Color.Red;
            }

            //dataGridView_config.CurrentCell = dataGridView_config.Rows[currentRow].Cells[currentCell];

            return res;
        }

        private bool SendData(IEnumerable<byte> outStream, bool addEol)
        {
            var outData = new List<byte>();
            outData.AddRange(outStream);
            if (addEol)
                outData.AddRange(new byte[] { 13, 10 });

            if (comboBox_portname1.SelectedIndex == 0)
            {
                if (_clientSocket.Client.Connected)
                {
                    try
                    {
                        _serverStream.Write(outData.ToArray(), 0, outData.Count);
                    }
                    catch (Exception ex)
                    {
                        _logger.AddText("Write error: " + ex + Environment.NewLine,
                            (byte)DataDirection.Error, DateTime.Now);
                        return false;
                    }
                }
                else
                {
                    TryReconnect();
                    return false;
                }
            }
            else
            {
                if (serialPort1.IsOpen)
                {
                    try
                    {
                        serialPort1.Write(outData.ToArray(), 0, outData.Count);
                    }
                    catch (Exception ex)
                    {
                        _logger.AddText("Write error: " + ex + Environment.NewLine,
                            (byte)DataDirection.Error, DateTime.Now);
                        return false;
                    }
                }
                else
                {
                    _logger.AddText("Port closed\r\n", (byte)DataDirection.Note, DateTime.Now);
                    Button_disconnect_Click(this, EventArgs.Empty);
                    return false;
                }
            }

            _logger.AddText(Encoding.ASCII.GetString(outData.ToArray()), (byte)DataDirection.Sent, DateTime.Now);

            return true;
        }

        private void ReadTelnet()
        {
            if (!IsClientConnected())
            {
                TryReconnect();
            }

            if (IsClientConnected())
            {
                var data = new List<byte>();
                while (_serverStream.DataAvailable)
                    try
                    {
                        var inStream = new byte[_clientSocket.Available];
                        _serverStream.Read(inStream, 0, inStream.Length);
                        data.AddRange(inStream);
                    }
                    catch (Exception ex)
                    {
                        _logger.AddText("Read error: " + ex + Environment.NewLine,
                            (byte)DataDirection.Error, DateTime.Now);
                    }

                ProcessInput(data.ToArray());
            }
        }

        private void SerialPort1_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            if (serialPort1.IsOpen)
            {
                var data = new List<byte>();
                while (serialPort1.BytesToRead > 0)
                    try
                    {
                        var d = new byte[serialPort1.BytesToRead];
                        serialPort1.Read(d, 0, d.Length);
                        data.AddRange(d);
                    }
                    catch (Exception ex)
                    {
                        _logger.AddText("Read error: " + ex + Environment.NewLine,
                            (byte)DataDirection.Error, DateTime.Now);
                    }

                ProcessInput(data.ToArray());
            }
            else
            {
                TryReconnect();
            }
        }

        private bool IsClientConnected()
        {
            var ipProperties = IPGlobalProperties.GetIPGlobalProperties();
            var tcpConnections = ipProperties.GetActiveTcpConnections();
            try
            {
                if (_clientSocket != null)
                {
                    foreach (var c in tcpConnections.Where(c => c.LocalEndPoint.Equals(_clientSocket.Client.LocalEndPoint) &&
                                    c.RemoteEndPoint.Equals(_clientSocket.Client.RemoteEndPoint)))
                    {
                        if (c.State == TcpState.Established)
                            return true;
                    }
                }
                else
                    return false;
            }
            catch (Exception ex)
            {
                _logger.AddText("Socket error: " + ex + Environment.NewLine,
                    (byte)DataDirection.Error, DateTime.Now);
                return false;
            }

            return false;
        }

        private void ProcessInput(byte[] data)
        {
            if (data == null || data.Length == 0) return;

            var incomingStr = Encoding.Default.GetString(data.ToArray());
            _logger.AddText(incomingStr, (byte)DataDirection.Received, DateTime.Now);

            var stringsCollection = ConvertBytesToStrings(data, ref _unparsedData);
            foreach (var s in stringsCollection)
            {
                foreach (var item in _awaitedReplies.Where(item => s.StartsWith(item.ParameterPattern)))// try to find awaited reply
                {
                    var parts = s.Split('=');
                    if (parts.Length > 1 && parts[1].StartsWith(item.IncorrectReplyPattern))
                    {
                        item.incorrect = true;
                    }
                    item.found = true;
                }
            }
        }

        private async Task<bool> WaitForReply(string paramName, string incorrectName, int timeout)
        {
            var result = false;

            var reply = new AwaitedReply()
            {
                ParameterPattern = paramName,
                IncorrectReplyPattern = incorrectName
            };
            _awaitedReplies.Add(reply);

            var start = DateTime.Now;
            await Task.Run(() =>
            {
                while (DateTime.Now.Subtract(start).TotalMilliseconds < timeout)
                {
                    Thread.Sleep(200);
                    Application.DoEvents();

                    if (reply.found)
                    {
                        result = !reply.incorrect;
                        break;
                    }
                }

                _awaitedReplies.Remove(reply);
            }).ConfigureAwait(true);

            return result;
        }

        #endregion

        #region Helpers

        private void SerialPopulate()
        {
            var portSelected = comboBox_portname1.SelectedItem?.ToString();
            comboBox_portname1.Items.Clear();
            //Serial settings populate
            comboBox_portname1.Items.Add(_ipAddress + ":" + _ipPort);
            //Add ports
            foreach (var s in SerialPort.GetPortNames()) comboBox_portname1.Items.Add(s);


            if (portSelected == null)
            {
                if (_comName.StartsWith("COM") && comboBox_portname1.Items.Contains(_comName))
                    comboBox_portname1.SelectedItem = _comName;
                else
                    comboBox_portname1.SelectedIndex = 0;
            }
            else
            {
                if (comboBox_portname1.Items.Contains(portSelected))
                    comboBox_portname1.SelectedItem = portSelected;
                else
                    comboBox_portname1.SelectedIndex = 0;
            }

            if (comboBox_portspeed1.Items.Contains(_comSpeed.ToString()))
                comboBox_portspeed1.SelectedItem = _comSpeed.ToString();
            else
                comboBox_portspeed1.SelectedIndex = 0;
        }

        private void Timer1_Tick(object sender, EventArgs e)
        {
            ReadTelnet();
        }

        private void TryReconnect()
        {
            _logger.AddText("not connected\r\n", (byte)DataDirection.Note, DateTime.Now);
            Button_disconnect_Click(this, EventArgs.Empty);
            Button_connect_Click(this, EventArgs.Empty);
        }

        private string[] ConvertBytesToStrings(IEnumerable<byte> data, ref StringBuilder unparsedData)
        {
            var stringCollection = new List<string>();
            foreach (var t in data)
            {
                var found = false;
                foreach (var c in _stringsDivider)
                    if ((char)t == c)
                    {
                        found = true;
                        break;
                    }

                if (found)
                {
                    if (unparsedData.Length > 0)
                    {
                        stringCollection.Add(unparsedData.ToString());
                        unparsedData.Clear();
                    }
                }
                else
                {
                    unparsedData.Append((char)t);
                }
            }

            return stringCollection.ToArray();
        }

        private void ParseIpAddress(string ipText)
        {
            var portPosition = ipText.IndexOf(':');
            if (portPosition == 0)
            {
                _ipAddress = _appConfig.ConfigStorage.DefaultAddress;
                int.TryParse(ipText.Substring(portPosition + 1), out _ipPort);
            }
            else if (portPosition > 0)
            {
                _ipAddress = ipText.Substring(0, portPosition);
                int.TryParse(ipText.Substring(portPosition + 1), out _ipPort);
            }
            else
            {
                _ipAddress = ipText;
                _ipPort = _appConfig.ConfigStorage.DefaultPort;
            }
        }

        private void PrepareConfig()
        {
            var newConfig = _configData.Rows.Cast<DataRow>()
                .Select(row => new CommandSetting
                {
                    Command = row[(int)Columns.Command].ToString(),
                    DefaultValue = row[(int)Columns.DefaultValue].ToString(),
                    CustomValue = row[(int)Columns.CustomValue].ToString(),
                    ParameterDescription = row[(int)Columns.ParameterDescription].ToString(),
                    ErrorReplyString = row[(int)Columns.ErrorReplyString].ToString(),
                    ParameterName = row[(int)Columns.ParameterName].ToString()
                })
                .ToList();

            _espSettings.ConfigStorage.Commands = newConfig;
        }

        private async Task<Dictionary<IPAddress, string>> PingAddress(byte[] addressBytes, int num, int netPort, int pingTimeout = 3000, int dataTimeOut = 20000)
        {
            var ping = new Ping();
            addressBytes[3] = (byte)num;
            var destIp = new IPAddress(addressBytes);
            var pingResultTask = await ping.SendPingAsync(destIp, pingTimeout).ConfigureAwait(true);
            ping.Dispose();

            if (pingResultTask?.Status != IPStatus.Success) return null;


            var deviceFound = new Dictionary<IPAddress, string>();
            using (var tmpSocket = new TcpClient())
            {
                try
                {
                    tmpSocket.Connect(destIp, netPort);
                    tmpSocket.ReceiveTimeout = pingTimeout;
                    tmpSocket.SendTimeout = pingTimeout;
                    tmpSocket.Client.ReceiveTimeout = pingTimeout;
                    tmpSocket.Client.SendTimeout = pingTimeout;
                    var data = new List<byte>();
                    using (var tmpStream = tmpSocket.GetStream())
                    {
                        deviceFound.Add(destIp, "");

                        if (tmpSocket.Client.Connected)
                        {
                            var outData = Encoding.ASCII.GetBytes("get_status\r\n");
                            try
                            {
                                tmpStream.Write(outData, 0, outData.Length);
                            }
                            catch (Exception ex)
                            {
                                Console.WriteLine(ex);
                                return null;
                            }
                        }
                        else
                        {
                            return null;
                        }

                        var startTime = DateTime.Now;

                        do
                        {
                            if (tmpStream.DataAvailable)
                            {
                                try
                                {
                                    var inStream = new byte[tmpSocket.Available];
                                    tmpStream.Read(inStream, 0, inStream.Length);
                                    data.AddRange(inStream);
                                    break;
                                }
                                catch (Exception ex)
                                {
                                    Console.WriteLine(ex);
                                }
                            }
                        } while (DateTime.Now.Subtract(startTime).TotalMilliseconds < dataTimeOut);
                    }

                    var tmp = new StringBuilder();
                    var stringsCollection = ConvertBytesToStrings(data.ToArray(), ref tmp);
                    if (stringsCollection.Length > 0) deviceFound[destIp] = stringsCollection[0];
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex);
                    if (tmpSocket.Client.Connected) tmpSocket.Client.Disconnect(false);

                    //serverStream.Close();
                    if (tmpSocket.Connected) tmpSocket.Close();
                    return null;
                }
            }

            return deviceFound;
        }

        private void CopySample()
        {
            if (dataGridView_config == null) return;

            if (dataGridView_config.SelectedCells.Count == 1)
            {
                Clipboard.SetText(dataGridView_config.SelectedCells[0].Value.ToString());
            }
            else
            {
                var copyText = new StringBuilder();
                foreach (DataGridViewCell cell in dataGridView_config.SelectedCells)
                {
                    copyText.Append(cell.ToString());
                }

                Clipboard.SetText(copyText.ToString());
            }
        }

        private List<CommandSetting> ParseCommands(string text)
        {
            var result = new List<CommandSetting>();

            var lineBuffer = text
                .Split(new[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries)
                .Where(n => n.Trim().StartsWith("#define"))
                .Select(n => n.Replace("#define ", "").Trim())
                .ToList();

            var replyIncorrectProperty = FindSubString(lineBuffer.FirstOrDefault(n => n.StartsWith("REPLY_INCORRECT")) ?? "");
            lineBuffer.Remove(replyIncorrectProperty);

            for (var i = 0; i < lineBuffer.Count; i++)
            {
                if (lineBuffer[i].StartsWith("CMD_"))
                {
                    var command = FindSubString(lineBuffer[i]);
                    var comment = FindComment(lineBuffer[i]);
                    var newCommand = new CommandSetting()
                    {
                        ErrorReplyString = replyIncorrectProperty,
                        Command = command,
                        ParameterDescription = comment
                    };
                    result.Add(newCommand);

                    if ((i + 1) < lineBuffer.Count && lineBuffer[i + 1].StartsWith("REPLY_"))
                    {
                        i++;
                        newCommand.ParameterName = FindSubString(lineBuffer[i]);
                    }
                }
            }

            return result;
        }

        private string FindSubString(string line)
        {
            var result = "";
            var dividerPosition = line.IndexOf('\"');
            if (dividerPosition >= 0 && dividerPosition < line.Length)
            {
                result = line.Substring(dividerPosition + 1);
                dividerPosition = result.IndexOf('\"');
                if (dividerPosition >= 0 && dividerPosition < result.Length)
                {
                    result = result.Substring(0, dividerPosition);
                }
            }

            return result;
        }

        private string FindComment(string line)
        {
            var result = "";
            var commentPosition = line.IndexOf("//");
            if (commentPosition >= 0 && commentPosition + 2 < line.Length)
            {
                result = line.Substring(commentPosition + 2);
            }

            return result;
        }

        #endregion

        #region GUI

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint | ControlStyles.DoubleBuffer, true);

            serialPort1.Encoding = Encoding.GetEncoding(_appConfig.ConfigStorage.CodePage);
            _ipAddress = _appConfig.ConfigStorage.DefaultAddress;
            _ipPort = _appConfig.ConfigStorage.DefaultPort;
            _comName = _appConfig.ConfigStorage.DefaultComName;
            _comSpeed = _appConfig.ConfigStorage.DefaultComSpeed;
            _replyTimeout = _appConfig.ConfigStorage.ReplyTimeout;
            textBox_replyTimeout.Text = _replyTimeout.ToString();
            _stringsDivider.AddRange(_appConfig.ConfigStorage.StringsDivider);

            SerialPopulate();
            _logger = new TextLogger(this)
            {
                FilterZeroChar = true,
                AutoSave = true,
                AutoScroll = checkBox_autoScroll.Checked,
                DefaultTextFormat = TextLogger.TextFormat.PlainText,
                DefaultTimeFormat = TextLogger.TimeFormat.LongTime,
                LogFileName = "loader.log",
                LineTimeLimit = 500,
                Channels = _directions
            };

            textBox_dataLog.DataBindings.Add("Text", _logger, "Text", false, DataSourceUpdateMode.OnPropertyChanged);

            foreach (var col in _columnNames)
                _configData.Columns.Add(col);

            _configData.Columns[(int)Columns.Command].ReadOnly = true;
            _configData.Columns[(int)Columns.DefaultValue].ReadOnly = true;
            _configData.Columns[(int)Columns.ParameterDescription].ReadOnly = true;
            _configData.Columns[(int)Columns.ErrorReplyString].ReadOnly = true;
            _configData.Columns[(int)Columns.ParameterName].ReadOnly = true;

            dataGridView_config.AllowUserToAddRows = false;
            dataGridView_config.RowHeadersVisible = false;
            dataGridView_config.DataSource = _configData;
            foreach (DataGridViewColumn col in dataGridView_config.Columns)
            {
                col.SortMode = DataGridViewColumnSortMode.NotSortable;
                col.AutoSizeMode = DataGridViewAutoSizeColumnMode.AllCells;
            }
            dataGridView_config.Columns[Columns.ErrorReplyString.ToString()].Visible = false;
            dataGridView_config.Columns[Columns.ParameterName.ToString()].Visible = false;

            _autoScroll = checkBox_autoScroll.Checked;
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            timer1.Enabled = false;
            if (_clientSocket?.Client?.Connected ?? false)
            {
                _clientSocket.Client.Disconnect(false);
                _clientSocket.Close();
                //_clientSocket.Dispose();
            }

            _appConfig.ConfigStorage.DefaultAddress = _ipAddress;
            _appConfig.ConfigStorage.DefaultPort = _ipPort;
            if (comboBox_portname1.SelectedIndex > 0)
                _appConfig.ConfigStorage.DefaultComName = comboBox_portname1.Items[comboBox_portname1.SelectedIndex].ToString();
            else
                _appConfig.ConfigStorage.DefaultComName = "";
            _appConfig.ConfigStorage.DefaultComSpeed = int.Parse(comboBox_portspeed1.SelectedItem.ToString());
            _appConfig.SaveConfig();
        }

        private void CheckBox_hex_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_hex.Checked)
                _logger.DefaultTextFormat = TextLogger.TextFormat.Hex;
            else
                _logger.DefaultTextFormat = TextLogger.TextFormat.AutoReplaceHex;
        }

        private void ComboBox_portname1_DropDown(object sender, EventArgs e)
        {
            SerialPopulate();
        }

        private void ComboBox_portname1_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (comboBox_portname1.SelectedIndex == 0)
            {
                comboBox_portname1.DropDownStyle = ComboBoxStyle.DropDown;
                comboBox_portspeed1.Enabled = false;
                button_scanIp.Enabled = true;
            }
            else
            {
                comboBox_portname1.DropDownStyle = ComboBoxStyle.DropDownList;
                comboBox_portspeed1.Enabled = true;
                button_scanIp.Enabled = false;
            }
        }

        private void ComboBox_portname1_Leave(object sender, EventArgs e)
        {
            if (comboBox_portname1.SelectedIndex < 0 &&
                comboBox_portname1.Items[0].ToString() != comboBox_portname1.Text)
            {
                ParseIpAddress(comboBox_portname1.Text);

                comboBox_portname1.Items[0] = _ipAddress + ":" + _ipPort;
                comboBox_portname1.SelectedIndex = 0;
            }
        }

        private void Button_clear_Click(object sender, EventArgs e)
        {
            _logger.Clear();
            textBox_dataLog.Clear();
        }

        private void Button_load_Click(object sender, EventArgs e)
        {
            openFileDialog1.FileName = "";
            openFileDialog1.Title = "Open text config";
            openFileDialog1.DefaultExt = "json";
            openFileDialog1.Filter = "JSON files|*.json|Header files|*.h";
            openFileDialog1.ShowDialog();
        }

        private void OpenFileDialog1_FileOk(object sender, CancelEventArgs e)
        {
            if (openFileDialog1.FileName.EndsWith(".json", StringComparison.OrdinalIgnoreCase))
            {
                _espSettings = new Config<CommandSettings>(openFileDialog1.FileName);
                FillCommandTable(_espSettings?.ConfigStorage.Commands);
            }
            else if (openFileDialog1.FileName.EndsWith(".h", StringComparison.OrdinalIgnoreCase))
            {
                string text = "";
                try
                {
                    text = File.ReadAllText(openFileDialog1.FileName);
                }
                catch
                {
                    MessageBox.Show($"Can't read file: {openFileDialog1.FileName}");
                }
                _espSettings = new Config<CommandSettings>();
                _espSettings.ConfigFileName = openFileDialog1.FileName.Substring(0, openFileDialog1.FileName.Length - 2) + ".json";
                _espSettings.ConfigStorage.Commands = ParseCommands(text);
                FillCommandTable(_espSettings?.ConfigStorage?.Commands);
            }
            else
                MessageBox.Show($"Incompatible file type: {openFileDialog1.FileName}");
        }

        private void FillCommandTable(List<CommandSetting> commandList)
        {
            if (commandList?.Count > 0)
            {
                _configData.Rows.Clear();
                foreach (var command in commandList)
                {
                    var newRow = _configData.NewRow();
                    newRow[(int)Columns.Command] = command.Command;
                    newRow[(int)Columns.DefaultValue] = command.DefaultValue;
                    newRow[(int)Columns.ParameterDescription] = command.ParameterDescription;
                    newRow[(int)Columns.CustomValue] = command.CustomValue;
                    newRow[(int)Columns.ErrorReplyString] = command.ErrorReplyString;
                    newRow[(int)Columns.ParameterName] = command.ParameterName;
                    _configData.Rows.Add(newRow);
                }
            }
        }

        private void Button_save_Click(object sender, EventArgs e)
        {
            saveFileDialog1.Title = "Save config...";
            saveFileDialog1.DefaultExt = "json";
            saveFileDialog1.Filter = "JSON files|*.json|All files|*.*";
            saveFileDialog1.FileName = string.IsNullOrEmpty(_espSettings?.ConfigFileName) ? "config.json" : _espSettings.ConfigFileName;

            saveFileDialog1.ShowDialog();
        }

        private void SaveFileDialog1_FileOk(object sender, CancelEventArgs e)
        {
            PrepareConfig();
            _espSettings.SaveConfig(saveFileDialog1.FileName);
        }

        private void CheckBox_autoScroll_CheckedChanged(object sender, EventArgs e)
        {
            _autoScroll = _logger.AutoScroll = checkBox_autoScroll.Checked;
        }

        private void TextBox_replyTimeout_Leave(object sender, EventArgs e)
        {
            int.TryParse(textBox_replyTimeout.Text, out _replyTimeout);
            textBox_replyTimeout.Text = _replyTimeout.ToString();
        }

        private void Button_sendCommand_Click(object sender, EventArgs e)
        {
            if (string.IsNullOrEmpty(textBox_customCommand.Text))
                return;

            var tmpStr = textBox_customCommand.Text;
            var outStream = new List<byte>();
            outStream.AddRange(Encoding.ASCII.GetBytes(tmpStr));

            SendData(outStream.ToArray(), checkBox_addCrLf.Checked);
        }

        private void Button_connect_Click(object sender, EventArgs e)
        {
            if (comboBox_portname1.SelectedIndex == 0)
            {
                try
                {
                    _clientSocket = new TcpClient();
                    _clientSocket.Connect(_ipAddress, _ipPort);
                    _clientSocket.ReceiveTimeout = 1000;
                    _clientSocket.SendTimeout = 1000;
                    _clientSocket.Client.ReceiveTimeout = 1000;
                    _clientSocket.Client.SendTimeout = 1000;
                    _serverStream = _clientSocket.GetStream();
                }
                catch (Exception ex)
                {
                    _logger.AddText("Connect error to: " + _ipAddress + ":" + _ipPort + ex + Environment.NewLine,
                        (byte)DataDirection.Error, DateTime.Now);
                    if (_clientSocket.Client.Connected) _clientSocket.Client.Disconnect(false);

                    //serverStream.Close();
                    if (_clientSocket.Connected) _clientSocket.Close();

                    _clientSocket = new TcpClient();
                    return;
                }

                _logger.AddText("Device connected to: " + _ipAddress + ":" + _ipPort + Environment.NewLine,
                    (byte)DataDirection.Note, DateTime.Now);
                timer1.Enabled = true;
            }
            else
            {
                if (serialPort1.IsOpen) serialPort1.Close();

                serialPort1.PortName = comboBox_portname1.Text;
                serialPort1.BaudRate = _comSpeed;
                serialPort1.DataBits = 8;
                serialPort1.Handshake = Handshake.None;
                serialPort1.Parity = Parity.None;
                serialPort1.StopBits = StopBits.One;
                serialPort1.ReadTimeout = 500;
                serialPort1.WriteTimeout = 500;
                serialPort1.ReadBufferSize = 8192;
                try
                {
                    serialPort1.Open();
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Error opening netPort " + serialPort1.PortName + ": " + ex.Message);
                    return;
                }

                _logger.AddText("Port opened\r\n", (byte)DataDirection.Note, DateTime.Now);
            }

            if (!serialPort1.IsOpen && !IsClientConnected())
            {
                Button_disconnect_Click(this, EventArgs.Empty);
                return;
            }

            button_connect.Enabled = false;
            button_disconnect.Enabled = true;
            button_send.Enabled = true;
            button_sendAll.Enabled = true;
            button_sendCommand.Enabled = true;
            button_getConfig.Enabled = true;
            button_getStatus.Enabled = true;
            button_getSensor.Enabled = true;
            button_setTime.Enabled = true;
            button_reset.Enabled = true;
            comboBox_portname1.Enabled = false;
            comboBox_portspeed1.Enabled = false;
        }

        private void Button_disconnect_Click(object sender, EventArgs e)
        {
            if (comboBox_portname1.SelectedIndex == 0)
            {
                timer1.Enabled = false;
                try
                {
                    _clientSocket.Client.Disconnect(false);
                    _serverStream.Close();
                    _clientSocket.Close();
                    _clientSocket = new TcpClient();
                }
                catch (Exception ex)
                {
                    _logger.AddText("Disconnect error: " + ex + Environment.NewLine, (byte)DataDirection.Error, DateTime.Now);
                    return;
                }

                _logger.AddText("Device disconnected ...\r\n", (byte)DataDirection.Note, DateTime.Now);
            }
            else
            {
                try
                {
                    serialPort1.Close();
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Error closing netPort " + serialPort1.PortName + ": " + ex.Message);
                }

                _logger.AddText("Port disconnected ...\r\n", (byte)DataDirection.Note, DateTime.Now);
            }

            button_connect.Enabled = true;
            button_disconnect.Enabled = false;
            button_send.Enabled = false;
            button_sendAll.Enabled = false;
            button_sendCommand.Enabled = false;
            button_getConfig.Enabled = false;
            button_getStatus.Enabled = false;
            button_getSensor.Enabled = false;
            button_setTime.Enabled = false;
            button_reset.Enabled = false;
            comboBox_portname1.Enabled = true;
            ComboBox_portname1_SelectedIndexChanged(this, EventArgs.Empty);
        }

        private async void Button_send_Click(object sender, EventArgs e)
        {
            if (dataGridView_config.CurrentRow == null)
                return;

            var currentRow = dataGridView_config.CurrentCell.RowIndex;
            var currentCell = dataGridView_config.CurrentCell.ColumnIndex;

            if (currentCell != (int)Columns.CustomValue
                && currentCell != (int)Columns.DefaultValue)
                return;

            await SendParameter(currentRow, currentCell);
        }

        private void TextBox_customCommand_KeyUp(object sender, KeyEventArgs e)
        {
            if (button_sendCommand.Enabled && e.KeyCode == Keys.Enter)
                Button_sendCommand_Click(this, EventArgs.Empty);
        }

        private void Button_getConfig_Click(object sender, EventArgs e)
        {
            const string tmpStr = "get_config";
            var outStream = Encoding.ASCII.GetBytes(tmpStr);

            SendData(outStream, checkBox_addCrLf.Checked);
        }

        private void Button_setTime_Click(object sender, EventArgs e)
        {
            var timeToSet = DateTime.Now;

            var tmpStr = "set_time="
                         + timeToSet.Year.ToString("D4")
                         + "." + timeToSet.Month.ToString("D2")
                         + "." + timeToSet.Day.ToString("D2")
                         + " " + timeToSet.Hour.ToString("D2")
                         + ":" + timeToSet.Minute.ToString("D2")
                         + ":" + timeToSet.Second.ToString("D2");

            var outStream = Encoding.ASCII.GetBytes(tmpStr);
            SendData(outStream, checkBox_addCrLf.Checked);
        }

        private void Button_getStatus_Click(object sender, EventArgs e)
        {
            const string tmpStr = "get_status";
            var outStream = Encoding.ASCII.GetBytes(tmpStr);

            SendData(outStream, checkBox_addCrLf.Checked);
        }

        private void Button_getSensor_Click(object sender, EventArgs e)
        {
            const string tmpStr = "get_sensor";
            var outStream = Encoding.ASCII.GetBytes(tmpStr);

            SendData(outStream, checkBox_addCrLf.Checked);
        }

        private void ComboBox_portspeed1_SelectedIndexChanged(object sender, EventArgs e)
        {
            int.TryParse(comboBox_portspeed1.SelectedItem.ToString(), out _comSpeed);
        }

        private void DataGridView_config_CellValueChanged(object sender, DataGridViewCellEventArgs e)
        {
            var currentColumn = dataGridView_config.CurrentCell.ColumnIndex;
            if (currentColumn != (int)Columns.CustomValue) return;

            if (dataGridView_config.CurrentRow.Cells[currentColumn].Style.BackColor !=
                dataGridView_config.DefaultCellStyle.BackColor)
                dataGridView_config.CurrentRow.Cells[currentColumn].Style.BackColor =
                    dataGridView_config.DefaultCellStyle.BackColor;
        }

        private void DataGridView_config_KeyDown(object sender, KeyEventArgs e)
        {
            if (button_sendCommand.Enabled && e.KeyCode == Keys.Enter)
            {
                Button_send_Click(this, EventArgs.Empty);
                e.Handled = true;
            }
            if (e.KeyCode == Keys.C && e.Control)
                CopySample();
        }

        private async void Button_scanIp_ClickAsync(object sender, EventArgs e)
        {
            var savedText = button_scanIp.Text;
            button_scanIp.Text = "Scanning...";
            var taskList = new List<Task<Dictionary<IPAddress, string>>>();

            var addressBytes = IPAddress.Parse(_ipAddress).GetAddressBytes();
            for (var i = 0; i < 255; i++)
            {
                var task = new Func<int, Task<Dictionary<IPAddress, string>>>(p =>
                    Task.Run(async () => await PingAddress(addressBytes, p, _ipPort).ConfigureAwait(true))).Invoke(i);
                taskList.Add(task);
            }

            await Task.WhenAll(taskList.ToArray()).ConfigureAwait(true);

            foreach (var ip in taskList.Where(x => x.Result != null))
            {
                _logger.AddText(ip.Result?.FirstOrDefault() + Environment.NewLine,
                    (byte)DataDirection.Note, DateTime.Now);
            }

            button_scanIp.Text = savedText;
        }

        private void TextBox_dataLog_TextChanged(object sender, EventArgs e)
        {
            if (_autoScroll)
                textBox_dataLog.SelectionStart = textBox_dataLog.Text.Length;
            else
                textBox_dataLog.SelectionStart = _selStart;

            textBox_dataLog.ScrollToCaret();

            /*if (textBox_dataLog != null)
                _selStart = textBox_dataLog.SelectionStart;*/
        }

        private void ComboBox_portname1_KeyUp(object sender, KeyEventArgs e)
        {
            if (comboBox_portname1.Enabled && e.KeyCode == Keys.Enter)
            {
                button_connect.Focus();
                Button_connect_Click(this, EventArgs.Empty);
            }
        }

        private void ComboBox_portspeed1_KeyUp(object sender, KeyEventArgs e)
        {
            if (comboBox_portname1.Enabled && e.KeyCode == Keys.Enter)
            {
                button_connect.Focus();
                Button_connect_Click(this, EventArgs.Empty);
            }
        }

        private void Button_reset_Click(object sender, EventArgs e)
        {
            const string tmpStr = "reset";
            var outStream = Encoding.ASCII.GetBytes(tmpStr);

            SendData(outStream, checkBox_addCrLf.Checked);
        }

        private void CheckBox_DTR_CheckedChanged(object sender, EventArgs e)
        {
            serialPort1.DtrEnable = checkBox_DTR.Checked;
        }

        private void CheckBox_RTS_CheckedChanged(object sender, EventArgs e)
        {
            serialPort1.RtsEnable = checkBox_RTS.Checked;
        }

        private void UpdateDataLogCursorPosition(object sender, EventArgs e)
        {
            _selStart = textBox_dataLog.SelectionStart;
        }

        #endregion

        private void ContextMenuStrip_item_Opening(object sender, CancelEventArgs e)
        {
            contextMenuStrip_item.Items[0].Enabled = (textBox_dataLog.SelectionLength > 0);
        }

        private void ToolStripMenuItem_parse_Click(object sender, EventArgs e)
        {
            var newSettings = ParseConfigText(textBox_dataLog.SelectedText);

            foreach (var setting in newSettings)
            {
                var row = _configData.Rows.Cast<DataRow>().FirstOrDefault(n => (string)n.ItemArray[(int)Columns.ParameterName] == setting.Key);

                if (row != null)
                {
                    var n = _configData.Rows.IndexOf(row);
                    var o = row.ItemArray;
                    o[(int)Columns.CustomValue] = setting.Value;
                    var newRow = _configData.NewRow();
                    newRow.ItemArray = o;
                    _configData.Rows[n].Delete();
                    _configData.Rows.InsertAt(newRow, n);
                }
            }
        }

        private Dictionary<string, string> ParseConfigText(string text)
        {
            var result = new Dictionary<string, string>();

            var lineBuffer = text
                .Split(new[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries)
                .Where(n => n.Trim().Contains(": "))
                .ToList();

            for (var i = 0; i < lineBuffer.Count; i++)
            {
                var param = lineBuffer[i].Substring(0, lineBuffer[i].IndexOf(": "));
                var paramValue = lineBuffer[i].Substring(lineBuffer[i].IndexOf(": ") + 2);
                result.Add(param, paramValue);
            }

            return result;
        }

    }
}
