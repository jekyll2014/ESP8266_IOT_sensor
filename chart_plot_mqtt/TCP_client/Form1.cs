using ChartPlotMQTT.Properties;

using MQTTnet;
using MQTTnet.Client;
using MQTTnet.Client.Disconnecting;
using MQTTnet.Client.Options;
using MQTTnet.Client.Receiving;
using MQTTnet.Diagnostics;
using MQTTnet.Implementations;

using System;
using System.Collections.Generic;
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

namespace ChartPlotMQTT
{
    public partial class Form1 : Form
    {
        private string ipAddress = string.Empty;
        private int ipPortMQTT = 1883;
        private int ipPortRest = 8080;
        private string login = string.Empty;
        private string password = string.Empty;
        private string subscribeTopic = string.Empty;
        private string publishTopic = string.Empty;
        private bool autoConnect;
        private bool autoReConnect;
        private volatile bool forcedDisconnect;
        private int keepTime = 60;
        private bool keepLocalDb;

        private static readonly IMqttNetLogger Logger = new MqttNetLogger();
        private readonly IMqttClient mqttClient = new MqttClient(new MqttClientAdapterFactory(Logger), Logger);

        private string clientId = "ChartPlotter";
        private bool initTimeSet;
        private bool valueRangeSet;

        private readonly Color[] currentColor =
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

        private float maxShowValue;
        private float minShowValue;

        private DateTime maxAllowedTime;
        private DateTime minAllowedTime;

        private DateTime maxShowTime;
        private DateTime minShowTime;

        private const double ScaleFactor = 10;
        private readonly double minChartValue = Convert.ToDouble(decimal.MinValue) / ScaleFactor;
        private readonly double maxChartValue = Convert.ToDouble(decimal.MaxValue) / ScaleFactor;

        private readonly PlotItems plotList = new PlotItems();

        private const int MaxLogLength = 4096;
        private readonly StringBuilder log = new StringBuilder();

        private string saveItem = "";

        private LiteDbLocal recordsDb;

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
                .WithClientId(clientId + (checkBox_addTimeStamp.Enabled ? DateTime.Now.ToString(CultureInfo.InvariantCulture) : ""))
                .WithTcpServer(ipAddress, ipPortMQTT)
                .WithCredentials(login, password)
                .WithCleanSession()
                .Build();
            try
            {
                await mqttClient.ConnectAsync(options, CancellationToken.None).ConfigureAwait(true);
                Msg("Device connected");
                // Subscribe to a topic
                if (subscribeTopic.Length > 0)
                {
                    await mqttClient.SubscribeAsync(new MqttTopicFilterBuilder().WithTopic(subscribeTopic).Build()).ConfigureAwait(false);
                    Msg("Topic subscribed");
                }
            }
            catch (Exception ex)
            {
                Msg("Exception: " + ex.Message);
                Invoke((MethodInvoker)delegate
                {
                    button_connect.Text = "Connect";
                    button_connect.Enabled = true;
                    Button_disconnect_Click(this, EventArgs.Empty);
                });
                return;
            }

            mqttClient.ApplicationMessageReceivedHandler =
                new MqttApplicationMessageReceivedHandlerDelegate(Mqtt_DataReceived);
            if (!mqttClient.IsConnected) Button_disconnect_Click(this, EventArgs.Empty);

            mqttClient.DisconnectedHandler = new MqttClientDisconnectedHandlerDelegate(MqttDisconnectedHandler);

            forcedDisconnect = false;

            //GetDataFromREST(keepTime);

            Invoke((MethodInvoker)delegate
            {
                button_connect.Text = "Connect";
                button_connect.Enabled = false;
                button_disconnect.Enabled = true;
                button_send.Enabled = true;
            });
        }

        private void MqttDisconnectedHandler(MqttClientDisconnectedEventArgs e)
        {
            if (autoReConnect && !forcedDisconnect)
            {
                Task.Delay(10000);
                Msg("Device disconnected. Reconnecting...");
                Button_connect_Click(this, EventArgs.Empty);
            }
            else
            {
                Button_disconnect_Click(this, EventArgs.Empty);
            }
        }

        private void Button_disconnect_Click(object sender, EventArgs e)
        {
            if (e != EventArgs.Empty) forcedDisconnect = true;

            mqttClient.ApplicationMessageReceivedHandler = null;
            mqttClient.DisconnectedHandler = null;

            try
            {
                mqttClient.DisconnectAsync().ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                Msg("Disconnect error: " + ex);
            }

            if (keepLocalDb) recordsDb?.Dispose();

            Msg("Device disconnected");
            Invoke((MethodInvoker)delegate
           {
               button_connect.Enabled = true;
               button_disconnect.Enabled = false;
               button_send.Enabled = false;
               textBox_mqttServer.Enabled = true;
           });
        }

