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
using System.Runtime.Serialization.Json;
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
        private int ipPort = 1883;
        private string login = string.Empty;
        private string password = string.Empty;
        private string subscribeTopic = string.Empty;
        private string publishTopic = string.Empty;
        private bool autoConnect;
        private bool autoReConnect;
        private volatile bool forcedDisconnect;
        private int keepTime = 60;

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

        private struct MinMaxValue
        {
            public float MaxValue;
            public float MinValue;
        }

        private class PlotItems
        {
            private readonly List<string> plots = new List<string>();
            private readonly List<MinMaxValue> valueLimits = new List<MinMaxValue>();
            private readonly List<bool> active = new List<bool>();

            public bool Add(string newPlot, bool activePlot = false)
            {
                if (plots.Contains(newPlot)) return false;

                plots.Add(newPlot);
                valueLimits.Add(new MinMaxValue());
                active.Add(activePlot);
                return true;
            }

            public bool Remove(string item)
            {
                var itemNum = ItemNumber(item);
                if (itemNum < 0) return false;

                plots.RemoveAt(itemNum);
                valueLimits.RemoveAt(itemNum);
                active.RemoveAt(itemNum);
                return true;
            }

            public bool Remove(int item)
            {
                if (item >= plots.Count) return false;

                plots.RemoveAt(item);
                valueLimits.RemoveAt(item);
                active.RemoveAt(item);
                return true;
            }

            public void Clear()
            {
                plots.Clear();
                valueLimits.Clear();
                active.Clear();
            }

            public bool Contains(string plot)
            {
                return plots.Contains(plot);
            }

            // The get accessor returns a value for a given string
            public MinMaxValue this[string plotName]
            {
                get => valueLimits[ItemNumber(plotName)];
                set => valueLimits[ItemNumber(plotName)] = value;
            }

            public MinMaxValue this[int plotNumber]
            {
                get => valueLimits[plotNumber];
                set => valueLimits[plotNumber] = value;
            }

            public int ItemNumber(string item)
            {
                if (!plots.Contains(item)) return -1;

                return plots.FindIndex(x => x == item);
            }

            public string ItemName(int plotNumber)
            {
                return plots[plotNumber];
            }

            public bool SetActive(string item, bool itemActive)
            {
                var itemNumber = ItemNumber(item);
                if (itemNumber < 0) return false;

                active[itemNumber] = itemActive;

                return true;
            }

            public bool GetActive(string item)
            {
                var itemNumber = ItemNumber(item);
                return itemNumber >= 0 && active[ItemNumber(item)];
            }

            public int Count => plots.Count;
        }

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

            if (recordsDb == null || recordsDb.Disposed) recordsDb = new LiteDbLocal(Settings.Default.DbFileName, clientId);

            var options = new MqttClientOptionsBuilder()
                .WithClientId(clientId + (checkBox_addTimeStamp.Enabled ? DateTime.Now.ToString(CultureInfo.InvariantCulture) : ""))
                .WithTcpServer(ipAddress, ipPort)
                .WithCredentials(login, password)
                .WithCleanSession()
                .Build();
            try
            {
                await mqttClient.ConnectAsync(options, CancellationToken.None).ConfigureAwait(false);

                // Subscribe to a topic
                if (subscribeTopic.Length > 0) await mqttClient.SubscribeAsync(new MqttTopicFilterBuilder().WithTopic(subscribeTopic).Build()).ConfigureAwait(false);
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

            recordsDb?.Dispose();

            Msg("Device disconnected ...");
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

            Invoke((MethodInvoker)delegate
            {
                UpdateChart(stringsSet, DateTime.Now);
            });
        }

        private void OpenFileDialog1_FileOk(object sender, System.ComponentModel.CancelEventArgs e)
        {
            var dataCollection = new List<LiteDbLocal.SensorDataRec>();
            try
            {
                dataCollection = ImportJson(openFileDialog1.FileName);
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

        private bool ExportJson(IEnumerable<LiteDbLocal.SensorDataRec> data, string fileName, string item = null)
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

        private List<LiteDbLocal.SensorDataRec> ImportJson(string fileName)
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

        private void ParseIpAddress(string ipText)
        {
            if (string.IsNullOrEmpty(ipText))
            {
                ipAddress = Settings.Default.DefaultAddress;
                ipPort = Settings.Default.DefaultPort;
                return;
            }

            var portPosition = ipText.IndexOf(':');
            if (portPosition > 0)
            {
                ipAddress = ipText.Substring(0, portPosition);
                if (!int.TryParse(ipText.Substring(portPosition + 1), out ipPort)) ipPort = Settings.Default.DefaultPort;
            }
            else if (portPosition == 0)
            {
                ipAddress = Settings.Default.DefaultAddress;
                if (!int.TryParse(ipText.Substring(1), out ipPort)) ipPort = Settings.Default.DefaultPort;
            }
            else
            {
                ipAddress = ipText;
                ipPort = Settings.Default.DefaultPort;
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

        // extract string processing
        private void UpdateChart(IDictionary<string, string> stringsSet, DateTime currentTime, bool saveToDb = true)
        {
            var currentResult = new LiteDbLocal.SensorDataRec();
            if (stringsSet.TryGetValue("DeviceName", out var devName))
            {
                stringsSet.Remove("DeviceName");

                if (!initTimeSet)
                {
                    maxAllowedTime = minAllowedTime = currentTime;
                    initTimeSet = true;
                    textBox_fromTime.Text =
                        minAllowedTime.ToShortDateString() + " " + minAllowedTime.ToLongTimeString();
                }
                else if (currentTime > maxAllowedTime)
                {
                    maxAllowedTime = currentTime;
                }
                else if (currentTime < minAllowedTime)
                {
                    minAllowedTime = currentTime;
                }

                currentResult.ValueList = new List<LiteDbLocal.ValueItemRec>();
                currentResult.DeviceName = devName;
                currentResult.Time = currentTime;

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

            if (saveToDb) currentResult.Id = recordsDb.AddRecord(currentResult);

            foreach (var item in chart1.Series)
                while (item.Points.Count > 0 && item.Points.First().XValue < DateTime.Now.Subtract(new TimeSpan(0, keepTime, 0)).ToOADate()) item.Points.RemoveAt(0);

            chart1.Invalidate();
        }

        private void UpdateChart(LiteDbLocal.SensorDataRec newData, bool saveToDb = true)
        {
            if (newData.ValueList.Count <= 0) return;

            if (!initTimeSet)
            {
                maxAllowedTime = minAllowedTime = newData.Time;
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
                if (!string.IsNullOrEmpty(item.ValueType))
                {
                    var fullValueType = "[" + newData.DeviceName + "]" + item.ValueType;
                    //add new plot if not yet in collection
                    if (!plotList.Contains(item.ValueType)) AddNewPlot(fullValueType, item.Value);

                    if (newData.Time >= minShowTime && newData.Time <= maxShowTime) AddNewPoint(fullValueType, newData.Time, item.Value);
                }

            if (saveToDb) newData.Id = recordsDb.AddRecord(newData);

            foreach (var item in chart1.Series)
            {
                item.Sort(PointSortOrder.Ascending, "X");
                while (item.Points.Count > 0 && item.Points.First().XValue < DateTime.Now.Subtract(new TimeSpan(0, keepTime, 0)).ToOADate()) item.Points.RemoveAt(0);
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
                Color c;
                if (checkedListBox_params.Items.Count < currentColor.Length)
                    c = currentColor[checkedListBox_params.Items.Count - 1];
                else
                    c = Color.FromArgb(0xff, (byte)(checkedListBox_params.Items.Count * 15),
                        (byte)(checkedListBox_params.Items.Count * 25),
                        (byte)(checkedListBox_params.Items.Count * 35));

                var series1 = new Series
                {
                    Name = valueType,
                    IsVisibleInLegend = true,
                    Enabled = active,
                    ChartType = SeriesChartType.Line,
                    XValueType = ChartValueType.DateTime,
                    Color = c
                };

                chart1.Series.Add(series1);

            }
            if (!plotList.Contains(valueType))
            {
                plotList.Add(valueType, active);
                maxShowValue = minShowValue + 0.01f;
                minShowValue = value;
                plotList[valueType] = new MinMaxValue { MinValue = value, MaxValue = value };
            }
        }

        private void AddNewPoint(string valueType, DateTime yValue, float xValue)
        {
            if (!valueRangeSet)
            {
                maxShowValue = xValue;
                minShowValue = maxShowValue;
                plotList[valueType] = new MinMaxValue { MinValue = xValue, MaxValue = xValue };
                valueRangeSet = true;
            }
            else if (xValue > plotList[valueType].MaxValue)
            {
                plotList[valueType] = new MinMaxValue { MinValue = plotList[valueType].MinValue, MaxValue = xValue };
            }
            else if (xValue < plotList[valueType].MinValue)
            {
                plotList[valueType] = new MinMaxValue { MinValue = xValue, MaxValue = plotList[valueType].MaxValue };
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

        private void RemovePlot(string item)
        {
            var itemNum = plotList.ItemNumber(item);

            chart1.Series.RemoveAt(itemNum);
            plotList.Remove(itemNum);
            checkedListBox_params.Items.RemoveAt(itemNum);
            ReCalculateCheckBoxSize();
        }

        private void RecheckMinMaxValue()
        {
            if (checkBox_autoRangeValue.CheckState != CheckState.Checked) return;

            valueRangeSet = false;
            for (var n = 0; n < plotList.Count; n++)
            {
                if (!plotList.GetActive(plotList.ItemName(n))) continue;

                if (!valueRangeSet)
                {
                    maxShowValue = plotList[n].MaxValue;
                    minShowValue = plotList[n].MinValue;
                    valueRangeSet = true;
                }
                else if (plotList[n].MinValue < minShowValue)
                {
                    minShowValue = plotList[n].MinValue;
                }
                else if (plotList[n].MaxValue > maxShowValue)
                {
                    maxShowValue = plotList[n].MaxValue;
                }
            }
            CheckBox_autoRangeValue_CheckStateChanged(this, EventArgs.Empty);
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
            ipPort = Settings.Default.DefaultPort;
            textBox_mqttServer.Text = ipAddress + ":" + ipPort;
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

            plotList.Clear();
            InitChart();
            checkedListBox_params.Items.Clear();
            InitLog();

            // restore latest charts from database
            recordsDb = new LiteDbLocal(Settings.Default.DbFileName, clientId);
            var restoredRange = recordsDb.GetRecordsRange(DateTime.Now.Subtract(new TimeSpan(0, keepTime, 0)), DateTime.Now);
            if (restoredRange != null)
                foreach (var record in restoredRange) UpdateChart(record, false);

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
            if (mqttClient.IsConnected) mqttClient.DisconnectAsync().ConfigureAwait(false);
            mqttClient.Dispose();

            recordsDb?.Dispose();

            Settings.Default.DefaultAddress = ipAddress;
            Settings.Default.DefaultPort = ipPort;
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

        private void CheckedListBox_params_SelectedValueChanged(object sender, EventArgs e)
        {
            for (var i = 0; i < plotList.Count; i++)
            {
                var item = plotList.ItemName(i);
                plotList.SetActive(checkedListBox_params.Items[i].ToString(), checkedListBox_params.GetItemChecked(i));
                chart1.Series[item].Enabled = plotList.GetActive(item);
            }

            RecheckMinMaxValue();
            CheckBox_autoRangeValue_CheckStateChanged(this, EventArgs.Empty);
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
            chart1.ChartAreas[0].AxisY.Maximum = maxY;
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
            chart1.ChartAreas[0].AxisY.Minimum = minY;
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

        private void SaveFileDialog1_FileOk(object sender, System.ComponentModel.CancelEventArgs e)
        {
            var startTime = DateTime.Now;
            var endTime = DateTime.Now;
            foreach (var item in chart1.Series)
                if (item.Points.Count > 0)
                {
                    var tmpTime = DateTime.FromOADate(item.Points.First().XValue);
                    if (tmpTime < startTime) startTime = tmpTime;

                    tmpTime = DateTime.FromOADate(item.Points.Last().XValue);
                    if (tmpTime > endTime) endTime = tmpTime;
                }

            if (endTime <= startTime) return;

            var restoredRange = recordsDb.GetRecordsRange(startTime, endTime);

            if (restoredRange == null) return;

            try
            {
                var jsonData = ExportJson(restoredRange, saveFileDialog1.FileName, saveItem);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error writing to file " + saveFileDialog1.FileName + ": " + ex.Message);
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
            if (checkBox_autoRangeValue.CheckState == CheckState.Checked)
            {
                checkBox_autoRangeValue.Text = "AutoRange";
                textBox_maxValue.Enabled = false;
                textBox_minValue.Enabled = false;
                chart1.ResetAutoValues();
                chart1.ChartAreas[0].AxisY.Maximum = double.NaN;
                chart1.ChartAreas[0].AxisY.Minimum = double.NaN;
                chart1.ChartAreas[0].RecalculateAxesScale();
                textBox_maxValue.Text = chart1.ChartAreas[0].AxisY.Maximum.ToString(CultureInfo.InvariantCulture);
                textBox_minValue.Text = chart1.ChartAreas[0].AxisY.Minimum.ToString(CultureInfo.InvariantCulture);
            }
            else if (checkBox_autoRangeValue.CheckState == CheckState.Indeterminate)
            {
                checkBox_autoRangeValue.Text = "Fit all";
                if (maxShowValue <= minShowValue) maxShowValue = minShowValue + 0.0001f;
                textBox_maxValue.Text = maxShowValue.ToString(CultureInfo.InvariantCulture);
                textBox_minValue.Text = minShowValue.ToString(CultureInfo.InvariantCulture);
                textBox_maxValue.Enabled = false;
                textBox_minValue.Enabled = false;
                chart1.ChartAreas[0].AxisY.Maximum = maxShowValue;
                chart1.ChartAreas[0].AxisY.Minimum = minShowValue;
            }
            else if (checkBox_autoRangeValue.CheckState == CheckState.Unchecked)
            {
                checkBox_autoRangeValue.Text = "FreeZoom";
                if (maxShowValue <= minShowValue) maxShowValue = minShowValue + 0.0001f;
                if (string.IsNullOrEmpty(textBox_maxValue.Text)) textBox_maxValue.Text = maxShowValue.ToString(CultureInfo.InvariantCulture);

                if (string.IsNullOrEmpty(textBox_minValue.Text)) textBox_minValue.Text = minShowValue.ToString(CultureInfo.InvariantCulture);

                textBox_maxValue.Enabled = true;
                textBox_minValue.Enabled = true;
                chart1.ChartAreas[0].AxisY.Maximum = maxShowValue;
                chart1.ChartAreas[0].AxisY.Minimum = minShowValue;
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
            textBox_mqttServer.Text = ipAddress + ":" + ipPort;
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
            if (tabControl1.SelectedIndex == 1) ReCalculateCheckBoxSize();
        }

        private void TextBox_keepTime_Leave(object sender, EventArgs e)
        {
            var oldKeepTime = keepTime;

            int.TryParse(textBox_keepTime.Text, out keepTime);
            if (oldKeepTime == keepTime) return;

            if (keepTime <= 0) keepTime = 1;

            textBox_keepTime.Text = keepTime.ToString();

            var endTime = DateTime.Now;
            var startTime = endTime.Subtract(new TimeSpan(0, keepTime, 0));
            if (endTime > startTime)
            {
                var restoredRange = recordsDb.GetRecordsRange(startTime, endTime);
                if (restoredRange != null)
                    foreach (var record in restoredRange) UpdateChart(record, false);
            }
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
        #endregion
    }
}