using ChartPlotMQTT.Properties;

using MQTTnet;
using MQTTnet.Client;
using MQTTnet.Client.Disconnecting;
using MQTTnet.Client.Options;
using MQTTnet.Client.Receiving;
using MQTTnet.Implementations;

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Reflection;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Security.Permissions;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using LiteDB;
using MQTTnet.Diagnostics.Logger;
using static ChartPlotMQTT.LiteDbLocal;

namespace ChartPlotMQTT
{
    public partial class Form1 : Form
    {
        private const string DeviceName = "DeviceName";
        private const string DeviceMac = "DeviceMAC";
        private const string FwVersion = "FW Version";

        private string _ipAddress = string.Empty;
        private int _ipPortMqtt = 1883;
        private int _ipPortRest = 8080;
        private string _login = string.Empty;
        private string _password = string.Empty;
        private string _subscribeTopic = string.Empty;
        private string _publishTopic = string.Empty;
        private bool _autoConnect;
        private bool _autoReConnect;
        private volatile bool _forcedDisconnect;
        private int _keepTime = 60;
        private bool _keepLocalDb;
        private const int ReconnectTimeOut = 10000;

        private static readonly IMqttNetLogger Logger = new MqttNetNullLogger();
        private readonly IMqttClient _mqttClient = new MqttClient(new MqttClientAdapterFactory(Logger), Logger);

        private const string DefaultFormCaption = "ChartPlotMQTT";
        private string _clientId = "ChartPlotter";
        private bool _initTimeSet;
        private bool _valueRangeSet;

        private readonly Color[] _currentColor =
        {
            Color.Black,
            Color.Red,
            Color.Green,
            Color.Blue,
            Color.Brown,
            Color.Yellow,
            Color.Orange,
            Color.Cyan,
            Color.Lime,
            Color.Magenta,
            Color.Gold,
            Color.Violet,
            Color.YellowGreen,
            Color.Indigo,
            Color.Coral,
            Color.Pink,
            Color.Silver
        };

        private float _maxShowValue;
        private float _minShowValue;

        private DateTime _maxAllowedTime;
        private DateTime _minAllowedTime;

        private DateTime _maxShowTime;
        private DateTime _minShowTime;

        private const double ScaleFactor = 10;
        private readonly double _minChartValue = Convert.ToDouble(decimal.MinValue) / ScaleFactor;
        private readonly double _maxChartValue = Convert.ToDouble(decimal.MaxValue) / ScaleFactor;

        private readonly PlotItems _plotList = new PlotItems();

        private TextLogger.TextLogger _logger;

        private enum DataDirection
        {
            Received,
            Sent,
            Info,
            Error
        }

        private readonly Dictionary<byte, string> _directions = new Dictionary<byte, string>
        {
            {(byte) DataDirection.Received, "<<"},
            {(byte) DataDirection.Sent, ">>"},
            {(byte) DataDirection.Info, "**"},
            {(byte) DataDirection.Error, "!!"}
        };

        private string _saveItem = "";

        private LiteDbLocal _recordsDb;

        #region Data acquisition

        private async void Button_connect_Click(object sender, EventArgs e)
        {
            Invoke((MethodInvoker)delegate
           {
               button_connect.Text = "Connecting...";
               button_connect.Enabled = false;
           });

            DbOpen();
            var options = new MqttClientOptionsBuilder()
                .WithClientId(_clientId + (checkBox_addTimeStamp.Enabled
                    ? DateTime.Now.ToString(CultureInfo.InvariantCulture)
                    : ""))
                .WithTcpServer(_ipAddress, _ipPortMqtt)
                .WithCredentials(_login, _password)
                .WithCleanSession()
                .Build();

            try
            {
                await _mqttClient.ConnectAsync(options, CancellationToken.None).ConfigureAwait(true);
                _logger.AddText("Device connected" + Environment.NewLine, (byte)DataDirection.Info, DateTime.Now);
                // Subscribe to a topic
                if (_subscribeTopic.Length > 0)
                {
                    await _mqttClient.SubscribeAsync(new MqttTopicFilterBuilder().WithTopic(_subscribeTopic).Build())
                        .ConfigureAwait(false);
                    _logger.AddText("Topic subscribed" + Environment.NewLine, (byte)DataDirection.Info, DateTime.Now);
                }
            }
            catch (Exception ex)
            {
                _logger.AddText("Exception: " + ex.Message + Environment.NewLine, (byte)DataDirection.Error, DateTime.Now);
                Invoke((MethodInvoker)delegate
               {
                   button_connect.Text = "Connect";
                   button_connect.Enabled = true;
                   Button_disconnect_Click(this, EventArgs.Empty);
               });
                return;
            }

            _mqttClient.ApplicationMessageReceivedHandler =
                new MqttApplicationMessageReceivedHandlerDelegate(Mqtt_DataReceived);
            if (!_mqttClient.IsConnected) Button_disconnect_Click(this, EventArgs.Empty);

            _mqttClient.DisconnectedHandler = new MqttClientDisconnectedHandlerDelegate(MqttDisconnectedHandler);

            _forcedDisconnect = false;

            Invoke((MethodInvoker)delegate
           {
               timer_reconnect.Enabled = false;
               Text = DefaultFormCaption + ": connected";
               button_connect.Text = "Connect";
               button_connect.Enabled = false;
               button_disconnect.Enabled = true;
               button_send.Enabled = true;
           });
        }

        private void MqttDisconnectedHandler(MqttClientDisconnectedEventArgs e)
        {
            if (_autoReConnect && !_forcedDisconnect)
            {
                Invoke((MethodInvoker)delegate
                {
                    timer_reconnect.Interval = ReconnectTimeOut;
                    timer_reconnect.Enabled = true;
                });

                _logger.AddText("Device disconnected. Reconnecting..." + Environment.NewLine, (byte)DataDirection.Error, DateTime.Now);
            }
            else
            {
                Button_disconnect_Click(this, EventArgs.Empty);
            }
        }

        private void Button_disconnect_Click(object sender, EventArgs e)
        {
            if (e != EventArgs.Empty) _forcedDisconnect = true;

            _mqttClient.ApplicationMessageReceivedHandler = null;
            _mqttClient.DisconnectedHandler = null;

            try
            {
                _mqttClient.DisconnectAsync().ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                _logger.AddText("Disconnect error: " + ex, (byte)DataDirection.Error, DateTime.Now);
            }

            if (_keepLocalDb) _recordsDb?.Dispose();

            _logger.AddText("Device disconnected", (byte)DataDirection.Info, DateTime.Now);
            Invoke((MethodInvoker)delegate
           {
               Text = DefaultFormCaption + ": disconnected";
               button_connect.Enabled = true;
               button_disconnect.Enabled = false;
               button_send.Enabled = false;
               textBox_mqttServer.Enabled = true;
           });
        }

