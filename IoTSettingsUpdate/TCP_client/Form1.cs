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

using IoTSettingsUpdate.Properties;

namespace IoTSettingsUpdate
{
    public partial class Form1 : Form
    {
        private TcpClient _clientSocket = new TcpClient();
        private static NetworkStream _serverStream;
        private string _ipAddress;
        private int _ipPort;
        private string _comName;
        private int _comSpeed;
        private int _replyTimeout = 10000;
        private readonly List<char> _stringsDivider = new List<char> { '\x0d', '\x0a' };

        private string _unparsedData = "";

        //private int _maxLogLength = 4096;
        private bool _autoScroll;

        private readonly DataTable _configData = new DataTable();

        private readonly string[] _columnNames = { "Parameter", "Default Value", "New value", "Reply string" };

        private enum Columns
        {
            Parameter,
            DefaultValue,
            NewValue,
            ReplyString
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

        private volatile List<string> _inputStrings = new List<string>();
        private volatile bool _waitReply;
        private volatile bool _sending;

        #region Data acquisition

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
                button_receive.Enabled = true;
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
            comboBox_portname1.Enabled = false;
            comboBox_portspeed1.Enabled = false;
        }

        private void Button_disconnect_Click(object sender, EventArgs e)
        {
            if (comboBox_portname1.SelectedIndex == 0)
            {
                timer1.Enabled = false;
                button_receive.Enabled = false;
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
            comboBox_portname1.Enabled = true;
            ComboBox_portname1_SelectedIndexChanged(this, EventArgs.Empty);
        }

        private void Button_send_Click(object sender, EventArgs e)
        {
            if (dataGridView_config.CurrentRow == null)
                return;

            var currentRow = dataGridView_config.CurrentCell.RowIndex;
            var currentCell = dataGridView_config.CurrentCell.ColumnIndex;

            if (currentCell != (int)Columns.NewValue
                && currentCell != (int)Columns.DefaultValue)
                return;

            SendParameter(currentRow, currentCell);
        }

        private void Button_sendAll_Click(object sender, EventArgs e)
        {
            if (_sending)
            {
                _sending = false;
                button_send.Enabled = true;
                button_sendAll.Text = "Send all";
                button_sendCommand.Enabled = true;
                button_getConfig.Enabled = true;
                button_getStatus.Enabled = true;
                button_getSensor.Enabled = true;
                button_setTime.Enabled = true;
                comboBox_portname1.Enabled = true;
                return;
            }

            var currentCell = dataGridView_config.CurrentCell.ColumnIndex;

            if (currentCell != (int)Columns.NewValue
                && currentCell != (int)Columns.DefaultValue)
                return;

            _sending = true;
            button_send.Enabled = false;
            button_sendAll.Text = "Stop";
            button_sendCommand.Enabled = false;
            button_getConfig.Enabled = false;
            button_getStatus.Enabled = false;
            button_getSensor.Enabled = false;
            button_setTime.Enabled = false;
            comboBox_portname1.Enabled = true;

            for (var currentRow = 0; currentRow < _configData.Rows.Count; currentRow++)
            {
                if (!_sending) break;
                SendParameter(currentRow, currentCell);
            }

            _sending = false;
            button_send.Enabled = true;
            button_sendAll.Text = "Send all";
            button_sendCommand.Enabled = true;
            button_getConfig.Enabled = true;
            button_getStatus.Enabled = true;
            button_getSensor.Enabled = true;
            button_setTime.Enabled = true;
            comboBox_portname1.Enabled = true;
        }

        private void SendParameter(int currentRow, int currentCell)
        {
            _waitReply = false;
            _inputStrings.Clear();

            var par = _configData.Rows[currentRow].ItemArray[currentCell].ToString();

            if (string.IsNullOrEmpty(par))
                par = " ";
            var tmpStr = _configData.Rows[currentRow].ItemArray[(int)Columns.Parameter] + "=" + par;

            var outStream = new List<byte>();
            outStream.AddRange(Encoding.ASCII.GetBytes(tmpStr));

            var waitSample = _configData.Rows[currentRow].ItemArray[(int)Columns.ReplyString].ToString();

            if (!string.IsNullOrEmpty(waitSample)) _waitReply = true;
            SendData(outStream.ToArray(), checkBox_addCrLf.Checked);

            if (string.IsNullOrEmpty(waitSample)) return;

            dataGridView_config.Rows[currentRow].Cells[currentCell].Style.BackColor = Color.Yellow;
            if (WaitForReply(waitSample, _replyTimeout))
            {
                dataGridView_config.Rows[currentRow].Cells[currentCell].Style.BackColor = Color.GreenYellow;
            }
            else
            {
                var row = currentRow;
                dataGridView_config.Rows[row].Cells[currentCell].Style.BackColor = Color.Red;
            }

            _waitReply = false;
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

        private void Button_receive_Click(object sender, EventArgs e)
        {
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
            else
            {
                TryReconnect();
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
            foreach (var c in tcpConnections)
            {
                var stateOfConnection = c.State;
                if (_clientSocket != null)
                    try
                    {
                        if (c.LocalEndPoint.Equals(_clientSocket.Client.LocalEndPoint) &&
                            c.RemoteEndPoint.Equals(_clientSocket.Client.RemoteEndPoint))
                        {
                            if (stateOfConnection == TcpState.Established)
                                return true;
                            return false;
                        }
                    }
                    catch (Exception ex)
                    {
                        _logger.AddText("Socket error: " + ex + Environment.NewLine,
                            (byte)DataDirection.Error, DateTime.Now);
                        return false;
                    }
                else
                    return false;
            }

            return false;
        }

        private void OpenFileDialog1_FileOk(object sender, CancelEventArgs e)
        {
            var data = new List<byte>();
            try
            {
                data.AddRange(File.ReadAllBytes(openFileDialog1.FileName));
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error reading to file " + openFileDialog1.FileName + ": " + ex.Message);
            }

            var paramList = ParseConfig(data.ToArray());

            if (paramList?.Count > 0)
            {
                _configData.Rows.Clear();
                foreach (var par in paramList)
                {
                    var newRow = _configData.NewRow();
                    newRow[(int)Columns.Parameter] = par.Key;
                    var n = par.Value.IndexOf(" //", StringComparison.Ordinal);
                    if (n > 0)
                    {
                        newRow[(int)Columns.DefaultValue] = par.Value.Substring(0, n);
                        newRow[(int)Columns.ReplyString] = par.Value.Substring(n + 3);
                    }
                    else
                    {
                        newRow[1] = par.Value;
                    }

                    _configData.Rows.Add(newRow);
                }
            }
        }

        private Dictionary<string, string> ParseConfig(byte[] data)
        {
            var dict = new Dictionary<string, string>();
            if (data?.Length == 0) return dict;
            var stringsCollection = ConvertBytesToStrings(data, ref _unparsedData);

            if (stringsCollection.Length <= 0) return null;

            foreach (var s in stringsCollection)
            {
                var pos = s.IndexOf('=');
                if (pos > 0)
                {
                    var paramName = s.Substring(0, pos);
                    var paramValue = s.Substring(pos + 1);
                    if (!dict.ContainsKey(paramName))
                        dict.Add(paramName, paramValue);
                    else
                        dict[paramName] = paramValue;
                }
            }

            return dict;
        }

        private void ProcessInput(byte[] data)
        {
            if (data == null || data.Length == 0) return;

            var incomingStr = Encoding.Default.GetString(data.ToArray());
            _logger.AddText(incomingStr, (byte)DataDirection.Received, DateTime.Now);

            var stringsCollection = ConvertBytesToStrings(data, ref _unparsedData);
            foreach (var s in stringsCollection)
            {
                if (_waitReply) _inputStrings.Add(s);
            }
        }

        private bool WaitForReply(string stringStart, int timeout)
        {
            var reply = false;

            var samples = stringStart.Split(';');
            var start = DateTime.Now;
            while (DateTime.Now.Subtract(start).TotalMilliseconds < timeout)
            {
                Application.DoEvents();
                Thread.Sleep(100);

                for (var i = 0; i < _inputStrings.Count; i++)
                {
                    foreach (var s in samples)
                        if (_inputStrings[i].StartsWith(s))
                        {
                            reply = true;
                            _inputStrings.RemoveAt(i);
                            break;
                        }

                    if (reply) break;
                }

                if (reply) break;
            }

            return reply;
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
            if (IsClientConnected())
            {
                Button_receive_Click(this, EventArgs.Empty);
            }
            else if (!serialPort1.IsOpen)
            {
                TryReconnect();
            }
        }

        private void TryReconnect()
        {
            _logger.AddText("not connected\r\n", (byte)DataDirection.Note, DateTime.Now);
            Button_disconnect_Click(this, EventArgs.Empty);
        }

        private string[] ConvertBytesToStrings(IEnumerable<byte> data, ref string unparsedData)
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
                        stringCollection.Add(unparsedData);
                        unparsedData = "";
                    }
                }
                else
                {
                    unparsedData += (char)t;
                }
            }

