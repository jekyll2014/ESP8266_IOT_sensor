namespace ChartPlotMQTT
{
    partial class Form1
    {
        /// <summary>
        /// Обязательная переменная конструктора.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Освободить все используемые ресурсы.
        /// </summary>
        /// <param name="disposing">истинно, если управляемый ресурс должен быть удален; иначе ложно.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Код, автоматически созданный конструктором форм Windows

        /// <summary>
        /// Требуемый метод для поддержки конструктора — не изменяйте 
        /// содержимое этого метода с помощью редактора кода.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea5 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Legend legend5 = new System.Windows.Forms.DataVisualization.Charting.Legend();
            System.Windows.Forms.DataVisualization.Charting.Series series5 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.Title title5 = new System.Windows.Forms.DataVisualization.Charting.Title();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.textBox_dataLog = new System.Windows.Forms.TextBox();
            this.textBox_message = new System.Windows.Forms.TextBox();
            this.button_send = new System.Windows.Forms.Button();
            this.button_connect = new System.Windows.Forms.Button();
            this.button_disconnect = new System.Windows.Forms.Button();
            this.checkBox_hex = new System.Windows.Forms.CheckBox();
            this.trackBar_max = new System.Windows.Forms.TrackBar();
            this.trackBar_min = new System.Windows.Forms.TrackBar();
            this.label7 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.checkBox_autoRangeValue = new System.Windows.Forms.CheckBox();
            this.label4 = new System.Windows.Forms.Label();
            this.checkedListBox_params = new System.Windows.Forms.CheckedListBox();
            this.contextMenuStrip_item = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.toolStripMenuItem_delete = new System.Windows.Forms.ToolStripMenuItem();
            this.saveSelectedToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.textBox_fromTime = new System.Windows.Forms.TextBox();
            this.textBox_toTime = new System.Windows.Forms.TextBox();
            this.textBox_minValue = new System.Windows.Forms.TextBox();
            this.textBox_maxValue = new System.Windows.Forms.TextBox();
            this.chart1 = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.checkBox_autoScroll = new System.Windows.Forms.CheckBox();
            this.button_clearLog = new System.Windows.Forms.Button();
            this.button_loadLog = new System.Windows.Forms.Button();
            this.button_saveLog = new System.Windows.Forms.Button();
            this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
            this.saveFileDialog1 = new System.Windows.Forms.SaveFileDialog();
            this.checkBox_autoReConnect = new System.Windows.Forms.CheckBox();
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.tabPage_connect = new System.Windows.Forms.TabPage();
            this.label3 = new System.Windows.Forms.Label();
            this.textBox_restPort = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label10 = new System.Windows.Forms.Label();
            this.label9 = new System.Windows.Forms.Label();
            this.label12 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.textBox_mqttServer = new System.Windows.Forms.TextBox();
            this.textBox_publishTopic = new System.Windows.Forms.TextBox();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.textBox_subscribeTopic = new System.Windows.Forms.TextBox();
            this.textBox_password = new System.Windows.Forms.TextBox();
            this.textBox_login = new System.Windows.Forms.TextBox();
            this.checkBox_autoConnect = new System.Windows.Forms.CheckBox();
            this.checkBox_addTimeStamp = new System.Windows.Forms.CheckBox();
            this.tabPage_chart = new System.Windows.Forms.TabPage();
            this.button_resetChart = new System.Windows.Forms.Button();
            this.label14 = new System.Windows.Forms.Label();
            this.label15 = new System.Windows.Forms.Label();
            this.label13 = new System.Windows.Forms.Label();
            this.textBox_keepTime = new System.Windows.Forms.TextBox();
            this.notifyIcon1 = new System.Windows.Forms.NotifyIcon(this.components);
            ((System.ComponentModel.ISupportInitialize)(this.trackBar_max)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar_min)).BeginInit();
            this.contextMenuStrip_item.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.chart1)).BeginInit();
            this.tabControl1.SuspendLayout();
            this.tabPage_connect.SuspendLayout();
            this.tabPage_chart.SuspendLayout();
            this.SuspendLayout();
            // 
            // textBox_dataLog
            // 
            this.textBox_dataLog.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox_dataLog.Location = new System.Drawing.Point(6, 84);
            this.textBox_dataLog.Multiline = true;
            this.textBox_dataLog.Name = "textBox_dataLog";
            this.textBox_dataLog.ReadOnly = true;
            this.textBox_dataLog.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.textBox_dataLog.Size = new System.Drawing.Size(714, 359);
            this.textBox_dataLog.TabIndex = 21;
            // 
            // textBox_message
            // 
            this.textBox_message.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox_message.Location = new System.Drawing.Point(6, 451);
            this.textBox_message.Name = "textBox_message";
            this.textBox_message.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.textBox_message.Size = new System.Drawing.Size(233, 20);
            this.textBox_message.TabIndex = 10;
            this.textBox_message.Leave += new System.EventHandler(this.TextBox_message_Leave);
            // 
            // button_send
            // 
            this.button_send.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.button_send.Enabled = false;
            this.button_send.Location = new System.Drawing.Point(245, 449);
            this.button_send.Name = "button_send";
            this.button_send.Size = new System.Drawing.Size(75, 23);
            this.button_send.TabIndex = 11;
            this.button_send.Text = "Send";
            this.button_send.UseVisualStyleBackColor = true;
            this.button_send.Click += new System.EventHandler(this.Button_send_ClickAsync);
            // 
            // button_connect
            // 
            this.button_connect.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.button_connect.Location = new System.Drawing.Point(562, 30);
            this.button_connect.Name = "button_connect";
            this.button_connect.Size = new System.Drawing.Size(75, 23);
            this.button_connect.TabIndex = 5;
            this.button_connect.Text = "Connect";
            this.button_connect.UseVisualStyleBackColor = true;
            this.button_connect.Click += new System.EventHandler(this.Button_connect_Click);
            // 
            // button_disconnect
            // 
            this.button_disconnect.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.button_disconnect.Enabled = false;
            this.button_disconnect.Location = new System.Drawing.Point(643, 30);
            this.button_disconnect.Name = "button_disconnect";
            this.button_disconnect.Size = new System.Drawing.Size(75, 23);
            this.button_disconnect.TabIndex = 6;
            this.button_disconnect.Text = "Disconnect";
            this.button_disconnect.UseVisualStyleBackColor = true;
            this.button_disconnect.Click += new System.EventHandler(this.Button_disconnect_Click);
            // 
            // checkBox_hex
            // 
            this.checkBox_hex.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.checkBox_hex.AutoSize = true;
            this.checkBox_hex.Location = new System.Drawing.Point(677, 453);
            this.checkBox_hex.Name = "checkBox_hex";
            this.checkBox_hex.Size = new System.Drawing.Size(43, 17);
            this.checkBox_hex.TabIndex = 23;
            this.checkBox_hex.Text = "hex";
            this.checkBox_hex.UseVisualStyleBackColor = true;
            this.checkBox_hex.CheckedChanged += new System.EventHandler(this.CheckBox_hex_CheckedChanged);
            // 
            // trackBar_max
            // 
            this.trackBar_max.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar_max.Location = new System.Drawing.Point(0, 442);
            this.trackBar_max.Maximum = 1000;
            this.trackBar_max.Minimum = 1;
            this.trackBar_max.Name = "trackBar_max";
            this.trackBar_max.Size = new System.Drawing.Size(565, 45);
            this.trackBar_max.TabIndex = 4;
            this.trackBar_max.Value = 1000;
            this.trackBar_max.Scroll += new System.EventHandler(this.TrackBar_max_Scroll);
            // 
            // trackBar_min
            // 
            this.trackBar_min.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar_min.Location = new System.Drawing.Point(0, 407);
            this.trackBar_min.Maximum = 1000;
            this.trackBar_min.Minimum = 1;
            this.trackBar_min.Name = "trackBar_min";
            this.trackBar_min.Size = new System.Drawing.Size(565, 45);
            this.trackBar_min.TabIndex = 3;
            this.trackBar_min.Value = 1;
            this.trackBar_min.Scroll += new System.EventHandler(this.TrackBar_min_Scroll);
            // 
            // label7
            // 
            this.label7.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(572, 428);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(33, 13);
            this.label7.TabIndex = 18;
            this.label7.Text = "From:";
            this.label7.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // label6
            // 
            this.label6.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(582, 454);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(23, 13);
            this.label6.TabIndex = 20;
            this.label6.Text = "To:";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(3, 5);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(37, 13);
            this.label5.TabIndex = 7;
            this.label5.Text = "Min. X";
            // 
            // checkBox_autoRangeValue
            // 
            this.checkBox_autoRangeValue.AutoSize = true;
            this.checkBox_autoRangeValue.Checked = true;
            this.checkBox_autoRangeValue.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox_autoRangeValue.Location = new System.Drawing.Point(204, 4);
            this.checkBox_autoRangeValue.Name = "checkBox_autoRangeValue";
            this.checkBox_autoRangeValue.Size = new System.Drawing.Size(80, 17);
            this.checkBox_autoRangeValue.TabIndex = 8;
            this.checkBox_autoRangeValue.Text = "AutoRange";
            this.checkBox_autoRangeValue.ThreeState = true;
            this.checkBox_autoRangeValue.UseVisualStyleBackColor = true;
            this.checkBox_autoRangeValue.CheckStateChanged += new System.EventHandler(this.CheckBox_autoRangeValue_CheckStateChanged);
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(158, 5);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(40, 13);
            this.label4.TabIndex = 9;
            this.label4.Text = "Max. X";
            // 
            // checkedListBox_params
            // 
            this.checkedListBox_params.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.checkedListBox_params.CheckOnClick = true;
            this.checkedListBox_params.ContextMenuStrip = this.contextMenuStrip_item;
            this.checkedListBox_params.FormattingEnabled = true;
            this.checkedListBox_params.HorizontalScrollbar = true;
            this.checkedListBox_params.Location = new System.Drawing.Point(611, 367);
            this.checkedListBox_params.Name = "checkedListBox_params";
            this.checkedListBox_params.Size = new System.Drawing.Size(115, 49);
            this.checkedListBox_params.TabIndex = 5;
            this.checkedListBox_params.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.CheckedListBox_params_ItemCheck);
            this.checkedListBox_params.SelectedIndexChanged += new System.EventHandler(this.checkedListBox_params_SelectedIndexChanged);
            this.checkedListBox_params.KeyDown += new System.Windows.Forms.KeyEventHandler(this.CheckedListBox_params_KeyDown);
            this.checkedListBox_params.MouseUp += new System.Windows.Forms.MouseEventHandler(this.CheckedListBox_params_MouseUp);
            // 
            // contextMenuStrip_item
            // 
            this.contextMenuStrip_item.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripMenuItem_delete,
            this.saveSelectedToolStripMenuItem});
            this.contextMenuStrip_item.Name = "contextMenuStrip_item";
            this.contextMenuStrip_item.Size = new System.Drawing.Size(145, 48);
            // 
            // toolStripMenuItem_delete
            // 
            this.toolStripMenuItem_delete.Name = "toolStripMenuItem_delete";
            this.toolStripMenuItem_delete.Size = new System.Drawing.Size(144, 22);
            this.toolStripMenuItem_delete.Text = "Delete";
            this.toolStripMenuItem_delete.Click += new System.EventHandler(this.ToolStripMenuItem_delete_Click);
            // 
            // saveSelectedToolStripMenuItem
            // 
            this.saveSelectedToolStripMenuItem.Name = "saveSelectedToolStripMenuItem";
            this.saveSelectedToolStripMenuItem.Size = new System.Drawing.Size(144, 22);
            this.saveSelectedToolStripMenuItem.Text = "Save selected";
            this.saveSelectedToolStripMenuItem.Click += new System.EventHandler(this.SaveSelectedToolStripMenuItem_Click);
            // 
            // textBox_fromTime
            // 
            this.textBox_fromTime.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox_fromTime.Location = new System.Drawing.Point(611, 425);
            this.textBox_fromTime.Name = "textBox_fromTime";
            this.textBox_fromTime.ReadOnly = true;
            this.textBox_fromTime.Size = new System.Drawing.Size(115, 20);
            this.textBox_fromTime.TabIndex = 19;
            this.textBox_fromTime.Text = "0";
            // 
            // textBox_toTime
            // 
            this.textBox_toTime.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox_toTime.Location = new System.Drawing.Point(611, 451);
            this.textBox_toTime.Name = "textBox_toTime";
            this.textBox_toTime.ReadOnly = true;
            this.textBox_toTime.Size = new System.Drawing.Size(115, 20);
            this.textBox_toTime.TabIndex = 21;
            this.textBox_toTime.Text = "0";
            // 
            // textBox_minValue
            // 
            this.textBox_minValue.Enabled = false;
            this.textBox_minValue.Location = new System.Drawing.Point(46, 2);
            this.textBox_minValue.Name = "textBox_minValue";
            this.textBox_minValue.Size = new System.Drawing.Size(50, 20);
            this.textBox_minValue.TabIndex = 0;
            this.textBox_minValue.Text = "0";
            this.textBox_minValue.KeyUp += new System.Windows.Forms.KeyEventHandler(this.TextBox_minValue_KeyUp);
            this.textBox_minValue.Leave += new System.EventHandler(this.TextBox_minValue_Leave);
            // 
            // textBox_maxValue
            // 
            this.textBox_maxValue.Enabled = false;
            this.textBox_maxValue.Location = new System.Drawing.Point(102, 2);
            this.textBox_maxValue.Name = "textBox_maxValue";
            this.textBox_maxValue.Size = new System.Drawing.Size(50, 20);
            this.textBox_maxValue.TabIndex = 1;
            this.textBox_maxValue.Text = "0";
            this.textBox_maxValue.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.textBox_maxValue.KeyUp += new System.Windows.Forms.KeyEventHandler(this.TextBox_maxValue_KeyUp);
            this.textBox_maxValue.Leave += new System.EventHandler(this.TextBox_maxValue_Leave);
            // 
            // chart1
            // 
            this.chart1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            chartArea5.AxisX.Crossing = -1.7976931348623157E+308D;
            chartArea5.AxisX.Interval = 1D;
            chartArea5.AxisX.LabelStyle.Interval = 0D;
            chartArea5.AxisX.LabelStyle.IntervalOffset = 0D;
            chartArea5.AxisX.LabelStyle.IntervalOffsetType = System.Windows.Forms.DataVisualization.Charting.DateTimeIntervalType.Auto;
            chartArea5.AxisX.LabelStyle.IntervalType = System.Windows.Forms.DataVisualization.Charting.DateTimeIntervalType.Auto;
            chartArea5.AxisX.MajorGrid.Interval = 0D;
            chartArea5.AxisX.MajorGrid.IntervalOffset = 0D;
            chartArea5.AxisX.MajorGrid.IntervalOffsetType = System.Windows.Forms.DataVisualization.Charting.DateTimeIntervalType.Auto;
            chartArea5.AxisX.MajorGrid.IntervalType = System.Windows.Forms.DataVisualization.Charting.DateTimeIntervalType.Auto;
            chartArea5.AxisX.MajorTickMark.Interval = 0D;
            chartArea5.AxisX.MajorTickMark.IntervalOffset = 0D;
            chartArea5.AxisX.MajorTickMark.IntervalOffsetType = System.Windows.Forms.DataVisualization.Charting.DateTimeIntervalType.Auto;
            chartArea5.AxisX.MajorTickMark.IntervalType = System.Windows.Forms.DataVisualization.Charting.DateTimeIntervalType.Auto;
            chartArea5.AxisY.LabelStyle.Interval = 0D;
            chartArea5.AxisY.LabelStyle.IntervalOffset = 0D;
            chartArea5.AxisY.LabelStyle.IntervalOffsetType = System.Windows.Forms.DataVisualization.Charting.DateTimeIntervalType.Auto;
            chartArea5.AxisY.LabelStyle.IntervalType = System.Windows.Forms.DataVisualization.Charting.DateTimeIntervalType.Auto;
            chartArea5.AxisY.MajorGrid.Interval = 0D;
            chartArea5.AxisY.MajorGrid.IntervalOffset = 0D;
            chartArea5.AxisY.MajorGrid.IntervalOffsetType = System.Windows.Forms.DataVisualization.Charting.DateTimeIntervalType.Auto;
            chartArea5.AxisY.MajorGrid.IntervalType = System.Windows.Forms.DataVisualization.Charting.DateTimeIntervalType.Auto;
            chartArea5.AxisY.MajorTickMark.Interval = 0D;
            chartArea5.AxisY.MajorTickMark.IntervalOffset = 0D;
            chartArea5.AxisY.MajorTickMark.IntervalOffsetType = System.Windows.Forms.DataVisualization.Charting.DateTimeIntervalType.Auto;
            chartArea5.AxisY.MajorTickMark.IntervalType = System.Windows.Forms.DataVisualization.Charting.DateTimeIntervalType.Auto;
            chartArea5.Name = "ChartArea1";
            this.chart1.ChartAreas.Add(chartArea5);
            this.chart1.IsSoftShadows = false;
            legend5.Name = "Legend1";
            this.chart1.Legends.Add(legend5);
            this.chart1.Location = new System.Drawing.Point(0, 0);
            this.chart1.Name = "chart1";
            series5.ChartArea = "ChartArea1";
            series5.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Line;
            series5.CustomProperties = "EmptyPointValue=Zero";
            series5.Legend = "Legend1";
            series5.Name = "Series1";
            this.chart1.Series.Add(series5);
            this.chart1.Size = new System.Drawing.Size(726, 416);
            this.chart1.SuppressExceptions = true;
            this.chart1.TabIndex = 6;
            this.chart1.Text = "chart1";
            title5.Alignment = System.Drawing.ContentAlignment.TopRight;
            title5.Name = "Title1";
            this.chart1.Titles.Add(title5);
            this.chart1.MouseMove += new System.Windows.Forms.MouseEventHandler(this.Chart1_MouseMove);
            // 
            // checkBox_autoScroll
            // 
            this.checkBox_autoScroll.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.checkBox_autoScroll.AutoSize = true;
            this.checkBox_autoScroll.Checked = true;
            this.checkBox_autoScroll.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox_autoScroll.Location = new System.Drawing.Point(599, 453);
            this.checkBox_autoScroll.Name = "checkBox_autoScroll";
            this.checkBox_autoScroll.Size = new System.Drawing.Size(72, 17);
            this.checkBox_autoScroll.TabIndex = 13;
            this.checkBox_autoScroll.Text = "Autoscroll";
            this.checkBox_autoScroll.UseVisualStyleBackColor = true;
            // 
            // button_clearLog
            // 
            this.button_clearLog.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.button_clearLog.Location = new System.Drawing.Point(651, 0);
            this.button_clearLog.Name = "button_clearLog";
            this.button_clearLog.Size = new System.Drawing.Size(75, 23);
            this.button_clearLog.TabIndex = 17;
            this.button_clearLog.Text = "Clear log";
            this.button_clearLog.UseVisualStyleBackColor = true;
            this.button_clearLog.Click += new System.EventHandler(this.Button_clearLog_Click);
            // 
            // button_loadLog
            // 
            this.button_loadLog.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.button_loadLog.Location = new System.Drawing.Point(570, 0);
            this.button_loadLog.Name = "button_loadLog";
            this.button_loadLog.Size = new System.Drawing.Size(75, 23);
            this.button_loadLog.TabIndex = 16;
            this.button_loadLog.Text = "Load log";
            this.button_loadLog.UseVisualStyleBackColor = true;
            this.button_loadLog.Click += new System.EventHandler(this.Button_loadLog_Click);
            // 
            // button_saveLog
            // 
            this.button_saveLog.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.button_saveLog.Location = new System.Drawing.Point(489, 0);
            this.button_saveLog.Name = "button_saveLog";
            this.button_saveLog.Size = new System.Drawing.Size(75, 23);
            this.button_saveLog.TabIndex = 15;
            this.button_saveLog.Text = "Save log";
            this.button_saveLog.UseVisualStyleBackColor = true;
            this.button_saveLog.Click += new System.EventHandler(this.Button_saveLog_Click);
            // 
            // openFileDialog1
            // 
            this.openFileDialog1.FileName = "openFileDialog1";
            this.openFileDialog1.FileOk += new System.ComponentModel.CancelEventHandler(this.OpenFileDialog1_FileOk);
            // 
            // saveFileDialog1
            // 
            this.saveFileDialog1.FileOk += new System.ComponentModel.CancelEventHandler(this.SaveFileDialog1_FileOk);
            // 
            // checkBox_autoReConnect
            // 
            this.checkBox_autoReConnect.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.checkBox_autoReConnect.AutoSize = true;
            this.checkBox_autoReConnect.Checked = true;
            this.checkBox_autoReConnect.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox_autoReConnect.Location = new System.Drawing.Point(621, 60);
            this.checkBox_autoReConnect.Name = "checkBox_autoReConnect";
            this.checkBox_autoReConnect.Size = new System.Drawing.Size(99, 17);
            this.checkBox_autoReConnect.TabIndex = 9;
            this.checkBox_autoReConnect.Text = "Auto reconnect";
            this.checkBox_autoReConnect.UseVisualStyleBackColor = true;
            this.checkBox_autoReConnect.CheckedChanged += new System.EventHandler(this.CheckBox_autoReconnect_CheckedChanged);
            // 
            // tabControl1
            // 
            this.tabControl1.Controls.Add(this.tabPage_connect);
            this.tabControl1.Controls.Add(this.tabPage_chart);
            this.tabControl1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tabControl1.Location = new System.Drawing.Point(0, 0);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(734, 500);
            this.tabControl1.TabIndex = 0;
            this.tabControl1.Selected += new System.Windows.Forms.TabControlEventHandler(this.TabControl1_Selected);
            // 
            // tabPage_connect
            // 
            this.tabPage_connect.Controls.Add(this.label3);
            this.tabPage_connect.Controls.Add(this.textBox_restPort);
            this.tabPage_connect.Controls.Add(this.textBox_dataLog);
            this.tabPage_connect.Controls.Add(this.label1);
            this.tabPage_connect.Controls.Add(this.label2);
            this.tabPage_connect.Controls.Add(this.label10);
            this.tabPage_connect.Controls.Add(this.label9);
            this.tabPage_connect.Controls.Add(this.label12);
            this.tabPage_connect.Controls.Add(this.label8);
            this.tabPage_connect.Controls.Add(this.textBox_mqttServer);
            this.tabPage_connect.Controls.Add(this.textBox_publishTopic);
            this.tabPage_connect.Controls.Add(this.textBox1);
            this.tabPage_connect.Controls.Add(this.textBox_subscribeTopic);
            this.tabPage_connect.Controls.Add(this.textBox_password);
            this.tabPage_connect.Controls.Add(this.textBox_login);
            this.tabPage_connect.Controls.Add(this.checkBox_hex);
            this.tabPage_connect.Controls.Add(this.textBox_message);
            this.tabPage_connect.Controls.Add(this.checkBox_autoScroll);
            this.tabPage_connect.Controls.Add(this.button_connect);
            this.tabPage_connect.Controls.Add(this.button_disconnect);
            this.tabPage_connect.Controls.Add(this.checkBox_autoConnect);
            this.tabPage_connect.Controls.Add(this.button_send);
            this.tabPage_connect.Controls.Add(this.checkBox_addTimeStamp);
            this.tabPage_connect.Controls.Add(this.checkBox_autoReConnect);
            this.tabPage_connect.Location = new System.Drawing.Point(4, 22);
            this.tabPage_connect.Name = "tabPage_connect";
            this.tabPage_connect.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage_connect.Size = new System.Drawing.Size(726, 474);
            this.tabPage_connect.TabIndex = 0;
            this.tabPage_connect.Text = "Connection";
            this.tabPage_connect.UseVisualStyleBackColor = true;
            // 
            // label3
            // 
            this.label3.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(435, 35);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(60, 13);
            this.label3.TabIndex = 18;
            this.label3.Text = "REST port:";
            // 
            // textBox_restPort
            // 
            this.textBox_restPort.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox_restPort.Location = new System.Drawing.Point(501, 32);
            this.textBox_restPort.Name = "textBox_restPort";
            this.textBox_restPort.Size = new System.Drawing.Size(55, 20);
            this.textBox_restPort.TabIndex = 4;
            this.textBox_restPort.Leave += new System.EventHandler(this.TextBox_restPort_Leave);
            // 
            // label1
            // 
            this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(326, 454);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(70, 13);
            this.label1.TabIndex = 22;
            this.label1.Text = "Publish topic:";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(2, 61);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(83, 13);
            this.label2.TabIndex = 20;
            this.label2.Text = "Subscribe topic:";
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(400, 9);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(50, 13);
            this.label10.TabIndex = 17;
            this.label10.Text = "Client ID:";
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(190, 9);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(56, 13);
            this.label9.TabIndex = 16;
            this.label9.Text = "Password:";
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.Location = new System.Drawing.Point(3, 35);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(48, 13);
            this.label12.TabIndex = 19;
            this.label12.Text = "Address:";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(3, 9);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(36, 13);
            this.label8.TabIndex = 15;
            this.label8.Text = "Login:";
            // 
            // textBox_mqttServer
            // 
            this.textBox_mqttServer.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox_mqttServer.Location = new System.Drawing.Point(57, 32);
            this.textBox_mqttServer.Name = "textBox_mqttServer";
            this.textBox_mqttServer.Size = new System.Drawing.Size(372, 20);
            this.textBox_mqttServer.TabIndex = 3;
            this.textBox_mqttServer.Leave += new System.EventHandler(this.TextBox_mqttServer_Leave);
            // 
            // textBox_publishTopic
            // 
            this.textBox_publishTopic.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox_publishTopic.Location = new System.Drawing.Point(402, 451);
            this.textBox_publishTopic.Name = "textBox_publishTopic";
            this.textBox_publishTopic.Size = new System.Drawing.Size(191, 20);
            this.textBox_publishTopic.TabIndex = 12;
            this.textBox_publishTopic.Leave += new System.EventHandler(this.TextBox_publishTopic_Leave);
            // 
            // textBox1
            // 
            this.textBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox1.Location = new System.Drawing.Point(456, 5);
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(164, 20);
            this.textBox1.TabIndex = 2;
            this.textBox1.Leave += new System.EventHandler(this.TextBox1_Leave);
            // 
            // textBox_subscribeTopic
            // 
            this.textBox_subscribeTopic.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox_subscribeTopic.Location = new System.Drawing.Point(91, 58);
            this.textBox_subscribeTopic.Name = "textBox_subscribeTopic";
            this.textBox_subscribeTopic.Size = new System.Drawing.Size(269, 20);
            this.textBox_subscribeTopic.TabIndex = 7;
            this.textBox_subscribeTopic.Leave += new System.EventHandler(this.TextBox_subscribeTopic_Leave);
            // 
            // textBox_password
            // 
            this.textBox_password.Location = new System.Drawing.Point(252, 6);
            this.textBox_password.Name = "textBox_password";
            this.textBox_password.Size = new System.Drawing.Size(142, 20);
            this.textBox_password.TabIndex = 1;
            this.textBox_password.Leave += new System.EventHandler(this.TextBox_password_Leave);
            // 
            // textBox_login
            // 
            this.textBox_login.Location = new System.Drawing.Point(45, 6);
            this.textBox_login.Name = "textBox_login";
            this.textBox_login.Size = new System.Drawing.Size(139, 20);
            this.textBox_login.TabIndex = 0;
            this.textBox_login.Leave += new System.EventHandler(this.TextBox_login_Leave);
            // 
            // checkBox_autoConnect
            // 
            this.checkBox_autoConnect.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.checkBox_autoConnect.AutoSize = true;
            this.checkBox_autoConnect.Checked = true;
            this.checkBox_autoConnect.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox_autoConnect.Location = new System.Drawing.Point(528, 60);
            this.checkBox_autoConnect.Name = "checkBox_autoConnect";
            this.checkBox_autoConnect.Size = new System.Drawing.Size(87, 17);
            this.checkBox_autoConnect.TabIndex = 8;
            this.checkBox_autoConnect.Text = "Autoconnect";
            this.checkBox_autoConnect.UseVisualStyleBackColor = true;
            this.checkBox_autoConnect.CheckedChanged += new System.EventHandler(this.CheckBox_autoconnect_CheckedChanged);
            // 
            // checkBox_addTimeStamp
            // 
            this.checkBox_addTimeStamp.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.checkBox_addTimeStamp.AutoSize = true;
            this.checkBox_addTimeStamp.Checked = true;
            this.checkBox_addTimeStamp.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox_addTimeStamp.Location = new System.Drawing.Point(626, 8);
            this.checkBox_addTimeStamp.Name = "checkBox_addTimeStamp";
            this.checkBox_addTimeStamp.Size = new System.Drawing.Size(94, 17);
            this.checkBox_addTimeStamp.TabIndex = 14;
            this.checkBox_addTimeStamp.Text = "add timestamp";
            this.checkBox_addTimeStamp.UseVisualStyleBackColor = true;
            this.checkBox_addTimeStamp.CheckedChanged += new System.EventHandler(this.CheckBox_autoReconnect_CheckedChanged);
            // 
            // tabPage_chart
            // 
            this.tabPage_chart.Controls.Add(this.button_resetChart);
            this.tabPage_chart.Controls.Add(this.label7);
            this.tabPage_chart.Controls.Add(this.button_saveLog);
            this.tabPage_chart.Controls.Add(this.button_loadLog);
            this.tabPage_chart.Controls.Add(this.trackBar_max);
            this.tabPage_chart.Controls.Add(this.button_clearLog);
            this.tabPage_chart.Controls.Add(this.label6);
            this.tabPage_chart.Controls.Add(this.checkBox_autoRangeValue);
            this.tabPage_chart.Controls.Add(this.textBox_fromTime);
            this.tabPage_chart.Controls.Add(this.checkedListBox_params);
            this.tabPage_chart.Controls.Add(this.textBox_toTime);
            this.tabPage_chart.Controls.Add(this.label14);
            this.tabPage_chart.Controls.Add(this.label15);
            this.tabPage_chart.Controls.Add(this.label13);
            this.tabPage_chart.Controls.Add(this.label5);
            this.tabPage_chart.Controls.Add(this.label4);
            this.tabPage_chart.Controls.Add(this.trackBar_min);
            this.tabPage_chart.Controls.Add(this.textBox_keepTime);
            this.tabPage_chart.Controls.Add(this.textBox_minValue);
            this.tabPage_chart.Controls.Add(this.textBox_maxValue);
            this.tabPage_chart.Controls.Add(this.chart1);
            this.tabPage_chart.Location = new System.Drawing.Point(4, 22);
            this.tabPage_chart.Name = "tabPage_chart";
            this.tabPage_chart.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage_chart.Size = new System.Drawing.Size(726, 474);
            this.tabPage_chart.TabIndex = 1;
            this.tabPage_chart.Text = "Chart";
            this.tabPage_chart.UseVisualStyleBackColor = true;
            // 
            // button_resetChart
            // 
            this.button_resetChart.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.button_resetChart.Location = new System.Drawing.Point(408, 0);
            this.button_resetChart.Name = "button_resetChart";
            this.button_resetChart.Size = new System.Drawing.Size(75, 23);
            this.button_resetChart.TabIndex = 14;
            this.button_resetChart.Text = "Reset chart";
            this.button_resetChart.UseVisualStyleBackColor = true;
            this.button_resetChart.Visible = false;
            this.button_resetChart.Click += new System.EventHandler(this.Button_resetChart_Click);
            // 
            // label14
            // 
            this.label14.AutoSize = true;
            this.label14.Location = new System.Drawing.Point(410, 5);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(43, 13);
            this.label14.TabIndex = 13;
            this.label14.Text = "minutes";
            // 
            // label15
            // 
            this.label15.AutoSize = true;
            this.label15.Location = new System.Drawing.Point(395, 5);
            this.label15.Name = "label15";
            this.label15.Size = new System.Drawing.Size(26, 13);
            this.label15.TabIndex = 12;
            this.label15.Text = "min.";
            // 
            // label13
            // 
            this.label13.AutoSize = true;
            this.label13.Location = new System.Drawing.Point(290, 5);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(62, 13);
            this.label13.TabIndex = 12;
            this.label13.Text = "Show latest";
            // 
            // textBox_keepTime
            // 
            this.textBox_keepTime.Location = new System.Drawing.Point(358, 2);
            this.textBox_keepTime.MaxLength = 4;
            this.textBox_keepTime.Name = "textBox_keepTime";
            this.textBox_keepTime.Size = new System.Drawing.Size(31, 20);
            this.textBox_keepTime.TabIndex = 2;
            this.textBox_keepTime.Text = "0";
            this.textBox_keepTime.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.textBox_keepTime.KeyDown += new System.Windows.Forms.KeyEventHandler(this.TextBox_keepTime_KeyDown);
            this.textBox_keepTime.Leave += new System.EventHandler(this.TextBox_keepTime_Leave);
            // 
            // notifyIcon1
            // 
            this.notifyIcon1.BalloonTipIcon = System.Windows.Forms.ToolTipIcon.Info;
            this.notifyIcon1.BalloonTipTitle = "ChartPlotMQTT";
            this.notifyIcon1.Icon = ((System.Drawing.Icon)(resources.GetObject("notifyIcon1.Icon")));
            this.notifyIcon1.Tag = "ChartPlotMQTT";
            this.notifyIcon1.Text = "ChartPlotMQTT";
            this.notifyIcon1.Visible = true;
            this.notifyIcon1.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.NotifyIcon1_MouseDoubleClick);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(734, 500);
            this.Controls.Add(this.tabControl1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(750, 210);
            this.Name = "Form1";
            this.Text = "ChartPlotMQTT";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Form1_FormClosing);
            this.Load += new System.EventHandler(this.Form1_Load);
            this.SizeChanged += new System.EventHandler(this.Form1_SizeChanged);
            this.Resize += new System.EventHandler(this.Form1_Resize);
            ((System.ComponentModel.ISupportInitialize)(this.trackBar_max)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar_min)).EndInit();
            this.contextMenuStrip_item.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.chart1)).EndInit();
            this.tabControl1.ResumeLayout(false);
            this.tabPage_connect.ResumeLayout(false);
            this.tabPage_connect.PerformLayout();
            this.tabPage_chart.ResumeLayout(false);
            this.tabPage_chart.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion
        private System.Windows.Forms.TextBox textBox_dataLog;
        private System.Windows.Forms.TextBox textBox_message;
        private System.Windows.Forms.Button button_send;
        private System.Windows.Forms.Button button_connect;
        private System.Windows.Forms.Button button_disconnect;
        private System.Windows.Forms.CheckBox checkBox_hex;
        private System.Windows.Forms.Button button_clearLog;
        private System.Windows.Forms.CheckBox checkBox_autoScroll;
        private System.Windows.Forms.DataVisualization.Charting.Chart chart1;
        private System.Windows.Forms.CheckedListBox checkedListBox_params;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox textBox_minValue;
        private System.Windows.Forms.TextBox textBox_maxValue;
        private System.Windows.Forms.TrackBar trackBar_max;
        private System.Windows.Forms.TrackBar trackBar_min;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.TextBox textBox_fromTime;
        private System.Windows.Forms.TextBox textBox_toTime;
        private System.Windows.Forms.Button button_loadLog;
        private System.Windows.Forms.Button button_saveLog;
        private System.Windows.Forms.OpenFileDialog openFileDialog1;
        private System.Windows.Forms.SaveFileDialog saveFileDialog1;
        private System.Windows.Forms.CheckBox checkBox_autoRangeValue;
        private System.Windows.Forms.CheckBox checkBox_autoReConnect;
        private System.Windows.Forms.TabControl tabControl1;
        private System.Windows.Forms.TabPage tabPage_connect;
        private System.Windows.Forms.TabPage tabPage_chart;
        private System.Windows.Forms.ContextMenuStrip contextMenuStrip_item;
        private System.Windows.Forms.ToolStripMenuItem toolStripMenuItem_delete;
        private System.Windows.Forms.ToolStripMenuItem saveSelectedToolStripMenuItem;
        private System.Windows.Forms.TextBox textBox_mqttServer;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox textBox_publishTopic;
        private System.Windows.Forms.TextBox textBox_subscribeTopic;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.TextBox textBox_password;
        private System.Windows.Forms.TextBox textBox_login;
        private System.Windows.Forms.CheckBox checkBox_autoConnect;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.Label label12;
        private System.Windows.Forms.CheckBox checkBox_addTimeStamp;
        private System.Windows.Forms.Label label14;
        private System.Windows.Forms.Label label13;
        private System.Windows.Forms.TextBox textBox_keepTime;
        private System.Windows.Forms.NotifyIcon notifyIcon1;
        private System.Windows.Forms.Button button_resetChart;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox textBox_restPort;
        private System.Windows.Forms.Label label15;
    }
}