        private void Button_send_ClickAsync(object sender, EventArgs e)
        {
            if (_mqttClient.IsConnected)
            {
                var outStream = new List<byte>();
                outStream.AddRange(checkBox_hex.Checked
                    ? Accessory.ConvertHexToByteArray(textBox_message.Text)
                    : Encoding.ASCII.GetBytes(textBox_message.Text));

                var message = new MqttApplicationMessageBuilder()
                    .WithTopic(_publishTopic)
                    .WithPayload(Encoding.UTF8.GetString(outStream.ToArray()))
                    .Build();

                try
                {
                    _mqttClient.PublishAsync(message).ConfigureAwait(false);
                }
                catch (Exception ex)
                {
                    _logger.AddText("Write error: " + ex, (byte)DataDirection.Error, DateTime.Now);
                }

                var tmpStr = "[" + _publishTopic + "] ";
                if (checkBox_hex.Checked)
                    tmpStr += "[" + Accessory.ConvertByteArrayToHex(outStream.ToArray()) + "]" + Environment.NewLine;
                else
                    tmpStr += Encoding.ASCII.GetString(outStream.ToArray()) + Environment.NewLine;

                _logger.AddText(tmpStr, (byte)DataDirection.Sent, DateTime.Now,
                    TextLogger.TextLogger.TextFormat.AutoReplaceHex);
            }
            else
            {
                _logger.AddText("...not connected" + Environment.NewLine, (byte)DataDirection.Error, DateTime.Now);
                Button_disconnect_Click(this, EventArgs.Empty);

                if (_autoReConnect && !_forcedDisconnect)
                {
                    _logger.AddText("Reconnecting..." + Environment.NewLine, (byte)DataDirection.Info, DateTime.Now);
                    Button_connect_Click(this, EventArgs.Empty);
                }
            }
        }

        private void Mqtt_DataReceived(MqttApplicationMessageReceivedEventArgs e)
        {
            if (e?.ApplicationMessage?.Payload == null || e.ApplicationMessage.Payload.Length == 0) return;

            var tmpStr = Encoding.UTF8.GetString(e.ApplicationMessage.Payload);

            Dictionary<string, string> stringsSet;
            var jss = new JavaScriptSerializer();
            try
            {
                stringsSet = jss.Deserialize<Dictionary<string, string>>(tmpStr);
            }
            catch (Exception)
            {
                return;
            }

            tmpStr = "";
            if (checkBox_hex.Checked)
                tmpStr = "[" + e.ApplicationMessage.Topic + "] " + Accessory.ConvertStringToHex(tmpStr) +
                         Environment.NewLine;
            else
                foreach (var s in stringsSet)
                    tmpStr += "[" + e.ApplicationMessage.Topic + "] " + s.Key + "=" + s.Value + Environment.NewLine;

            _logger.AddText(tmpStr, (byte)DataDirection.Received, DateTime.Now,
                TextLogger.TextLogger.TextFormat.AutoReplaceHex);

            var recordTime = DateTime.Now;

            /*foreach (var s in stringsSet)
                if (s.Key == "Time" && DateTime.TryParse(s.Value, out recordTime))
                    break;*/

            Invoke((MethodInvoker)delegate { UpdateChart(stringsSet, recordTime, _keepLocalDb); });
        }

        private void OpenFileDialog1_FileOk(object sender, CancelEventArgs e)
        {
            var dataCollection = new List<LiteDbLocal.SensorDataRec>();
            try
            {
                dataCollection = ImportJsonFile(openFileDialog1.FileName);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error reading to file " + openFileDialog1.FileName + ": " + ex.Message);
            }

            if (dataCollection == null) return;

            checkedListBox_params.Items.Clear();
            _plotList.Clear();
            InitChart();
            checkBox_autoRangeValue.Checked = true;

            if (trackBar_min.Value != trackBar_min.Minimum) trackBar_min.Value = trackBar_min.Minimum;
            TrackBar_min_Scroll(this, EventArgs.Empty);

            if (trackBar_max.Value != trackBar_max.Maximum) trackBar_max.Value = trackBar_max.Maximum;
            TrackBar_max_Scroll(this, EventArgs.Empty);

            foreach (var record in dataCollection) UpdateChart(record, false);
        }

        private async Task<bool> GetDataFromRest(int time, int retryCount = 3)
        {
            //http://jekyll-server:8080/api/Sensors/GetLatestRecords?minutes=60

            var baseUrl = "http://" + _ipAddress + ":" + _ipPortRest;
            var servicePath = "/api/Sensors/GetRecords?minutes=" + time;

            var credentials = new NetworkCredential(_login, _password);

            using (var client =
                new HttpClient(new HttpClientHandler { Credentials = credentials, PreAuthenticate = true }))
            {
                client.Timeout = TimeSpan.FromMinutes(1);
                var resultContentString = "";
                //client.Timeout = TimeSpan.FromSeconds(30);
                client.BaseAddress = new Uri(baseUrl);

                var reqResult = false;
                do
                {
                    retryCount--;
                    try
                    {
                        var result = await client.GetAsync(servicePath).ConfigureAwait(true);

                        result.EnsureSuccessStatusCode();

                        var r = result.IsSuccessStatusCode;
                        if (!r)
                        {
                            await Task.Delay(1000);
                            continue;
                        }

                        resultContentString = await result.Content.ReadAsStringAsync();
                        reqResult = true;
                    }
                    catch (Exception exception)
                    {
                        _logger.AddText("REST error: " + exception.Message + Environment.NewLine, (byte)DataDirection.Error, DateTime.Now);
                    }
                } while (retryCount > 0 && !reqResult);

                if (!reqResult) return false;

                if (string.IsNullOrEmpty(resultContentString)) return true;

                var restoredRange = new List<LiteDbLocal.SensorDataRec>();
                try
                {
                    restoredRange = ImportJsonString(resultContentString);
                }
                catch (Exception ex)
                {
                    _logger.AddText("Error parsing data from url " + baseUrl + servicePath + ": " + ex.Message,
                        (byte)DataDirection.Error, DateTime.Now);
                }

                if (restoredRange != null)
                {
                    var screenWidth = SystemInformation.PrimaryMonitorSize.Width;
                    var ratio = restoredRange.Count / screenWidth;
                    if (ratio < 1) ratio = 1;
                    var counter = 0;
                    var accumulatedRecord = new SensorDataRec
                    {
                        ValueList = new List<ValueItemRec>()
                    };
                    var recordTimes = new DateTime[ratio];
                    foreach (var recordsGroup in restoredRange.GroupBy(n => n.DeviceName + n.DeviceMAC))
                    {
                        foreach (var record in recordsGroup.OrderBy(n => n.Time))
                        {
                            if (counter >= ratio)
                            {
                                counter = 0;
                                if (accumulatedRecord.ValueList.Count > 0)
                                {
                                    if (ratio == 1) accumulatedRecord.Time = recordTimes[0];
                                    else accumulatedRecord.Time = recordTimes.Min().AddSeconds(recordTimes.Max().Subtract(recordTimes.Min()).TotalSeconds / 2);

                                    if (accumulatedRecord.DeviceName == null && accumulatedRecord.DeviceMAC == null)
                                    {
                                        accumulatedRecord.DeviceMAC = record.DeviceMAC;
                                        accumulatedRecord.DeviceName = record.DeviceName;
                                    }

                                    UpdateChart(accumulatedRecord, false);
                                }

                                accumulatedRecord = new SensorDataRec
                                {
                                    DeviceMAC = record.DeviceMAC,
                                    DeviceName = record.DeviceName,
                                    ValueList = new List<ValueItemRec>()
                                };
                            }

                            if (ratio == 1)
                            {
                                recordTimes[0] = record.Time;
                                accumulatedRecord.ValueList = record.ValueList;
                            }
                            else
                            {
                                recordTimes[counter] = record.Time;

                                foreach (var sensor in record.ValueList)
                                {
                                    var valueIndex = -1;
                                    for (var i = 0; i < accumulatedRecord.ValueList.Count; i++)
                                        if (accumulatedRecord.ValueList[i].ValueType == sensor.ValueType)
                                        {
                                            valueIndex = i;
                                            break;
                                        }
                                    if (valueIndex == -1)
                                    {
                                        accumulatedRecord.ValueList.Add(sensor);
                                    }
                                    else
                                    {
                                        accumulatedRecord.ValueList[valueIndex].Value += sensor.Value;
                                        accumulatedRecord.ValueList[valueIndex].Value /= 2;
                                    }
                                }
                            }

                            counter++;
                        }

                        if (accumulatedRecord.ValueList.Count > 0)
                        {
                            UpdateChart(accumulatedRecord, false);
                            accumulatedRecord = new SensorDataRec
                            {
                                ValueList = new List<ValueItemRec>()
                            };
                        }
                    }

                }
            }

            return true;
        }

