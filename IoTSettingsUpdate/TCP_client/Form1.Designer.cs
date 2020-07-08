namespace IoTSettingsUpdate
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
            this.textBox_dataLog = new System.Windows.Forms.TextBox();
            this.button_send = new System.Windows.Forms.Button();
            this.button_connect = new System.Windows.Forms.Button();
            this.button_disconnect = new System.Windows.Forms.Button();
            this.button_receive = new System.Windows.Forms.Button();
            this.checkBox_hex = new System.Windows.Forms.CheckBox();
            this.contextMenuStrip_item = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.toolStripMenuItem_delete = new System.Windows.Forms.ToolStripMenuItem();
            this.saveSelectedToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.checkBox_autoScroll = new System.Windows.Forms.CheckBox();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
            this.saveFileDialog1 = new System.Windows.Forms.SaveFileDialog();
            this.comboBox_portspeed1 = new System.Windows.Forms.ComboBox();
            this.comboBox_portname1 = new System.Windows.Forms.ComboBox();
            this.checkBox_addCrLf = new System.Windows.Forms.CheckBox();
            this.serialPort1 = new System.IO.Ports.SerialPort(this.components);
            this.dataGridView_config = new System.Windows.Forms.DataGridView();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.button_clear = new System.Windows.Forms.Button();
            this.button_save = new System.Windows.Forms.Button();
            this.button_load = new System.Windows.Forms.Button();
            this.button_sendAll = new System.Windows.Forms.Button();
            this.button_getConfig = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.textBox_replyTimeout = new System.Windows.Forms.TextBox();
            this.textBox_customCommand = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.button_sendCommand = new System.Windows.Forms.Button();
            this.button_setTime = new System.Windows.Forms.Button();
            this.button_getSensor = new System.Windows.Forms.Button();
            this.button_getStatus = new System.Windows.Forms.Button();
            this.button_scanIp = new System.Windows.Forms.Button();
            this.contextMenuStrip_item.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dataGridView_config)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.SuspendLayout();
            // 
            // textBox_dataLog
            // 
            this.textBox_dataLog.Dock = System.Windows.Forms.DockStyle.Fill;
            this.textBox_dataLog.Location = new System.Drawing.Point(0, 0);
            this.textBox_dataLog.Multiline = true;
            this.textBox_dataLog.Name = "textBox_dataLog";
            this.textBox_dataLog.ReadOnly = true;
            this.textBox_dataLog.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.textBox_dataLog.Size = new System.Drawing.Size(383, 328);
            this.textBox_dataLog.TabIndex = 9;
            // 
            // button_send
            // 
            this.button_send.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.button_send.Enabled = false;
            this.button_send.Location = new System.Drawing.Point(12, 373);
            this.button_send.Name = "button_send";
            this.button_send.Size = new System.Drawing.Size(75, 23);
            this.button_send.TabIndex = 6;
            this.button_send.Text = "Send";
            this.button_send.UseVisualStyleBackColor = true;
            this.button_send.Click += new System.EventHandler(this.Button_send_Click);
            // 
            // button_connect
            // 
            this.button_connect.Location = new System.Drawing.Point(299, 10);
            this.button_connect.Name = "button_connect";
            this.button_connect.Size = new System.Drawing.Size(75, 23);
            this.button_connect.TabIndex = 2;
            this.button_connect.Text = "Connect";
            this.button_connect.UseVisualStyleBackColor = true;
            this.button_connect.Click += new System.EventHandler(this.Button_connect_Click);
            // 
            // button_disconnect
            // 
            this.button_disconnect.Enabled = false;
            this.button_disconnect.Location = new System.Drawing.Point(380, 10);
            this.button_disconnect.Name = "button_disconnect";
            this.button_disconnect.Size = new System.Drawing.Size(75, 23);
            this.button_disconnect.TabIndex = 3;
            this.button_disconnect.Text = "Disconnect";
            this.button_disconnect.UseVisualStyleBackColor = true;
            this.button_disconnect.Click += new System.EventHandler(this.Button_disconnect_Click);
            // 
            // button_receive
            // 
            this.button_receive.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.button_receive.Enabled = false;
            this.button_receive.Location = new System.Drawing.Point(576, 399);
            this.button_receive.Name = "button_receive";
            this.button_receive.Size = new System.Drawing.Size(75, 23);
            this.button_receive.TabIndex = 7;
            this.button_receive.Text = "Receive";
            this.button_receive.UseVisualStyleBackColor = true;
            this.button_receive.Click += new System.EventHandler(this.Button_receive_Click);
            // 
            // checkBox_hex
            // 
            this.checkBox_hex.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.checkBox_hex.AutoSize = true;
            this.checkBox_hex.Location = new System.Drawing.Point(735, 403);
            this.checkBox_hex.Name = "checkBox_hex";
            this.checkBox_hex.Size = new System.Drawing.Size(43, 17);
            this.checkBox_hex.TabIndex = 4;
            this.checkBox_hex.Text = "hex";
            this.checkBox_hex.UseVisualStyleBackColor = true;
            this.checkBox_hex.CheckedChanged += new System.EventHandler(this.CheckBox_hex_CheckedChanged);
            // 
            // contextMenuStrip_item
            // 
            this.contextMenuStrip_item.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripMenuItem_delete,
            this.saveSelectedToolStripMenuItem});
            this.contextMenuStrip_item.Name = "contextMenuStrip_item";
            this.contextMenuStrip_item.Size = new System.Drawing.Size(68, 48);
            // 
            // toolStripMenuItem_delete
            // 
            this.toolStripMenuItem_delete.Name = "toolStripMenuItem_delete";
            this.toolStripMenuItem_delete.Size = new System.Drawing.Size(67, 22);
            // 
            // saveSelectedToolStripMenuItem
            // 
            this.saveSelectedToolStripMenuItem.Name = "saveSelectedToolStripMenuItem";
            this.saveSelectedToolStripMenuItem.Size = new System.Drawing.Size(67, 22);
            // 
            // checkBox_autoScroll
            // 
            this.checkBox_autoScroll.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.checkBox_autoScroll.AutoSize = true;
            this.checkBox_autoScroll.Checked = true;
            this.checkBox_autoScroll.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox_autoScroll.Location = new System.Drawing.Point(657, 403);
            this.checkBox_autoScroll.Name = "checkBox_autoScroll";
            this.checkBox_autoScroll.Size = new System.Drawing.Size(72, 17);
            this.checkBox_autoScroll.TabIndex = 8;
            this.checkBox_autoScroll.Text = "Autoscroll";
            this.checkBox_autoScroll.UseVisualStyleBackColor = true;
            this.checkBox_autoScroll.CheckedChanged += new System.EventHandler(this.CheckBox_autoScroll_CheckedChanged);
            // 
            // timer1
            // 
            this.timer1.Interval = 1000;
            this.timer1.Tick += new System.EventHandler(this.Timer1_Tick);
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
            // comboBox_portspeed1
            // 
            this.comboBox_portspeed1.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBox_portspeed1.FormattingEnabled = true;
            this.comboBox_portspeed1.Items.AddRange(new object[] {
            "250000",
            "230400",
            "115200",
            "57600",
            "38400",
            "19200",
            "9600",
            "4800",
            "2400",
            "1200",
            "600",
            "300"});
            this.comboBox_portspeed1.Location = new System.Drawing.Point(208, 12);
            this.comboBox_portspeed1.Name = "comboBox_portspeed1";
            this.comboBox_portspeed1.Size = new System.Drawing.Size(85, 21);
            this.comboBox_portspeed1.TabIndex = 14;
            this.comboBox_portspeed1.SelectedIndexChanged += new System.EventHandler(this.ComboBox_portspeed1_SelectedIndexChanged);
            // 
            // comboBox_portname1
            // 
            this.comboBox_portname1.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBox_portname1.ForeColor = System.Drawing.SystemColors.WindowText;
            this.comboBox_portname1.FormattingEnabled = true;
            this.comboBox_portname1.Location = new System.Drawing.Point(12, 12);
            this.comboBox_portname1.Name = "comboBox_portname1";
            this.comboBox_portname1.Size = new System.Drawing.Size(190, 21);
            this.comboBox_portname1.TabIndex = 13;
            this.comboBox_portname1.DropDown += new System.EventHandler(this.ComboBox_portname1_DropDown);
            this.comboBox_portname1.SelectedIndexChanged += new System.EventHandler(this.ComboBox_portname1_SelectedIndexChanged);
            this.comboBox_portname1.Leave += new System.EventHandler(this.ComboBox_portname1_Leave);
            // 
            // checkBox_addCrLf
            // 
            this.checkBox_addCrLf.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.checkBox_addCrLf.AutoSize = true;
            this.checkBox_addCrLf.Checked = true;
            this.checkBox_addCrLf.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox_addCrLf.Location = new System.Drawing.Point(489, 403);
            this.checkBox_addCrLf.Name = "checkBox_addCrLf";
            this.checkBox_addCrLf.Size = new System.Drawing.Size(81, 17);
            this.checkBox_addCrLf.TabIndex = 8;
            this.checkBox_addCrLf.Text = "Add CR+LF";
            this.checkBox_addCrLf.UseVisualStyleBackColor = true;
            // 
            // serialPort1
            // 
            this.serialPort1.ReadBufferSize = 8192;
            this.serialPort1.WriteBufferSize = 8192;
            this.serialPort1.DataReceived += new System.IO.Ports.SerialDataReceivedEventHandler(this.SerialPort1_DataReceived);
            // 
            // dataGridView_config
            // 
            this.dataGridView_config.ClipboardCopyMode = System.Windows.Forms.DataGridViewClipboardCopyMode.EnableWithoutHeaderText;
            this.dataGridView_config.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dataGridView_config.Dock = System.Windows.Forms.DockStyle.Fill;
            this.dataGridView_config.EditMode = System.Windows.Forms.DataGridViewEditMode.EditOnKeystroke;
            this.dataGridView_config.Location = new System.Drawing.Point(0, 0);
            this.dataGridView_config.MultiSelect = false;
            this.dataGridView_config.Name = "dataGridView_config";
            this.dataGridView_config.RowHeadersWidthSizeMode = System.Windows.Forms.DataGridViewRowHeadersWidthSizeMode.AutoSizeToAllHeaders;
            this.dataGridView_config.RowTemplate.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.dataGridView_config.Size = new System.Drawing.Size(379, 328);
            this.dataGridView_config.TabIndex = 15;
            this.dataGridView_config.CellValueChanged += new System.Windows.Forms.DataGridViewCellEventHandler(this.DataGridView_config_CellValueChanged);
            this.dataGridView_config.KeyDown += new System.Windows.Forms.KeyEventHandler(this.DataGridView_config_KeyDown);
            // 
            // splitContainer1
            // 
            this.splitContainer1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.splitContainer1.Location = new System.Drawing.Point(12, 39);
            this.splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.dataGridView_config);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.textBox_dataLog);
            this.splitContainer1.Size = new System.Drawing.Size(766, 328);
            this.splitContainer1.SplitterDistance = 379;
            this.splitContainer1.TabIndex = 16;
            // 
            // button_clear
            // 
            this.button_clear.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.button_clear.Location = new System.Drawing.Point(703, 10);
            this.button_clear.Name = "button_clear";
            this.button_clear.Size = new System.Drawing.Size(75, 23);
            this.button_clear.TabIndex = 17;
            this.button_clear.Text = "Clear log";
            this.button_clear.UseVisualStyleBackColor = true;
            this.button_clear.Click += new System.EventHandler(this.Button_clear_Click);
            // 
            // button_save
            // 
            this.button_save.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.button_save.Location = new System.Drawing.Point(622, 10);
            this.button_save.Name = "button_save";
            this.button_save.Size = new System.Drawing.Size(75, 23);
            this.button_save.TabIndex = 18;
            this.button_save.Text = "Save";
            this.button_save.UseVisualStyleBackColor = true;
            this.button_save.Click += new System.EventHandler(this.Button_save_Click);
            // 
            // button_load
            // 
            this.button_load.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.button_load.Location = new System.Drawing.Point(541, 10);
            this.button_load.Name = "button_load";
            this.button_load.Size = new System.Drawing.Size(75, 23);
            this.button_load.TabIndex = 19;
            this.button_load.Text = "Load";
            this.button_load.UseVisualStyleBackColor = true;
            this.button_load.Click += new System.EventHandler(this.Button_load_Click);
            // 
            // button_sendAll
            // 
            this.button_sendAll.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.button_sendAll.Enabled = false;
            this.button_sendAll.Location = new System.Drawing.Point(93, 373);
            this.button_sendAll.Name = "button_sendAll";
            this.button_sendAll.Size = new System.Drawing.Size(75, 23);
            this.button_sendAll.TabIndex = 20;
            this.button_sendAll.Text = "Send all";
            this.button_sendAll.UseVisualStyleBackColor = true;
            this.button_sendAll.Click += new System.EventHandler(this.Button_sendAll_Click);
            // 
            // button_getConfig
            // 
            this.button_getConfig.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.button_getConfig.Enabled = false;
            this.button_getConfig.Location = new System.Drawing.Point(93, 403);
            this.button_getConfig.Name = "button_getConfig";
            this.button_getConfig.Size = new System.Drawing.Size(75, 23);
            this.button_getConfig.TabIndex = 21;
            this.button_getConfig.Text = "Get config";
            this.button_getConfig.UseVisualStyleBackColor = true;
            this.button_getConfig.Click += new System.EventHandler(this.Button_getConfig_Click);
            // 
            // label1
            // 
            this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(348, 403);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(69, 13);
            this.label1.TabIndex = 22;
            this.label1.Text = "reply timeout:";
            // 
            // textBox_replyTimeout
            // 
            this.textBox_replyTimeout.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox_replyTimeout.Location = new System.Drawing.Point(424, 400);
            this.textBox_replyTimeout.MaxLength = 5;
            this.textBox_replyTimeout.Name = "textBox_replyTimeout";
            this.textBox_replyTimeout.Size = new System.Drawing.Size(59, 20);
            this.textBox_replyTimeout.TabIndex = 23;
            this.textBox_replyTimeout.Leave += new System.EventHandler(this.TextBox_replyTimeout_Leave);
            // 
            // textBox_customCommand
            // 
            this.textBox_customCommand.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox_customCommand.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.SuggestAppend;
            this.textBox_customCommand.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.CustomSource;
            this.textBox_customCommand.Location = new System.Drawing.Point(352, 375);
            this.textBox_customCommand.Name = "textBox_customCommand";
            this.textBox_customCommand.Size = new System.Drawing.Size(345, 20);
            this.textBox_customCommand.TabIndex = 24;
            this.textBox_customCommand.KeyUp += new System.Windows.Forms.KeyEventHandler(this.TextBox_customCommand_KeyUp);
            // 
            // label2
            // 
            this.label2.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(255, 378);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(91, 13);
            this.label2.TabIndex = 22;
            this.label2.Text = "Custom command";
            // 
            // button_sendCommand
            // 
            this.button_sendCommand.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.button_sendCommand.Enabled = false;
            this.button_sendCommand.Location = new System.Drawing.Point(703, 373);
            this.button_sendCommand.Name = "button_sendCommand";
            this.button_sendCommand.Size = new System.Drawing.Size(75, 23);
            this.button_sendCommand.TabIndex = 25;
            this.button_sendCommand.Text = "Send";
            this.button_sendCommand.UseVisualStyleBackColor = true;
            this.button_sendCommand.Click += new System.EventHandler(this.Button_sendCommand_Click);
            // 
            // button_setTime
            // 
            this.button_setTime.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.button_setTime.Enabled = false;
            this.button_setTime.Location = new System.Drawing.Point(174, 373);
            this.button_setTime.Name = "button_setTime";
            this.button_setTime.Size = new System.Drawing.Size(75, 23);
            this.button_setTime.TabIndex = 21;
            this.button_setTime.Text = "Set time";
            this.button_setTime.UseVisualStyleBackColor = true;
            this.button_setTime.Click += new System.EventHandler(this.Button_setTime_Click);
            // 
            // button_getSensor
            // 
            this.button_getSensor.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.button_getSensor.Enabled = false;
            this.button_getSensor.Location = new System.Drawing.Point(174, 403);
            this.button_getSensor.Name = "button_getSensor";
            this.button_getSensor.Size = new System.Drawing.Size(75, 23);
            this.button_getSensor.TabIndex = 21;
            this.button_getSensor.Text = "Get sensor";
            this.button_getSensor.UseVisualStyleBackColor = true;
            this.button_getSensor.Click += new System.EventHandler(this.Button_getSensor_Click);
            // 
            // button_getStatus
            // 
            this.button_getStatus.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.button_getStatus.Enabled = false;
            this.button_getStatus.Location = new System.Drawing.Point(12, 403);
            this.button_getStatus.Name = "button_getStatus";
            this.button_getStatus.Size = new System.Drawing.Size(75, 23);
            this.button_getStatus.TabIndex = 21;
            this.button_getStatus.Text = "Get status";
            this.button_getStatus.UseVisualStyleBackColor = true;
            this.button_getStatus.Click += new System.EventHandler(this.Button_getStatus_Click);
            // 
            // button_scanIp
            // 
            this.button_scanIp.Location = new System.Drawing.Point(461, 10);
            this.button_scanIp.Name = "button_scanIp";
            this.button_scanIp.Size = new System.Drawing.Size(75, 23);
            this.button_scanIp.TabIndex = 26;
            this.button_scanIp.Text = "Scan IP";
            this.button_scanIp.UseVisualStyleBackColor = true;
            this.button_scanIp.Click += new System.EventHandler(this.button_scanIp_ClickAsync);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(790, 438);
            this.Controls.Add(this.button_scanIp);
            this.Controls.Add(this.button_sendCommand);
            this.Controls.Add(this.textBox_customCommand);
            this.Controls.Add(this.textBox_replyTimeout);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.button_getStatus);
            this.Controls.Add(this.button_getSensor);
            this.Controls.Add(this.button_setTime);
            this.Controls.Add(this.button_getConfig);
            this.Controls.Add(this.button_sendAll);
            this.Controls.Add(this.button_load);
            this.Controls.Add(this.button_save);
            this.Controls.Add(this.button_clear);
            this.Controls.Add(this.splitContainer1);
            this.Controls.Add(this.comboBox_portspeed1);
            this.Controls.Add(this.comboBox_portname1);
            this.Controls.Add(this.button_send);
            this.Controls.Add(this.checkBox_hex);
            this.Controls.Add(this.button_disconnect);
            this.Controls.Add(this.button_receive);
            this.Controls.Add(this.checkBox_addCrLf);
            this.Controls.Add(this.button_connect);
            this.Controls.Add(this.checkBox_autoScroll);
            this.MinimumSize = new System.Drawing.Size(800, 300);
            this.Name = "Form1";
            this.Text = "IoT settings updater";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Form1_FormClosing);
            this.Load += new System.EventHandler(this.Form1_Load);
            this.contextMenuStrip_item.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.dataGridView_config)).EndInit();
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.Panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.TextBox textBox_dataLog;
        private System.Windows.Forms.Button button_send;
        private System.Windows.Forms.Button button_connect;
        private System.Windows.Forms.Button button_disconnect;
        private System.Windows.Forms.Button button_receive;
        private System.Windows.Forms.CheckBox checkBox_hex;
        private System.Windows.Forms.CheckBox checkBox_autoScroll;
        private System.Windows.Forms.Timer timer1;
        private System.Windows.Forms.OpenFileDialog openFileDialog1;
        private System.Windows.Forms.SaveFileDialog saveFileDialog1;
        private System.Windows.Forms.ComboBox comboBox_portspeed1;
        private System.Windows.Forms.ComboBox comboBox_portname1;
        private System.IO.Ports.SerialPort serialPort1;
        private System.Windows.Forms.CheckBox checkBox_addCrLf;
        private System.Windows.Forms.ContextMenuStrip contextMenuStrip_item;
        private System.Windows.Forms.ToolStripMenuItem toolStripMenuItem_delete;
        private System.Windows.Forms.ToolStripMenuItem saveSelectedToolStripMenuItem;
        private System.Windows.Forms.DataGridView dataGridView_config;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.Button button_clear;
        private System.Windows.Forms.Button button_save;
        private System.Windows.Forms.Button button_load;
        private System.Windows.Forms.Button button_sendAll;
        private System.Windows.Forms.Button button_getConfig;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox textBox_replyTimeout;
        private System.Windows.Forms.TextBox textBox_customCommand;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button button_sendCommand;
        private System.Windows.Forms.Button button_setTime;
        private System.Windows.Forms.Button button_getSensor;
        private System.Windows.Forms.Button button_getStatus;
        private System.Windows.Forms.Button button_scanIp;
    }
}