        private void Button_send_ClickAsync(object sender, EventArgs e)
        {
            if (mqttClient.IsConnected)
            {
                var outStream = new List<byte>();
                outStream.AddRange(checkBox_hex.Checked
                    ? Accessory.ConvertHexToByteArray(textBox_message.Text)
                    : Encoding.ASCII.GetBytes(textBox_message.Text));

                var message = new MqttApplicationMessageBuilder()
                    .WithTopic(publishTopic)
                    .WithPayload(Encoding.UTF8.GetString(outStream.ToArray()))
                    .Build();

                try
                {
                    mqttClient.PublishAsync(message).ConfigureAwait(false);
                }
                catch (Exception ex)
                {
                    Msg("Write error: " + ex);
                }

                string tmpStr;
                if (checkBox_hex.Checked)
                    tmpStr = "[" + publishTopic + "]>> [" + Accessory.ConvertByteArrayToHex(outStream.ToArray()) + "]" + Environment.NewLine;
                else
                    tmpStr = "[" + publishTopic + "]>> " + Encoding.ASCII.GetString(outStream.ToArray()) + Environment.NewLine;

                AddTextToLog(tmpStr);
            }
            else
            {
                Msg("not connected");
                Button_disconnect_Click(this, EventArgs.Empty);

                if (autoReConnect && !forcedDisconnect)
                {
                    Msg("Reconnecting...");
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

            if (checkBox_hex.Checked)
            {
                tmpStr = "[" + e.ApplicationMessage.Topic + "]<< " + Accessory.ConvertStringToHex(tmpStr) + Environment.NewLine;
            }
            else
            {
                tmpStr = "";
                foreach (var s in stringsSet)
                    tmpStr += "[" + e.ApplicationMessage.Topic + "]<< " + s.Key + "=" + s.Value + Environment.NewLine;
            }
            AddTextToLog(tmpStr);

            var recordTime = DateTime.Now;

            foreach (var s in stringsSet)
                if (s.Key == "Time" && DateTime.TryParse(s.Value, out recordTime)) break;

            Invoke((MethodInvoker)delegate
            {
                UpdateChart(stringsSet, recordTime, keepLocalDb);
            });
        }

        private void OpenFileDialog1_FileOk(object sender, System.ComponentModel.CancelEventArgs e)
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
            plotList.Clear();
            InitChart();
            checkBox_autoRangeValue.Checked = true;

            if (trackBar_min.Value != trackBar_min.Minimum) trackBar_min.Value = trackBar_min.Minimum;
            TrackBar_min_Scroll(this, EventArgs.Empty);

            if (trackBar_max.Value != trackBar_max.Maximum) trackBar_max.Value = trackBar_max.Maximum;
            TrackBar_max_Scroll(this, EventArgs.Empty);

            foreach (var record in dataCollection) UpdateChart(record, false);
        }

        private async Task<bool> GetDataFromREST(int time, int retryCount = 3)
        {
            //http://jekyll-server:8080/api/Sensors/GetLatestRecords?minutes=60

            var baseUrl = "http://" + ipAddress + ":" + ipPortRest;
            var servicePath = "/api/Sensors/GetRecords?minutes=" + time;

            var credentials = new NetworkCredential(login, password);

            using (var client = new HttpClient(new HttpClientHandler() { Credentials = credentials, PreAuthenticate = true }))
            {
                client.Timeout = TimeSpan.FromMinutes(1);
                var resultContentString = "";
                client.Timeout = TimeSpan.FromSeconds(30);
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
                            await Task.Delay(100);
                            continue;
                        }

                        resultContentString = await result.Content.ReadAsStringAsync();
                        reqResult = true;
                    }
                    catch (Exception exception)
                    {
                        //MessageBox.Show(exception.ToString(), "REST error");
                        Msg("REST error: " + exception.Message.ToString());
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
                    Msg("Error parsing data from url " + baseUrl + servicePath + ": " + ex.Message);
                    //MessageBox.Show("Error parsing data from url " + baseUrl + servicePath + ": " + ex.Message);
                }
                if (restoredRange != null)
                    foreach (var record in restoredRange)
                    {
                        UpdateChart(record, false);
                    }
            }
            return true;
        }

        #endregion

        #region Helpers

        private void Msg(string message)
        {
            AddTextToLog("!!! " + message + Environment.NewLine);
        }

        private delegate void SetTextCallback1(string text);

        private void AddTextToLog(string text)
        {
            text = Accessory.FilterZeroChar(text);
            if (textBox_dataLog.InvokeRequired)
            {
                SetTextCallback1 d = AddTextToLog;
                BeginInvoke(d, text);
            }
            else
            {
                var pos = textBox_dataLog.SelectionStart;

                if (text.Length + log.Length > MaxLogLength)
                {
                    var toRemove = log.Length + text.Length - MaxLogLength;
                    log.Remove(0, toRemove);
                    pos -= toRemove;
                    if (pos < 0) pos = 0;
                }
                log.Append(text);
                textBox_dataLog.Text = log.ToString();

                if (checkBox_autoScroll.Checked)
                {
                    textBox_dataLog.SelectionStart = textBox_dataLog.Text.Length;
                    textBox_dataLog.ScrollToCaret();
                }
                else
                {
                    textBox_dataLog.SelectionStart = pos;
                    textBox_dataLog.ScrollToCaret();
                }
            }
        }

        private bool ExportJsonFile(IEnumerable<LiteDbLocal.SensorDataRec> data, string fileName, string item = null)
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
                        if (line.ValueType == item) newPack.ValueList.Add(line);

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
                Msg("JSON parse error: " + ex.Message + Environment.NewLine);
                return false;
            }
            return true;
        }

        private List<LiteDbLocal.SensorDataRec> ImportJsonFile(string fileName)
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
                Msg("JSON parse error: " + ex.Message + Environment.NewLine);
                newValues = null;
            }

            return newValues;
        }

        private List<LiteDbLocal.SensorDataRec> ImportJsonString(string data)
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
                Msg("JSON parse error: " + ex.Message + Environment.NewLine);
                newValues = null;
            }

            return newValues;
        }

        private void ParseIpAddress(string ipText)
        {
            if (string.IsNullOrEmpty(ipText))
            {
                ipAddress = Settings.Default.DefaultAddress;
                ipPortMQTT = Settings.Default.DefaultPort;
                return;
            }

            var portPosition = ipText.IndexOf(':');
            if (portPosition > 0)
            {
                ipAddress = ipText.Substring(0, portPosition);
                if (!int.TryParse(ipText.Substring(portPosition + 1), out ipPortMQTT)) ipPortMQTT = Settings.Default.DefaultPort;
            }
            else if (portPosition == 0)
            {
                ipAddress = Settings.Default.DefaultAddress;
                if (!int.TryParse(ipText.Substring(1), out ipPortMQTT)) ipPortMQTT = Settings.Default.DefaultPort;
            }
            else
            {
                ipAddress = ipText;
                ipPortMQTT = Settings.Default.DefaultPort;
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

            valueRangeSet = false;
            for (var n = 0; n < plotList.Count; n++)
            {
                if (!plotList.GetActive(plotList.ItemName(n))) continue;

                if (!valueRangeSet)
                {
                    maxShowValue = plotList.GetValueRange(n).MaxValue;
                    minShowValue = plotList.GetValueRange(n).MinValue;
                    valueRangeSet = true;
                }
                else if (plotList.GetValueRange(n).MinValue < minShowValue)
                {
                    minShowValue = plotList.GetValueRange(n).MinValue;
                }
                else if (plotList.GetValueRange(n).MaxValue > maxShowValue)
                {
                    maxShowValue = plotList.GetValueRange(n).MaxValue;
                }
            }
        }

        private void DbOpen()
        {
            if (!keepLocalDb) return;
            if (recordsDb == null || recordsDb.Disposed)
            {
                if (string.IsNullOrEmpty(Settings.Default.DbPassword))
                    recordsDb = new LiteDbLocal(Settings.Default.DbFileName, clientId);
                else
                {
                    var connString = new LiteDB.ConnectionString
                    {
                        Filename = Settings.Default.DbFileName,
                        Password = Settings.Default.DbPassword
                    };
                    recordsDb = new LiteDbLocal(connString, clientId);
                }
            }
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
            plotList.Clear();
        }

        private void InitLog()
        {
            initTimeSet = false;
            valueRangeSet = false;
        }

        // extract string processing to ConvertStringsToSensorData()
        private void UpdateChart(IDictionary<string, string> stringsSet, DateTime recordTime, bool saveToDb)
        {
            var currentResult = new LiteDbLocal.SensorDataRec();
            if (stringsSet.TryGetValue("DeviceName", out var devName))
            {
                stringsSet.Remove("DeviceName");

                if (!initTimeSet)
                {
                    maxAllowedTime = recordTime;
                    minAllowedTime = recordTime.AddMilliseconds(-1);
                    initTimeSet = true;
                    textBox_fromTime.Text =
                        minAllowedTime.ToShortDateString() + " " + minAllowedTime.ToLongTimeString();
                }
                else if (recordTime > maxAllowedTime)
                {
                    maxAllowedTime = recordTime;
                }
                else if (recordTime < minAllowedTime)
                {
                    minAllowedTime = recordTime;
                }

                currentResult.ValueList = new List<LiteDbLocal.ValueItemRec>();
                currentResult.DeviceName = devName;
                currentResult.Time = recordTime;

                if (trackBar_max.Value == trackBar_max.Maximum)
                {
                    maxShowTime = maxAllowedTime;
                    textBox_toTime.Text =
                        maxShowTime.ToShortDateString() + " " + maxShowTime.ToLongTimeString();
                }
            }
            else
            {
                return;
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
                if (!plotList.Contains(fullValueType)) AddNewPlot(fullValueType, value);

                if (currentResult.Time >= minShowTime && currentResult.Time <= maxShowTime) AddNewPoint(fullValueType, currentResult.Time, value);
            }

            if (saveToDb)
            {
                DbOpen();
                currentResult.Id = recordsDb.AddRecord(currentResult);
            }

            minAllowedTime = DateTime.Now.Subtract(new TimeSpan(0, keepTime, 0));
            foreach (var item in chart1.Series)
            {
                item.Sort(PointSortOrder.Ascending, "X");
                var points = item.Points;
                if (points != null && points.Count > 0)
                {
                    while (points.Count > 0 && points[0].XValue < minAllowedTime.ToOADate()) points.RemoveAt(0);
                    TrackBar_min_Scroll(this, EventArgs.Empty);
                }
            }

            chart1.Invalidate();
        }

        private void UpdateChart(LiteDbLocal.SensorDataRec newData, bool saveToDb)
        {
            if (newData.ValueList.Count <= 0) return;

            if (!initTimeSet)
            {
                maxAllowedTime = newData.Time;
                minAllowedTime = maxAllowedTime.AddMilliseconds(-1);
                initTimeSet = true;
                textBox_fromTime.Text =
                    minAllowedTime.ToShortDateString() + " " + minAllowedTime.ToLongTimeString();
            }
            else if (newData.Time > maxAllowedTime)
            {
                maxAllowedTime = newData.Time;
            }
            else if (newData.Time < minAllowedTime)
            {
                minAllowedTime = newData.Time;
            }

            if (trackBar_max.Value == trackBar_max.Maximum)
            {
                maxShowTime = maxAllowedTime;
                textBox_toTime.Text =
                    maxShowTime.ToShortDateString() + " " + maxShowTime.ToLongTimeString();
            }

            foreach (var item in newData.ValueList)
            {
                var valType = item.ValueType;
                if (!string.IsNullOrEmpty(valType))
                {
                    var fullValueType = "[" + newData.DeviceName + "]" + valType;
                    //add new plot if not yet in collection
                    var val = item.Value;
                    if (!plotList.Contains(valType)) AddNewPlot(fullValueType, val);

                    var newTime = newData.Time;
                    if (newTime >= minShowTime && newTime <= maxShowTime)
                        AddNewPoint(fullValueType, newTime, val);
                }
            }

            if (saveToDb)
            {
                DbOpen();
                newData.Id = recordsDb.AddRecord(newData);
            }

            minAllowedTime = DateTime.Now.Subtract(new TimeSpan(0, keepTime, 0));
            foreach (var item in chart1.Series)
            {
                item.Sort(PointSortOrder.Ascending, "X");
                var points = item.Points;
                if (points != null && points.Count > 0)
                {
                    while (points.Count > 0 && points[0].XValue < minAllowedTime.ToOADate()) points.RemoveAt(0);
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
                if (checkedListBox_params.Items.Count < currentColor.Length)
                    plotColor = currentColor[checkedListBox_params.Items.Count - 1];
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

            if (!plotList.Contains(valueType))
            {
                plotList.Add(valueType, active);
                maxShowValue = minShowValue + 0.01f;
                minShowValue = value;
                plotList.SetValueRange(valueType, new MinMaxValue { MinValue = value, MaxValue = value });
            }
        }

        private void RemovePlot(string item)
        {
            var itemNum = plotList.ItemNumber(item);

            chart1.Series.RemoveAt(itemNum);
            plotList.Remove(itemNum);
            checkedListBox_params.Items.RemoveAt(itemNum);
            ReCalculateCheckBoxSize();
        }

        private void AddNewPoint(string valueType, DateTime yValue, float xValue)
        {
            if (!valueRangeSet)
            {
                maxShowValue = xValue;
                minShowValue = maxShowValue;
                plotList.SetValueRange(valueType, new MinMaxValue { MinValue = xValue, MaxValue = xValue });
                valueRangeSet = true;
            }
            else if (xValue > plotList.GetValueRange(valueType).MaxValue)
            {
                plotList.SetValueRange(valueType, new MinMaxValue { MinValue = plotList.GetValueRange(valueType).MinValue, MaxValue = xValue });
            }
            else if (xValue < plotList.GetValueRange(valueType).MinValue)
            {
                plotList.SetValueRange(valueType, new MinMaxValue { MinValue = xValue, MaxValue = plotList.GetValueRange(valueType).MaxValue });
            }

            // Microsoft chart component breaks on very large/small values
            double filteredXvalue;
            if (xValue < minChartValue)
                filteredXvalue = minChartValue;
            else if (xValue > maxChartValue)
                filteredXvalue = maxChartValue;
            else
                filteredXvalue = xValue;

            chart1.Series[valueType].Points.AddXY(yValue, filteredXvalue);
        }

        [ReflectionPermission(SecurityAction.Demand, MemberAccess = true)]
        void ResetExceptionState(Control control)
        {
            typeof(Control).InvokeMember("SetState", BindingFlags.NonPublic |
                                                     BindingFlags.InvokeMethod | BindingFlags.Instance, null,
                control, new object[] { 0x400000, false });
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
            ipAddress = Settings.Default.DefaultAddress;
            ipPortMQTT = Settings.Default.DefaultPort;
            textBox_mqttServer.Text = ipAddress + ":" + ipPortMQTT.ToString();
            ipPortRest = Settings.Default.RESTPort;
            textBox_restPort.Text = ipPortRest.ToString();
            textBox_login.Text = login = Settings.Default.DefaultLogin;
            textBox_password.Text = password = Settings.Default.DefaultPassword;
            textBox_publishTopic.Text = publishTopic = Settings.Default.DefaultPublishTopic;
            textBox_subscribeTopic.Text = subscribeTopic = Settings.Default.DefaultSubscribeTopic;
            checkBox_autoConnect.Checked = autoConnect = Settings.Default.AutoConnect;
            checkBox_autoReConnect.Checked = autoReConnect = Settings.Default.AutoReConnect;
            textBox1.Text = clientId = Settings.Default.ClientID;
            checkBox_addTimeStamp.Checked = Settings.Default.AddTimeStamp;
            keepTime = Settings.Default.KeepTime;
            textBox_keepTime.Text = keepTime.ToString();
            keepLocalDb = Settings.Default.keepLocalDb;

            plotList.Clear();
            InitChart();
            checkedListBox_params.Items.Clear();
            InitLog();

            // restore latest charts from database
            if (keepLocalDb)
            {
                DbOpen();
                var restoredRange =
                    recordsDb.GetRecordsRange(DateTime.Now.Subtract(new TimeSpan(0, keepTime, 0)), DateTime.Now);
                if (restoredRange != null)
                    foreach (var record in restoredRange)
                        UpdateChart(record, false);
            }

            if (autoConnect) Button_connect_Click(this, EventArgs.Empty);
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

            forcedDisconnect = true;
            mqttClient.ApplicationMessageReceivedHandler = null;
            mqttClient.DisconnectedHandler = null;
            if (mqttClient.IsConnected) mqttClient.DisconnectAsync().ConfigureAwait(true);
            mqttClient.Dispose();

            if (keepLocalDb) recordsDb?.Dispose();

            Settings.Default.DefaultAddress = ipAddress;
            Settings.Default.DefaultPort = ipPortMQTT;
            Settings.Default.RESTPort = ipPortRest;
            Settings.Default.DefaultLogin = login;
            Settings.Default.DefaultPassword = password;
            Settings.Default.AutoConnect = autoConnect;
            Settings.Default.AutoReConnect = autoReConnect;
            Settings.Default.DefaultPublishTopic = publishTopic;
            Settings.Default.DefaultSubscribeTopic = subscribeTopic;
            Settings.Default.ClientID = clientId;
            Settings.Default.AddTimeStamp = checkBox_addTimeStamp.Checked;
            Settings.Default.KeepTime = keepTime;
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
            plotList.Clear();
            InitChart();
            InitLog();
        }

        private void TextBox_message_Leave(object sender, EventArgs e)
        {
            if (checkBox_hex.Checked) textBox_message.Text = Accessory.CheckHexString(textBox_message.Text);
        }

        private void TextBox_maxValue_Leave(object sender, EventArgs e)
        {
            textBox_maxValue.Text = textBox_maxValue.Text.Replace(".", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
            textBox_maxValue.Text = textBox_maxValue.Text.Replace(",", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
            if (!float.TryParse(textBox_maxValue.Text, out var maxY)) maxY = maxShowValue;

            textBox_minValue.Text = textBox_minValue.Text.Replace(".", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
            textBox_minValue.Text = textBox_minValue.Text.Replace(",", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
            if (!float.TryParse(textBox_minValue.Text, out var minY)) minY = minShowValue;

            if (maxY <= minY) maxY = minY + 1;

            textBox_maxValue.Text = maxY.ToString(CultureInfo.InvariantCulture);
            chart1.ChartAreas[0].AxisY.Maximum = maxY > maxChartValue ? maxChartValue : maxY;
            SetCursorInterval();
        }

        private void TextBox_minValue_Leave(object sender, EventArgs e)
        {
            textBox_minValue.Text = textBox_minValue.Text.Replace(".", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
            textBox_minValue.Text = textBox_minValue.Text.Replace(",", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
            if (!float.TryParse(textBox_minValue.Text, out var minY)) minY = minShowValue;

            textBox_maxValue.Text = textBox_maxValue.Text.Replace(".", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
            textBox_maxValue.Text = textBox_maxValue.Text.Replace(",", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
            if (!float.TryParse(textBox_maxValue.Text, out var maxY)) maxY = maxShowValue;

            if (minY >= maxY) minY = maxY - 1;

            textBox_minValue.Text = minY.ToString(CultureInfo.InvariantCulture);
            chart1.ChartAreas[0].AxisY.Minimum = minY < minChartValue ? minChartValue : minY;
            SetCursorInterval();
        }

        private void TrackBar_min_Scroll(object sender, EventArgs e)
        {
            if (trackBar_min.Value == trackBar_min.Minimum)
            {
                minShowTime = minAllowedTime;
            }
            else if (trackBar_min.Value == trackBar_min.Maximum)
            {
                minShowTime = maxAllowedTime.AddMilliseconds(-1);
            }
            else
            {
                minShowTime = minAllowedTime.AddSeconds((maxAllowedTime - minAllowedTime).TotalSeconds * trackBar_min.Value / trackBar_min.Maximum);
                if (minShowTime >= maxShowTime)
                {
                    maxShowTime = minShowTime.AddMilliseconds(1);
                    trackBar_max.Value = trackBar_min.Value + 1;
                    TrackBar_max_Scroll(this, EventArgs.Empty);
                }
            }
            textBox_fromTime.Text = minShowTime.ToShortDateString() + " " + minShowTime.ToLongTimeString();
            chart1.ChartAreas[0].AxisX.Minimum = minShowTime.ToOADate();
            SetCursorInterval();
            chart1.Invalidate();
        }

        private void TrackBar_max_Scroll(object sender, EventArgs e)
        {
            if (trackBar_max.Value == trackBar_max.Minimum)
            {
                maxShowTime = minAllowedTime.AddMilliseconds(1);
            }
            else if (trackBar_max.Value == trackBar_max.Maximum)
            {
                maxShowTime = maxAllowedTime;
            }
            else
            {
                maxShowTime = minAllowedTime.AddSeconds((maxAllowedTime - minAllowedTime).TotalSeconds * trackBar_max.Value / trackBar_max.Maximum);
                if (maxShowTime <= minShowTime)
                {
                    minShowTime = maxShowTime.AddMilliseconds(-1);
                    trackBar_min.Value = trackBar_max.Value - 1;
                    TrackBar_min_Scroll(this, EventArgs.Empty);
                }
            }
            textBox_toTime.Text = maxShowTime.ToShortDateString() + " " + maxShowTime.ToLongTimeString();
            chart1.ChartAreas[0].AxisX.Maximum = maxShowTime.ToOADate();
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

        private async void SaveFileDialog1_FileOk(object sender, System.ComponentModel.CancelEventArgs e)
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

            if (keepLocalDb)
            {
                DbOpen();
                restoredRange = recordsDb.GetRecordsRange(startTime, endTime);
            }
            else
            {
                var baseUrl = "http://" + ipAddress + ":" + ipPortRest;
                var servicePath = "/api/Sensors/GetRecords?startTime=" + startTime.ToString("yyyy-MM-ddTHH:mm:ss") +
                                  "&endTime=" + endTime.ToString("yyyy-MM-ddTHH:mm:ss");

                var credentials = new NetworkCredential(login, password);

                using (var client = new HttpClient(new HttpClientHandler { Credentials = credentials }))
                {
                    var resultContentString = "";
                    client.Timeout = TimeSpan.FromSeconds(30);
                    client.BaseAddress = new Uri(baseUrl);

                    var retryCount = 5;
                    var reqResult = false;
                    while (retryCount > 0 && !reqResult)
                    {
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
                            Msg("REST error: " + exception.ToString());
                            //MessageBox.Show(exception.ToString(), "REST error");
                        }
                    }

                    try
                    {
                        restoredRange = ImportJsonString(resultContentString);
                    }
                    catch (Exception ex)
                    {
                        Msg("Error parsing data from url " + baseUrl + servicePath + ": " + ex.Message);
                        //MessageBox.Show("Error parsing data from url " + baseUrl + servicePath + ": " + ex.Message);
                    }
                }
            }

            if (restoredRange == null) return;

            try
            {
                var jsonData = ExportJsonFile(restoredRange, saveFileDialog1.FileName, saveItem);
            }
            catch (Exception ex)
            {
                Msg("Error writing to file " + saveFileDialog1.FileName + ": " + ex.Message);
                //MessageBox.Show("Error writing to file " + saveFileDialog1.FileName + ": " + ex.Message);
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
                //chart1.ResetAutoValues();
                chart1.ChartAreas[0].AxisY.Maximum = double.NaN;
                chart1.ChartAreas[0].AxisY.Minimum = double.NaN;
                chart1.ChartAreas[0].RecalculateAxesScale();
                textBox_maxValue.Text = chart1.ChartAreas[0].AxisY.Maximum.ToString(CultureInfo.InvariantCulture);
                textBox_minValue.Text = chart1.ChartAreas[0].AxisY.Minimum.ToString(CultureInfo.InvariantCulture);
                //SetCursorInterval();
            }
            else if (checkBox_autoRangeValue.CheckState == CheckState.Indeterminate)
            {
                checkBox_autoRangeValue.Text = "Fit all";
                textBox_maxValue.Enabled = false;
                textBox_minValue.Enabled = false;
                //if (maxShowValue <= minShowValue) maxShowValue = minShowValue + 0.0001f;
                textBox_maxValue.Text = maxShowValue.ToString(CultureInfo.InvariantCulture);
                textBox_minValue.Text = minShowValue.ToString(CultureInfo.InvariantCulture);
                chart1.ChartAreas[0].AxisY.Maximum = maxShowValue;
                chart1.ChartAreas[0].AxisY.Minimum = minShowValue;
                SetCursorInterval();
            }
            else if (checkBox_autoRangeValue.CheckState == CheckState.Unchecked)
            {
                checkBox_autoRangeValue.Text = "FreeZoom";
                if (maxShowValue <= minShowValue) maxShowValue = minShowValue + 0.0001f;
                if (string.IsNullOrEmpty(textBox_maxValue.Text)) textBox_maxValue.Text = maxShowValue.ToString(CultureInfo.InvariantCulture);
                if (string.IsNullOrEmpty(textBox_minValue.Text)) textBox_minValue.Text = minShowValue.ToString(CultureInfo.InvariantCulture);
                textBox_maxValue.Enabled = true;
                textBox_minValue.Enabled = true;
                TextBox_maxValue_Leave(this, EventArgs.Empty);
                TextBox_minValue_Leave(this, EventArgs.Empty);
                SetCursorInterval();
            }
        }

        private void SaveSelectedToolStripMenuItem_Click(object sender, EventArgs e)
        {
            saveItem = plotList.ItemName(checkedListBox_params.SelectedIndex);
            Button_saveLog_Click(this, EventArgs.Empty);
            saveItem = "";
        }

        private void TextBox_mqttServer_Leave(object sender, EventArgs e)
        {
            ParseIpAddress(textBox_mqttServer.Text);
            textBox_mqttServer.Text = ipAddress + ":" + ipPortMQTT;
        }

        private void TextBox_subscribeTopic_Leave(object sender, EventArgs e)
        {
            if (subscribeTopic == textBox_subscribeTopic.Text) return;

            if (!mqttClient.IsConnected) return;

            mqttClient.UnsubscribeAsync(subscribeTopic).ConfigureAwait(false);
            subscribeTopic = textBox_subscribeTopic.Text;
            mqttClient.SubscribeAsync(new MqttTopicFilterBuilder().WithTopic(subscribeTopic).Build()).ConfigureAwait(false);
        }

        private void TextBox_publishTopic_Leave(object sender, EventArgs e)
        {
            publishTopic = textBox_publishTopic.Text;
        }

        private void CheckBox_autoconnect_CheckedChanged(object sender, EventArgs e)
        {
            autoConnect = checkBox_autoConnect.Checked;
        }

        private void CheckBox_autoReconnect_CheckedChanged(object sender, EventArgs e)
        {
            autoReConnect = checkBox_autoReConnect.Checked;
        }

        private void TextBox_login_Leave(object sender, EventArgs e)
        {
            login = textBox_login.Text;
        }

        private void TextBox_password_Leave(object sender, EventArgs e)
        {
            password = textBox_password.Text;
        }

        private void TextBox1_Leave(object sender, EventArgs e)
        {
            clientId = textBox1.Text;
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
            if (newKeepTime == keepTime || newKeepTime <= 0)
            {
                textBox_keepTime.Text = keepTime.ToString();
                return;
            }

            keepTime = newKeepTime;
            textBox_keepTime.Text = keepTime.ToString();
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

        private void Button_resetChart_Click(object sender, EventArgs e)
        {
            ResetExceptionState(chart1);
        }

        private void TextBox_restPort_Leave(object sender, EventArgs e)
        {
            var oldIpPortRest = ipPortRest;

            int.TryParse(textBox_restPort.Text, out ipPortRest);
            if (oldIpPortRest == ipPortRest) return;
            if (ipPortRest <= 0) ipPortRest = 8080;
            textBox_restPort.Text = ipPortRest.ToString();
        }

        private async void TextBox_keepTime_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                e.SuppressKeyPress = false;
                int.TryParse(textBox_keepTime.Text, out int newKeepTime);
                if (newKeepTime <= 0)
                {
                    textBox_keepTime.Text = keepTime.ToString();
                    return;
                }

                if (newKeepTime != keepTime)
                {
                    keepTime = newKeepTime;
                    textBox_keepTime.Text = keepTime.ToString();
                }

                textBox_keepTime.Enabled = false;
                var result = await GetDataFromREST(keepTime).ConfigureAwait(true);

                if (result)
                {
                    var endTime = DateTime.Now;
                    var startTime = endTime.Subtract(new TimeSpan(0, keepTime, 0));

                    minShowTime = minAllowedTime = startTime;
                    trackBar_min.Value = trackBar_min.Minimum;
                    textBox_fromTime.Text = minShowTime.ToShortDateString() + " " + minShowTime.ToLongTimeString();
                    chart1.ChartAreas[0].AxisX.Minimum = minShowTime.ToOADate();

                    maxShowTime = maxAllowedTime;
                    trackBar_max.Value = trackBar_max.Maximum;
                    textBox_toTime.Text = maxShowTime.ToShortDateString() + " " + maxShowTime.ToLongTimeString();
                    chart1.ChartAreas[0].AxisX.Maximum = maxShowTime.ToOADate();
                    chart1.Invalidate();

                    //plotList.Clear();
                    //InitChart();
                    //checkedListBox_params.Items.Clear();

                    Msg("Data loaded from server");
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
            chart1.ChartAreas[0].AxisX.Title = "Time = " + DateTime.FromOADate(chart1.ChartAreas[0].CursorX.Position).ToLongTimeString();
        }

        private void CheckedListBox_params_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            var item = checkedListBox_params.Items[e.Index].ToString();
            var newValue = e.NewValue != CheckState.Unchecked;
            plotList.SetActive(item, newValue);
            chart1.Series[item].Enabled = newValue;
            CheckBox_autoRangeValue_CheckStateChanged(this, EventArgs.Empty);
        }

        #endregion

        private void checkedListBox_params_SelectedIndexChanged(object sender, EventArgs e)
        {
            var n = checkedListBox_params.SelectedItem.ToString();
            foreach (var plot in chart1.Series)
            {
                if (n == plot.Name) plot.MarkerStyle = MarkerStyle.Diamond;
                else plot.MarkerStyle = MarkerStyle.None;
            }
        }
    }
}