        private void Timer_reconnect_Tick(object sender, EventArgs e)
        {
            _logger.AddText("Trying to reconnect..." + Environment.NewLine, (byte)DataDirection.Error, DateTime.Now);
            Button_connect_Click(this, EventArgs.Empty);
        }

        #endregion

        #region Helpers

        private bool ExportJsonFile(IEnumerable<SensorDataRec> data, string fileName, string item = null)
        {
            var filteredData = new List<LiteDbLocal.SensorDataRec>();
            if (!string.IsNullOrEmpty(item))
                foreach (var pack in data)
                {
                    var newPack = new LiteDbLocal.SensorDataRec
                    {
                        Time = pack.Time
                    };
                    foreach (var line in newPack.ValueList)
                        if (line.ValueType == item)
                            newPack.ValueList.Add(line);

                    if (newPack.ValueList.Count > 0) filteredData.Add(newPack);
                }
            else
                filteredData = data.ToList();

            var jsonSerializer = new DataContractJsonSerializer(typeof(List<LiteDbLocal.SensorDataRec>));
            try
            {
                var fileStream = File.Open(fileName, FileMode.Create);
                jsonSerializer.WriteObject(fileStream, filteredData);
                fileStream.Close();
                fileStream.Dispose();
            }
            catch (Exception ex)
            {
                _logger.AddText("JSON parse error: " + ex.Message + Environment.NewLine, (byte)DataDirection.Error,
                    DateTime.Now);
                return false;
            }

            return true;
        }

        private List<SensorDataRec> ImportJsonFile(string fileName)
        {
            List<LiteDbLocal.SensorDataRec> newValues;
            var jsonSerializer = new DataContractJsonSerializer(typeof(List<LiteDbLocal.SensorDataRec>));
            try
            {
                var fileStream = File.Open(fileName, FileMode.Open);

                newValues = (List<LiteDbLocal.SensorDataRec>)jsonSerializer.ReadObject(fileStream);
                fileStream.Close();
                fileStream.Dispose();
            }
            catch (Exception ex)
            {
                _logger.AddText("JSON parse error: " + ex.Message + Environment.NewLine, (byte)DataDirection.Error,
                    DateTime.Now);
                newValues = null;
            }

            return newValues;
        }

        private List<SensorDataRec> ImportJsonString(string data)
        {
            List<LiteDbLocal.SensorDataRec> newValues;
            var settings = new DataContractJsonSerializerSettings
            {
                DateTimeFormat = new DateTimeFormat("yyyy-MM-ddTHH:mm:ss.FFFK")
            };

            var jsonSerializer = new DataContractJsonSerializer(typeof(List<LiteDbLocal.SensorDataRec>), settings);
            try
            {
                using (var memo = new MemoryStream(Encoding.Unicode.GetBytes(data)))
                {
                    newValues = (List<LiteDbLocal.SensorDataRec>)jsonSerializer.ReadObject(memo);
                }
            }
            catch (Exception ex)
            {
                _logger.AddText("JSON parse error: " + ex.Message + Environment.NewLine, (byte)DataDirection.Error,
                    DateTime.Now);
                newValues = null;
            }

            return newValues;
        }

        private void ParseIpAddress(string ipText)
        {
            if (string.IsNullOrEmpty(ipText))
            {
                _ipAddress = Settings.Default.DefaultAddress;
                _ipPortMqtt = Settings.Default.DefaultPort;
                return;
            }

            var portPosition = ipText.IndexOf(':');
            if (portPosition > 0)
            {
                _ipAddress = ipText.Substring(0, portPosition);
                if (!int.TryParse(ipText.Substring(portPosition + 1), out _ipPortMqtt))
                    _ipPortMqtt = Settings.Default.DefaultPort;
            }
            else if (portPosition == 0)
            {
                _ipAddress = Settings.Default.DefaultAddress;
                if (!int.TryParse(ipText.Substring(1), out _ipPortMqtt)) _ipPortMqtt = Settings.Default.DefaultPort;
            }
            else
            {
                _ipAddress = ipText;
                _ipPortMqtt = Settings.Default.DefaultPort;
            }
        }

        private void ReCalculateCheckBoxSize()
        {
            if (checkedListBox_params.Items.Count <= 0) return;

            checkedListBox_params.Height =
                checkedListBox_params.GetItemRectangle(checkedListBox_params.Items.Count - 1).Height *
                (checkedListBox_params.Items.Count + 1);
            if (checkedListBox_params.Height > chart1.Height / 2) checkedListBox_params.Height = chart1.Height / 2;

            checkedListBox_params.Top = textBox_fromTime.Top - checkedListBox_params.Height;

            checkedListBox_params.Left = textBox_fromTime.Right - checkedListBox_params.Width;
            checkedListBox_params.Width = checkedListBox_params.PreferredSize.Width;
        }

        private void SetCursorInterval()
        {
            var max = chart1.ChartAreas[0].AxisY.Maximum;
            var min = chart1.ChartAreas[0].AxisY.Minimum;
            var h = chart1.ChartAreas[0].Position.Height * chart1.Height / 100;
            chart1.ChartAreas[0].CursorY.Interval = (max - min) / h;

            max = chart1.ChartAreas[0].AxisX.Maximum;
            min = chart1.ChartAreas[0].AxisX.Minimum;
            var w = chart1.ChartAreas[0].Position.Width * chart1.Width / 100;
            chart1.ChartAreas[0].CursorX.Interval = (max - min) / w;
        }

        private void RecheckMinMaxValue()
        {
            if (checkBox_autoRangeValue.CheckState == CheckState.Unchecked) return;

            _valueRangeSet = false;
            for (var n = 0; n < _plotList.Count; n++)
            {
                if (!_plotList.GetActive(_plotList.ItemName(n))) continue;

                if (!_valueRangeSet)
                {
                    _maxShowValue = _plotList.GetValueRange(n).MaxValue;
                    _minShowValue = _plotList.GetValueRange(n).MinValue;
                    _valueRangeSet = true;
                }
                else if (_plotList.GetValueRange(n).MinValue < _minShowValue)
                {
                    _minShowValue = _plotList.GetValueRange(n).MinValue;
                }
                else if (_plotList.GetValueRange(n).MaxValue > _maxShowValue)
                {
                    _maxShowValue = _plotList.GetValueRange(n).MaxValue;
                }
            }
        }