            return stringCollection.ToArray();
        }

        private void ParseIpAddress(string ipText)
        {
            var portPosition = ipText.IndexOf(':');
            if (portPosition == 0)
            {
                _ipAddress = Settings.Default.DefaultAddress;
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
                _ipPort = Settings.Default.DefaultPort;
            }
        }

        private string PrepareConfig()
        {
            var str = new StringBuilder();

            foreach (DataRow row in _configData.Rows)
            {
                var value = row[(int)Columns.NewValue].ToString().Length == 0 ? row[(int)Columns.DefaultValue] : row[(int)Columns.NewValue];
                str.AppendLine(row[(int)Columns.Parameter] + "=" + value + " //" +
                               row[(int)Columns.ReplyString]);
            }

            return str.ToString();
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

                    var tmp = "";
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

        #endregion

        #region GUI

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint | ControlStyles.DoubleBuffer, true);

            serialPort1.Encoding = Encoding.GetEncoding(Settings.Default.CodePage);
            _ipAddress = Settings.Default.DefaultAddress;
            _ipPort = Settings.Default.DefaultPort;
            _comName = Settings.Default.DefaultComName;
            _comSpeed = Settings.Default.DefaultComSpeed;
            _replyTimeout = Settings.Default.ReplyTimeout;
            textBox_replyTimeout.Text = _replyTimeout.ToString();
            _stringsDivider.AddRange(Settings.Default.StringsDivider);