        private void DbOpen()
        {
            if (!_keepLocalDb) return;
            if (_recordsDb == null || _recordsDb.Disposed)
            {
                if (string.IsNullOrEmpty(Settings.Default.DbPassword))
                {
                    _recordsDb = new LiteDbLocal(Settings.Default.DbFileName, _clientId);
                }
                else
                {
                    var connString = new ConnectionString
                    {
                        Filename = Settings.Default.DbFileName,
                        Password = Settings.Default.DbPassword
                    };
                    _recordsDb = new LiteDbLocal(connString, _clientId);
                }
            }
        }

        [ReflectionPermission(SecurityAction.Demand, MemberAccess = true)]
        private void ResetExceptionState(Control control)
        {
            typeof(Control).InvokeMember("SetState", BindingFlags.NonPublic |
                                                     BindingFlags.InvokeMethod | BindingFlags.Instance, null,
                control, new object[] { 0x400000, false });
        }

        private string MacToString(byte[] mac)
        {
            if (mac.Length != 6) return "";

            return string.Format("X2", mac[0])
                   + string.Format("X2", mac[1])
                   + string.Format("X2", mac[2])
                   + string.Format("X2", mac[3])
                   + string.Format("X2", mac[4])
                   + string.Format("X2", mac[5]);
        }

        private byte[] MacFromString(string macStr)
        {
            var macTokens = new List<string>();
            if (macStr.Length == 12)
            {
                for (var i = 0; i < 6; i++)
                {
                    macTokens.Add(macStr.Substring(i * 2, 2));
                }
            }
            else if (macStr.Contains(':'))
            {
                macTokens.AddRange(macStr.Split(new[] { ':' }, StringSplitOptions.RemoveEmptyEntries));
            }
            else if (macStr.Contains('.'))
            {
                macTokens.AddRange(macStr.Split(new[] { '.' }, StringSplitOptions.RemoveEmptyEntries));
            }
            else if (macStr.Contains(' '))
            {
                macTokens.AddRange(macStr.Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries));
            }

            var mac = new byte[] { 0, 0, 0, 0, 0, 0 };
            if (macTokens.Count == 6)
            {

                for (var i = 0; i < macTokens.Count; i++)
                {
                    if (macTokens[i].Length > 2
                        && byte.TryParse(macTokens[i],
                            NumberStyles.HexNumber,
                            CultureInfo.InvariantCulture,
                            out mac[i]))
                    {
                        return new byte[6] { 0, 0, 0, 0, 0, 0 };
                    }
                }
            }

            return mac;
        }

        #endregion

        #region Chart plot

        private void InitChart()
        {
            chart1.Titles.Clear();
            chart1.Titles.Add("");
            chart1.Titles[0].Alignment = ContentAlignment.TopRight;
            chart1.ChartAreas[0].AxisX.Title = "Time";
            chart1.ChartAreas[0].AxisX.TitleAlignment = StringAlignment.Center;
            chart1.ChartAreas[0].AxisX.Interval = 1;
            chart1.ChartAreas[0].AxisX.IntervalOffset = 1;
            chart1.ChartAreas[0].AxisX.IntervalType = DateTimeIntervalType.Seconds;
            chart1.ChartAreas[0].AxisX.LabelStyle.Format = "HH:mm:ss";
            chart1.ChartAreas[0].AxisY.Title = "Value";
            chart1.ChartAreas[0].AxisY.TitleAlignment = StringAlignment.Center;

            chart1.Series.Clear();
            checkedListBox_params.Items.Clear();
            _plotList.Clear();
        }

        private void InitLog()
        {
            _initTimeSet = false;
            _valueRangeSet = false;
        }

        // extract string processing to ConvertStringsToSensorData()
        private void UpdateChart(IDictionary<string, string> stringsSet, DateTime recordTime, bool saveToDb)
        {
            var currentResult = new LiteDbLocal.SensorDataRec();
            if (stringsSet.TryGetValue(DeviceName, out var devName))
            {
                stringsSet.Remove(DeviceName);

                if (!_initTimeSet)
                {
                    _maxAllowedTime = recordTime;
                    _minAllowedTime = recordTime.AddMilliseconds(-1);
                    _initTimeSet = true;
                    textBox_fromTime.Text =
                        _minAllowedTime.ToShortDateString() + " " + _minAllowedTime.ToLongTimeString();
                }
                else if (recordTime > _maxAllowedTime)
                {
                    _maxAllowedTime = recordTime;
                }
                else if (recordTime < _minAllowedTime)
                {
                    _minAllowedTime = recordTime;
                }

                if (currentResult.ValueList == null)
                    currentResult.ValueList = new List<LiteDbLocal.ValueItemRec>();

                currentResult.DeviceName = devName;
                currentResult.Time = recordTime;

                if (trackBar_max.Value == trackBar_max.Maximum)
                {
                    _maxShowTime = _maxAllowedTime;
                    textBox_toTime.Text =
                        _maxShowTime.ToShortDateString() + " " + _maxShowTime.ToLongTimeString();
                }
            }
            else if (stringsSet.TryGetValue(DeviceMac, out var devMac))
            {
                stringsSet.Remove(DeviceMac);

                if (!_initTimeSet)
                {
                    _maxAllowedTime = recordTime;
                    _minAllowedTime = recordTime.AddMilliseconds(-1);
                    _initTimeSet = true;
                    textBox_fromTime.Text =
                        _minAllowedTime.ToShortDateString() + " " + _minAllowedTime.ToLongTimeString();
                }
                else if (recordTime > _maxAllowedTime)
                {
                    _maxAllowedTime = recordTime;
                }
                else if (recordTime < _minAllowedTime)
                {
                    _minAllowedTime = recordTime;
                }

                if (!currentResult.ValueList.Any())
                {
                    currentResult.ValueList = new List<LiteDbLocal.ValueItemRec>();
                }

                currentResult.DeviceMAC = MacFromString(devMac);
                currentResult.Time = recordTime;

                if (trackBar_max.Value == trackBar_max.Maximum)
                {
                    _maxShowTime = _maxAllowedTime;
                    textBox_toTime.Text =
                        _maxShowTime.ToShortDateString() + " " + _maxShowTime.ToLongTimeString();
                }
            }
            else
            {
                return;
            }

            if (stringsSet.TryGetValue(FwVersion, out var fwVer))
            {
                stringsSet.Remove(FwVersion);
                currentResult.FwVersion = fwVer;
            }

            foreach (var item in stringsSet)
            {
                var newValue = new LiteDbLocal.ValueItemRec();
                var v = item.Value.Replace(".", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
                v = v.Replace(",", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
                if (!float.TryParse(v, out var value)) continue;

                //add result to log
                newValue.Value = value;
                newValue.ValueType = item.Key;
                currentResult.ValueList.Add(newValue);

                var fullValueType = "[" + currentResult.DeviceName + "]" + item.Key;
                //add new plot if not yet in collection
                if (!_plotList.Contains(fullValueType))
                    AddNewPlot(fullValueType, value);

                if (currentResult.Time >= _minShowTime && currentResult.Time <= _maxShowTime)
                    AddNewPoint(fullValueType, currentResult.Time, value);
            }

            if (saveToDb)
            {
                DbOpen();
                currentResult.Id = _recordsDb.AddRecord(currentResult);
            }

            _minAllowedTime = DateTime.Now.Subtract(new TimeSpan(0, _keepTime, 0));
            foreach (var item in chart1.Series)
            {
                item.Sort(PointSortOrder.Ascending, "X");
                var points = item.Points;
                if (points != null && points.Count > 0)
                {
                    while (points.Count > 0 && points[0].XValue < _minAllowedTime.ToOADate()) points.RemoveAt(0);
                    TrackBar_min_Scroll(this, EventArgs.Empty);
                }
            }

            chart1.Invalidate();
        }

        private void UpdateChart(LiteDbLocal.SensorDataRec newData, bool saveToDb)
        {
            if (newData.ValueList.Count <= 0) return;

            if (!_initTimeSet)
            {
                _maxAllowedTime = newData.Time;
                _minAllowedTime = _maxAllowedTime.AddMilliseconds(-1);
                _initTimeSet = true;
                textBox_fromTime.Text =
                    _minAllowedTime.ToShortDateString() + " " + _minAllowedTime.ToLongTimeString();
            }
            else if (newData.Time > _maxAllowedTime)
            {
                _maxAllowedTime = newData.Time;
            }
            else if (newData.Time < _minAllowedTime)
            {
                _minAllowedTime = newData.Time;
            }

            if (trackBar_max.Value == trackBar_max.Maximum)
            {
                _maxShowTime = _maxAllowedTime;
                textBox_toTime.Text =
                    _maxShowTime.ToShortDateString() + " " + _maxShowTime.ToLongTimeString();
            }

            foreach (var item in newData.ValueList)
            {
                var valType = item.ValueType;
                if (!string.IsNullOrEmpty(valType))
                {
                    var fullValueType = "[" + newData.DeviceName + "]" + valType;
                    //add new plot if not yet in collection
                    var val = item.Value;
                    if (!_plotList.Contains(fullValueType))
                        AddNewPlot(fullValueType, val);

                    var newTime = newData.Time;
                    if (newTime >= _minShowTime && newTime <= _maxShowTime)
                        AddNewPoint(fullValueType, newTime, val);
                }
            }

            if (saveToDb)
            {
                DbOpen();
                newData.Id = _recordsDb.AddRecord(newData);
            }

            _minAllowedTime = DateTime.Now.Subtract(new TimeSpan(0, _keepTime, 0));
            foreach (var item in chart1.Series)
            {
                item.Sort(PointSortOrder.Ascending, "X");
                var points = item.Points;
                if (points != null && points.Count > 0)
                {
                    while (points.Count > 0 && points[0].XValue < _minAllowedTime.ToOADate()) points.RemoveAt(0);
                    TrackBar_min_Scroll(this, EventArgs.Empty);
                }
            }

            chart1.Invalidate();
        }

        private void AddNewPlot(string valueType, float value, bool active = false)
        {
            if (!checkedListBox_params.Items.Contains(valueType))
            {
                checkedListBox_params.Items.Add(valueType);
                checkedListBox_params.SetItemChecked(checkedListBox_params.Items.Count - 1, active);
                ReCalculateCheckBoxSize();
            }

            if (chart1.Series.IsUniqueName(valueType))
            {
                Color plotColor;
                if (checkedListBox_params.Items.Count < _currentColor.Length)
                    plotColor = _currentColor[checkedListBox_params.Items.Count - 1];
                else
                    plotColor = Color.FromArgb(0xff, (byte)(checkedListBox_params.Items.Count * 15),
                        (byte)(checkedListBox_params.Items.Count * 25),
                        (byte)(checkedListBox_params.Items.Count * 35));

                var series1 = new Series
                {
                    Name = valueType,
                    IsVisibleInLegend = true,
                    Enabled = active,
                    ChartType = SeriesChartType.Line,
                    XValueType = ChartValueType.DateTime,
                    Color = plotColor,
                    MarkerStyle = MarkerStyle.None
                };

                chart1.Series.Add(series1);
            }

            if (!_plotList.Contains(valueType))
            {
                _plotList.Add(valueType, active);
                _maxShowValue = _minShowValue + 0.01f;
                _minShowValue = value;
                _plotList.SetValueRange(valueType, new MinMaxValue { MinValue = value, MaxValue = value });
            }
        }

        private void RemovePlot(string item)
        {
            var itemNum = _plotList.ItemNumber(item);

            if (itemNum >= 0)
            {
                chart1.Series.RemoveAt(itemNum);
                _plotList.Remove(itemNum);
                checkedListBox_params.Items.RemoveAt(itemNum);
                ReCalculateCheckBoxSize();
            }
        }

        private void AddNewPoint(string valueType, DateTime yValue, float xValue)
        {
            if (!_valueRangeSet)
            {
                _maxShowValue = xValue;
                _minShowValue = _maxShowValue;
                _plotList.SetValueRange(valueType, new MinMaxValue { MinValue = xValue, MaxValue = xValue });
                _valueRangeSet = true;
            }
            else if (xValue > _plotList.GetValueRange(valueType).MaxValue)
            {
                _plotList.SetValueRange(valueType,
                    new MinMaxValue { MinValue = _plotList.GetValueRange(valueType).MinValue, MaxValue = xValue });
            }
            else if (xValue < _plotList.GetValueRange(valueType).MinValue)
            {
                _plotList.SetValueRange(valueType,
                    new MinMaxValue { MinValue = xValue, MaxValue = _plotList.GetValueRange(valueType).MaxValue });
            }

            // Microsoft chart component breaks on very large/small values
            double filteredXvalue;
            if (xValue < _minChartValue)
                filteredXvalue = _minChartValue;
            else if (xValue > _maxChartValue)
                filteredXvalue = _maxChartValue;
            else
                filteredXvalue = xValue;

            chart1.Series[valueType].Points.AddXY(yValue, filteredXvalue);

            if (trackBar_max.Value == trackBar_max.Maximum)
            {
                _maxShowTime = _maxAllowedTime;
                textBox_toTime.Text =
                    _maxShowTime.ToShortDateString() + " " + _maxShowTime.ToLongTimeString();
            }
        }

        #endregion

        #region GUI

        public Form1()
        {
            InitializeComponent();
            notifyIcon1.Visible = false;
            notifyIcon1.MouseDoubleClick += NotifyIcon1_MouseDoubleClick;
            Resize += Form1_Resize;
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            _ipAddress = Settings.Default.DefaultAddress;
            _ipPortMqtt = Settings.Default.DefaultPort;
            textBox_mqttServer.Text = _ipAddress + ":" + _ipPortMqtt;
            _ipPortRest = Settings.Default.RESTPort;
            textBox_restPort.Text = _ipPortRest.ToString();
            textBox_login.Text = _login = Settings.Default.DefaultLogin;
            textBox_password.Text = _password = Settings.Default.DefaultPassword;
            textBox_publishTopic.Text = _publishTopic = Settings.Default.DefaultPublishTopic;
            textBox_subscribeTopic.Text = _subscribeTopic = Settings.Default.DefaultSubscribeTopic;
            checkBox_autoConnect.Checked = _autoConnect = Settings.Default.AutoConnect;
            checkBox_autoReConnect.Checked = _autoReConnect = Settings.Default.AutoReConnect;
            textBox1.Text = _clientId = Settings.Default.ClientID;
            checkBox_addTimeStamp.Checked = Settings.Default.AddTimeStamp;
            _keepTime = Settings.Default.KeepTime;
            textBox_keepTime.Text = _keepTime.ToString();
            _keepLocalDb = Settings.Default.keepLocalDb;

            _logger = new TextLogger.TextLogger(this)
            {
                Channels = _directions,
                FilterZeroChar = false
            };
            textBox_dataLog.DataBindings.Add("Text", _logger, "Text", false, DataSourceUpdateMode.OnPropertyChanged);

            _logger.LineTimeLimit = 100;
            _logger.LineLimit = 500;

            _logger.DefaultTextFormat = checkBox_hex.Checked
                ? TextLogger.TextLogger.TextFormat.Hex
                : TextLogger.TextLogger.TextFormat.AutoReplaceHex;

            _logger.DefaultTimeFormat = TextLogger.TextLogger.TimeFormat.LongTime;

            _logger.DefaultDateFormat = TextLogger.TextLogger.DateFormat.ShortDate;

            _logger.AutoScroll = checkBox_autoScroll.Checked;

            CheckBox_autoScroll_CheckedChanged(null, EventArgs.Empty);

            _plotList.Clear();
            InitChart();
            checkedListBox_params.Items.Clear();
            InitLog();

            // restore latest charts from database
            if (_keepLocalDb)
            {
                DbOpen();
                var restoredRange =
                    _recordsDb.GetRecordsRange(DateTime.Now.Subtract(new TimeSpan(0, _keepTime, 0)), DateTime.Now);
                if (restoredRange != null)
                    foreach (var record in restoredRange)
                        UpdateChart(record, false);
            }

            if (_autoConnect) Button_connect_Click(this, EventArgs.Empty);
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            var window = MessageBox.Show(
                "Close the window?",
                "Are you sure?",
                MessageBoxButtons.YesNo);
            if (window == DialogResult.No)
            {
                e.Cancel = true;
                return;
            }

            _forcedDisconnect = true;
            _mqttClient.ApplicationMessageReceivedHandler = null;
            _mqttClient.DisconnectedHandler = null;
            if (_mqttClient.IsConnected) _mqttClient.DisconnectAsync().ConfigureAwait(true);
            _mqttClient.Dispose();

            if (_keepLocalDb) _recordsDb?.Dispose();

            Settings.Default.DefaultAddress = _ipAddress;
            Settings.Default.DefaultPort = _ipPortMqtt;
            Settings.Default.RESTPort = _ipPortRest;
            Settings.Default.DefaultLogin = _login;
            Settings.Default.DefaultPassword = _password;
            Settings.Default.AutoConnect = _autoConnect;
            Settings.Default.AutoReConnect = _autoReConnect;
            Settings.Default.DefaultPublishTopic = _publishTopic;
            Settings.Default.DefaultSubscribeTopic = _subscribeTopic;
            Settings.Default.ClientID = _clientId;
            Settings.Default.AddTimeStamp = checkBox_addTimeStamp.Checked;
            Settings.Default.KeepTime = _keepTime;
            Settings.Default.Save();
        }

        private void CheckBox_hex_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_hex.Checked)
            {
                textBox_message.Text = Accessory.ConvertStringToHex(textBox_message.Text);
            }
            else
            {
                textBox_message.Text = Accessory.CheckHexString(textBox_message.Text);
                textBox_message.Text = Accessory.ConvertHexToString(textBox_message.Text);
            }
        }