            SerialPopulate();
            _logger = new TextLogger(this)
            {
                FilterZeroChar = true,
                AutoSave = true,
                AutoScroll = checkBox_autoScroll.Checked,
                DefaultTextFormat = TextLogger.TextFormat.PlainText,
                DefaultTimeFormat = TextLogger.TimeFormat.LongTime,
                LogFileName = "loader.log",
                LineTimeLimit = 500
            };
            _logger.Channels = _directions;

            textBox_dataLog.DataBindings.Add("Text", _logger, "Text", false, DataSourceUpdateMode.OnPropertyChanged);

            foreach (var col in _columnNames) _configData.Columns.Add(col);

            _configData.Columns[(int)Columns.Parameter].ReadOnly = true;
            _configData.Columns[(int)Columns.DefaultValue].ReadOnly = true;
            _configData.Columns[(int)Columns.ReplyString].ReadOnly = true;

            dataGridView_config.AllowUserToAddRows = false;
            dataGridView_config.RowHeadersVisible = false;
            dataGridView_config.DataSource = _configData;
            foreach (DataGridViewColumn col in dataGridView_config.Columns)
            {
                col.SortMode = DataGridViewColumnSortMode.NotSortable;
                col.AutoSizeMode = DataGridViewAutoSizeColumnMode.AllCells;
            }

            _autoScroll = _logger.AutoScroll = checkBox_autoScroll.Checked;
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            timer1.Enabled = false;
            if (_clientSocket.Client.Connected)
            {
                _clientSocket.Client.Disconnect(false);
                _clientSocket.Close();
                //_clientSocket.Dispose();
            }

            Settings.Default.DefaultAddress = _ipAddress;
            Settings.Default.DefaultPort = _ipPort;
            if (comboBox_portname1.SelectedIndex > 0)
                Settings.Default.DefaultComName = comboBox_portname1.Items[comboBox_portname1.SelectedIndex].ToString();
            else
                Settings.Default.DefaultComName = "";
            Settings.Default.DefaultComSpeed = int.Parse(comboBox_portspeed1.SelectedItem.ToString());
            Settings.Default.Save();
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
            openFileDialog1.Filter = "CFG files|*.cfg|All files|*.*";
            openFileDialog1.ShowDialog();
        }

        private void Button_save_Click(object sender, EventArgs e)
        {
            saveFileDialog1.Title = "Save config as text...";
            saveFileDialog1.DefaultExt = "cfg";
            saveFileDialog1.Filter = "CFG files|*.cfg|All files|*.*";
            saveFileDialog1.FileName = "config" + ".cfg";
            saveFileDialog1.ShowDialog();
        }

        private void SaveFileDialog1_FileOk(object sender, CancelEventArgs e)
        {
            var configData = PrepareConfig();
            try
            {
                File.WriteAllText(saveFileDialog1.FileName, configData);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error writing to file " + saveFileDialog1.FileName + ": " + ex.Message);
            }
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

        private void TextBox_customCommand_KeyUp(object sender, KeyEventArgs e)
        {
            if (button_sendCommand.Enabled && e.KeyCode == Keys.Enter) Button_sendCommand_Click(this, EventArgs.Empty);
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
            var currentCell = dataGridView_config.CurrentCell.ColumnIndex;
            if (currentCell != (int)Columns.NewValue) return;

            if (dataGridView_config.CurrentRow.Cells[currentCell].Style.BackColor !=
                dataGridView_config.DefaultCellStyle.BackColor)
                dataGridView_config.CurrentRow.Cells[currentCell].Style.BackColor =
                    dataGridView_config.DefaultCellStyle.BackColor;
        }

        private void DataGridView_config_KeyDown(object sender, KeyEventArgs e)
        {
            if (button_sendCommand.Enabled && e.KeyCode == Keys.Enter)
            {
                Button_send_Click(this, EventArgs.Empty);
                e.Handled = true;
            }
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
                _logger.AddText(ip.Result?.FirstOrDefault() + Environment.NewLine,
                    (byte)DataDirection.Note, DateTime.Now);
            button_scanIp.Text = savedText;
        }

        private void TextBox_dataLog_TextChanged(object sender, EventArgs e)
        {
            if (_autoScroll)
            {
                textBox_dataLog.SelectionStart = textBox_dataLog.Text.Length;
                textBox_dataLog.ScrollToCaret();
            }
            /*else
            {
                textBox_dataLog.SelectionStart = _selStart;
                textBox_dataLog.ScrollToCaret();
            }

            if (textBox_dataLog != null) _selStart = textBox_dataLog.SelectionStart;*/
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
            button_connect.Focus();
            if (comboBox_portname1.Enabled && e.KeyCode == Keys.Enter)
            {
                button_connect.Focus();
                Button_connect_Click(this, EventArgs.Empty);
            }
        }

        #endregion

    }
}