        private void Button_clearLog_Click(object sender, EventArgs e)
        {
            textBox_dataLog.Clear();
            checkedListBox_params.Items.Clear();
            _plotList.Clear();
            InitChart();
            InitLog();
        }

        private void TextBox_message_Leave(object sender, EventArgs e)
        {
            if (checkBox_hex.Checked) textBox_message.Text = Accessory.CheckHexString(textBox_message.Text);
        }

        private void TextBox_maxValue_Leave(object sender, EventArgs e)
        {
            textBox_maxValue.Text =
                textBox_maxValue.Text.Replace(".", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
            textBox_maxValue.Text =
                textBox_maxValue.Text.Replace(",", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
            if (!float.TryParse(textBox_maxValue.Text, out var maxY)) maxY = _maxShowValue;

            textBox_minValue.Text =
                textBox_minValue.Text.Replace(".", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
            textBox_minValue.Text =
                textBox_minValue.Text.Replace(",", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
            if (!float.TryParse(textBox_minValue.Text, out var minY)) minY = _minShowValue;

            if (maxY <= minY) maxY = minY + 1;

            textBox_maxValue.Text = maxY.ToString(CultureInfo.InvariantCulture);
            chart1.ChartAreas[0].AxisY.Maximum = maxY > _maxChartValue ? _maxChartValue : maxY;
            SetCursorInterval();
        }

        private void TextBox_minValue_Leave(object sender, EventArgs e)
        {
            textBox_minValue.Text =
                textBox_minValue.Text.Replace(".", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
            textBox_minValue.Text =
                textBox_minValue.Text.Replace(",", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
            if (!float.TryParse(textBox_minValue.Text, out var minY)) minY = _minShowValue;

            textBox_maxValue.Text =
                textBox_maxValue.Text.Replace(".", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
            textBox_maxValue.Text =
                textBox_maxValue.Text.Replace(",", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
            if (!float.TryParse(textBox_maxValue.Text, out var maxY)) maxY = _maxShowValue;

            if (minY >= maxY) minY = maxY - 1;

            textBox_minValue.Text = minY.ToString(CultureInfo.InvariantCulture);
            chart1.ChartAreas[0].AxisY.Minimum = minY < _minChartValue ? _minChartValue : minY;
            SetCursorInterval();
        }

        private void TrackBar_min_Scroll(object sender, EventArgs e)
        {
            if (trackBar_min.Value == trackBar_min.Minimum)
            {
                _minShowTime = _minAllowedTime;
            }
            else if (trackBar_min.Value == trackBar_min.Maximum)
            {
                trackBar_min.Value -= 1;
                _minShowTime = _maxAllowedTime.AddMilliseconds(-1);
            }
            else
            {
                _minShowTime = _minAllowedTime.AddSeconds((_maxAllowedTime - _minAllowedTime).TotalSeconds *
                    trackBar_min.Value / trackBar_min.Maximum);
                if (_minShowTime >= _maxShowTime)
                {
                    _maxShowTime = _minShowTime.AddMilliseconds(1);
                    trackBar_max.Value = trackBar_min.Value + 1;
                    TrackBar_max_Scroll(this, EventArgs.Empty);
                }
            }

            textBox_fromTime.Text = _minShowTime.ToShortDateString() + " " + _minShowTime.ToLongTimeString();
            chart1.ChartAreas[0].AxisX.Minimum = _minShowTime.ToOADate();
            SetCursorInterval();
            chart1.Invalidate();
        }

        private void TrackBar_max_Scroll(object sender, EventArgs e)
        {
            if (trackBar_max.Value == trackBar_max.Minimum)
            {
                trackBar_max.Value += 1;
                _maxShowTime = _minAllowedTime.AddMilliseconds(1);
                chart1.ChartAreas[0].AxisX.Maximum = _maxShowTime.ToOADate();
            }
            else if (trackBar_max.Value == trackBar_max.Maximum)
            {
                _maxShowTime = _maxAllowedTime;
                chart1.ChartAreas[0].AxisX.Maximum = double.NaN;
            }
            else
            {
                _maxShowTime = _minAllowedTime.AddSeconds((_maxAllowedTime - _minAllowedTime).TotalSeconds *
                    trackBar_max.Value / trackBar_max.Maximum);
                if (_maxShowTime <= _minShowTime)
                {
                    _minShowTime = _maxShowTime.AddMilliseconds(-1);
                    trackBar_min.Value = trackBar_max.Value - 1;
                    TrackBar_min_Scroll(this, EventArgs.Empty);
                }

                chart1.ChartAreas[0].AxisX.Maximum = _maxShowTime.ToOADate();
            }

            textBox_toTime.Text = _maxShowTime.ToShortDateString() + " " + _maxShowTime.ToLongTimeString();
            SetCursorInterval();
            chart1.Invalidate();
        }

        private void Button_saveLog_Click(object sender, EventArgs e)
        {
            saveFileDialog1.Title = "Save data as JSON...";
            saveFileDialog1.DefaultExt = "json";
            saveFileDialog1.Filter = "JSON files|*.json|All files|*.*";
            saveFileDialog1.FileName = "data_" + DateTime.Today.ToShortDateString().Replace("/", "_") + ".json";
            saveFileDialog1.ShowDialog();
        }

        private async void SaveFileDialog1_FileOk(object sender, CancelEventArgs e)
        {
            var startTime = DateTime.Now;
            var endTime = DateTime.Now;
            // get them from sliders
            foreach (var item in chart1.Series)
            {
                var points = item.Points;
                if (points.Count > 0)
                {
                    var tmpTime = DateTime.FromOADate(points.First().XValue);
                    if (tmpTime < startTime) startTime = tmpTime;

                    tmpTime = DateTime.FromOADate(points.Last().XValue);
                    if (tmpTime > endTime) endTime = tmpTime;
                }
            }

            if (endTime <= startTime) return;

            var restoredRange = new List<LiteDbLocal.SensorDataRec>();

            if (_keepLocalDb)
            {
                DbOpen();
                restoredRange = _recordsDb.GetRecordsRange(startTime, endTime);
            }
            else
            {
                var baseUrl = "http://" + _ipAddress + ":" + _ipPortRest;
                var servicePath = "/api/Sensors/GetRecords?startTime=" + startTime.ToString("yyyy-MM-ddTHH:mm:ss") +
                                  "&endTime=" + endTime.ToString("yyyy-MM-ddTHH:mm:ss");

                var credentials = new NetworkCredential(_login, _password);

                using (var client = new HttpClient(new HttpClientHandler { Credentials = credentials }))
                {
                    var resultContentString = "";
                    client.Timeout = TimeSpan.FromSeconds(30);
                    client.BaseAddress = new Uri(baseUrl);

                    var retryCount = 5;
                    var reqResult = false;
                    while (retryCount > 0 && !reqResult)
                        try
                        {
                            var result = await client.GetAsync(servicePath).ConfigureAwait(true);

                            if (!result.IsSuccessStatusCode)
                            {
                                retryCount--;
                                await Task.Delay(100);
                                continue;
                            }

                            resultContentString = await result.Content.ReadAsStringAsync().ConfigureAwait(true);
                            reqResult = true;
                        }
                        catch (Exception exception)
                        {
                            _logger.AddText("REST error: " + exception + Environment.NewLine, (byte)DataDirection.Error, DateTime.Now);
                        }

                    try
                    {
                        restoredRange = ImportJsonString(resultContentString);
                    }
                    catch (Exception ex)
                    {
                        _logger.AddText("Error parsing data from url " + baseUrl + servicePath + ": " + ex.Message + Environment.NewLine,
                            (byte)DataDirection.Error, DateTime.Now);
                    }
                }
            }

            if (restoredRange == null) return;

            try
            {
                var jsonData = ExportJsonFile(restoredRange, saveFileDialog1.FileName, _saveItem);
            }
            catch (Exception ex)
            {
                _logger.AddText("Error writing to file " + saveFileDialog1.FileName + ": " + ex.Message + Environment.NewLine,
                    (byte)DataDirection.Error, DateTime.Now);
            }
        }

        private void Button_loadLog_Click(object sender, EventArgs e)
        {
            openFileDialog1.FileName = string.Empty;
            openFileDialog1.Title = "Open JSON table";
            openFileDialog1.DefaultExt = "json";
            openFileDialog1.Filter = "JSON files|*.json|All files|*.*";
            openFileDialog1.ShowDialog();
        }

        private void CheckedListBox_params_MouseUp(object sender, MouseEventArgs e)
        {
            var index = checkedListBox_params.IndexFromPoint(e.Location);
            if (index != ListBox.NoMatches && e.Button == MouseButtons.Right)
            {
                checkedListBox_params.SelectedIndex = index;
                contextMenuStrip_item.Visible = true;
            }
            else
            {
                contextMenuStrip_item.Visible = false;
            }
        }

        private void ToolStripMenuItem_delete_Click(object sender, EventArgs e)
        {
            RemovePlot(checkedListBox_params.SelectedItem.ToString());
        }

        private void CheckedListBox_params_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Delete) RemovePlot(checkedListBox_params.SelectedItem.ToString());
        }

        private void CheckBox_autoRangeValue_CheckStateChanged(object sender, EventArgs e)
        {
            RecheckMinMaxValue();
            if (checkBox_autoRangeValue.CheckState == CheckState.Checked)
            {
                checkBox_autoRangeValue.Text = "AutoRange";
                textBox_maxValue.Enabled = false;
                textBox_minValue.Enabled = false;
                chart1.ChartAreas[0].AxisY.Maximum = double.NaN;
                chart1.ChartAreas[0].AxisY.Minimum = double.NaN;
                chart1.ChartAreas[0].RecalculateAxesScale();
                textBox_maxValue.Text = chart1.ChartAreas[0].AxisY.Maximum.ToString(CultureInfo.InvariantCulture);
                textBox_minValue.Text = chart1.ChartAreas[0].AxisY.Minimum.ToString(CultureInfo.InvariantCulture);
            }
            else if (checkBox_autoRangeValue.CheckState == CheckState.Indeterminate)
            {
                checkBox_autoRangeValue.Text = "Fit all";
                textBox_maxValue.Enabled = false;
                textBox_minValue.Enabled = false;
                textBox_maxValue.Text = _maxShowValue.ToString(CultureInfo.InvariantCulture);
                textBox_minValue.Text = _minShowValue.ToString(CultureInfo.InvariantCulture);
                chart1.ChartAreas[0].AxisY.Maximum = _maxShowValue;
                chart1.ChartAreas[0].AxisY.Minimum = _minShowValue;
                SetCursorInterval();
            }
            else if (checkBox_autoRangeValue.CheckState == CheckState.Unchecked)
            {
                checkBox_autoRangeValue.Text = "FreeZoom";
                if (_maxShowValue <= _minShowValue) _maxShowValue = _minShowValue + 0.0001f;
                if (string.IsNullOrEmpty(textBox_maxValue.Text))
                    textBox_maxValue.Text = _maxShowValue.ToString(CultureInfo.InvariantCulture);
                if (string.IsNullOrEmpty(textBox_minValue.Text))
                    textBox_minValue.Text = _minShowValue.ToString(CultureInfo.InvariantCulture);
                textBox_maxValue.Enabled = true;
                textBox_minValue.Enabled = true;
                TextBox_maxValue_Leave(this, EventArgs.Empty);
                TextBox_minValue_Leave(this, EventArgs.Empty);
                SetCursorInterval();
            }
        }

        private void SaveSelectedToolStripMenuItem_Click(object sender, EventArgs e)
        {
            _saveItem = _plotList.ItemName(checkedListBox_params.SelectedIndex);
            Button_saveLog_Click(this, EventArgs.Empty);
            _saveItem = "";
        }

        private void TextBox_mqttServer_Leave(object sender, EventArgs e)
        {
            ParseIpAddress(textBox_mqttServer.Text);
            textBox_mqttServer.Text = _ipAddress + ":" + _ipPortMqtt;
        }

        private void TextBox_subscribeTopic_Leave(object sender, EventArgs e)
        {
            if (_subscribeTopic == textBox_subscribeTopic.Text) return;

            if (!_mqttClient.IsConnected) return;

            _mqttClient.UnsubscribeAsync(_subscribeTopic).ConfigureAwait(false);
            _subscribeTopic = textBox_subscribeTopic.Text;
            _mqttClient.SubscribeAsync(new MqttTopicFilterBuilder().WithTopic(_subscribeTopic).Build())
                .ConfigureAwait(false);
        }

        private void TextBox_publishTopic_Leave(object sender, EventArgs e)
        {
            _publishTopic = textBox_publishTopic.Text;
        }

        private void CheckBox_autoconnect_CheckedChanged(object sender, EventArgs e)
        {
            _autoConnect = checkBox_autoConnect.Checked;
        }

        private void CheckBox_autoReconnect_CheckedChanged(object sender, EventArgs e)
        {
            _autoReConnect = checkBox_autoReConnect.Checked;
        }

        private void TextBox_login_Leave(object sender, EventArgs e)
        {
            _login = textBox_login.Text;
        }

        private void TextBox_password_Leave(object sender, EventArgs e)
        {
            _password = textBox_password.Text;
        }

        private void TextBox1_Leave(object sender, EventArgs e)
        {
            _clientId = textBox1.Text;
        }

        private void TextBox_maxValue_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter) TextBox_maxValue_Leave(this, EventArgs.Empty);
        }

        private void TextBox_minValue_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter) TextBox_minValue_Leave(this, EventArgs.Empty);
        }

        private void Chart1_PostPaint(object sender, ChartPaintEventArgs e)
        {
            ReCalculateCheckBoxSize();
            chart1.PostPaint -= Chart1_PostPaint;
        }

        private void TabControl1_Selected(object sender, TabControlEventArgs e)
        {
            if (tabControl1.SelectedIndex == 1) chart1.PostPaint += Chart1_PostPaint;
        }

        private void Form1_SizeChanged(object sender, EventArgs e)
        {
            if (tabControl1.SelectedIndex == 1)
            {
                ReCalculateCheckBoxSize();
                SetCursorInterval();
            }
        }

        private void TextBox_keepTime_Leave(object sender, EventArgs e)
        {
            int.TryParse(textBox_keepTime.Text, out var newKeepTime);
            if (newKeepTime == _keepTime || newKeepTime <= 0)
            {
                textBox_keepTime.Text = _keepTime.ToString();
                return;
            }

            _keepTime = newKeepTime;
            textBox_keepTime.Text = _keepTime.ToString();
        }

        private void NotifyIcon1_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            Show();
            notifyIcon1.Visible = false;
            WindowState = FormWindowState.Normal;
        }

        private void Form1_Resize(object sender, EventArgs e)
        {
            if (WindowState == FormWindowState.Minimized)
            {
                Hide();
                notifyIcon1.Visible = true;
            }
        }

        private void TextBox_restPort_Leave(object sender, EventArgs e)
        {
            var oldIpPortRest = _ipPortRest;

            int.TryParse(textBox_restPort.Text, out _ipPortRest);
            if (oldIpPortRest == _ipPortRest) return;
            if (_ipPortRest <= 0) _ipPortRest = 8080;
            textBox_restPort.Text = _ipPortRest.ToString();
        }

        private async void TextBox_keepTime_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                e.SuppressKeyPress = false;
                int.TryParse(textBox_keepTime.Text, out var newKeepTime);
                if (newKeepTime <= 0)
                {
                    textBox_keepTime.Text = _keepTime.ToString();
                    return;
                }

                if (newKeepTime != _keepTime)
                {
                    _keepTime = newKeepTime;
                    textBox_keepTime.Text = _keepTime.ToString();
                }

                textBox_keepTime.Enabled = false;
                var result = await GetDataFromRest(_keepTime).ConfigureAwait(true);

                if (result)
                {
                    var endTime = DateTime.Now;
                    var startTime = endTime.Subtract(new TimeSpan(0, _keepTime, 0));

                    _minShowTime = _minAllowedTime = startTime;
                    trackBar_min.Value = trackBar_min.Minimum;
                    textBox_fromTime.Text = _minShowTime.ToShortDateString() + " " + _minShowTime.ToLongTimeString();
                    chart1.ChartAreas[0].AxisX.Minimum = _minShowTime.ToOADate();

                    _maxShowTime = _maxAllowedTime;
                    trackBar_max.Value = trackBar_max.Maximum;
                    textBox_toTime.Text = _maxShowTime.ToShortDateString() + " " + _maxShowTime.ToLongTimeString();
                    chart1.ChartAreas[0].AxisX.Maximum = double.NaN;
                    chart1.Invalidate();
                    _logger.AddText("Data loaded from server" + Environment.NewLine, (byte)DataDirection.Info, DateTime.Now);
                }

                textBox_keepTime.Enabled = true;

                chart1.Invalidate();
            }
        }

        private void Chart1_MouseMove(object sender, MouseEventArgs e)
        {
            var mousePoint = new Point(e.X, e.Y);
            chart1.ChartAreas[0].CursorY.SetCursorPixelPosition(mousePoint, true);
            chart1.ChartAreas[0].AxisY.Title = "Value = " + chart1.ChartAreas[0].CursorY.Position.ToString("F3");

            chart1.ChartAreas[0].CursorX.SetCursorPixelPosition(mousePoint, true);
            try
            {
                chart1.ChartAreas[0].AxisX.Title = "Time = " +
                                                   DateTime.FromOADate(chart1.ChartAreas[0].CursorX.Position)
                                                       .ToLongTimeString();
            }
            catch
            {
            }
        }

        private void CheckedListBox_params_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            var item = checkedListBox_params.Items[e.Index].ToString();
            var newValue = e.NewValue != CheckState.Unchecked;
            _plotList.SetActive(item, newValue);
            chart1.Series[item].Enabled = newValue;
            CheckBox_autoRangeValue_CheckStateChanged(this, EventArgs.Empty);
        }

        private void CheckedListBox_params_SelectedIndexChanged(object sender, EventArgs e)
        {
            var n = checkedListBox_params.SelectedItem.ToString();
            foreach (var plot in chart1.Series)
                if (n == plot.Name) plot.MarkerStyle = MarkerStyle.Diamond;
                else plot.MarkerStyle = MarkerStyle.None;
        }

        private void CheckBox_autoScroll_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_autoScroll.Checked)
            {
                _logger.AutoScroll = true;
                textBox_dataLog.TextChanged += TextBox_dataLog_TextChanged;
            }
            else
            {
                _logger.AutoScroll = false;
                textBox_dataLog.TextChanged -= TextBox_dataLog_TextChanged;
            }
        }

        private void TextBox_dataLog_TextChanged(object sender, EventArgs e)
        {
            if (checkBox_autoScroll.Checked)
            {
                textBox_dataLog.SelectionStart = textBox_dataLog.Text.Length;
                textBox_dataLog.ScrollToCaret();
            }
        }

        #endregion

    }
}