using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace IoTSettingsUpdate
{
    public class TextLogger : IDisposable, INotifyPropertyChanged
    {
        // ring buffer for strings
        // string complete timeout
        // string length limit
        // Time/date format combine

        public bool noScreenOutput = false;
        public int LinesLimit = 500;
        public string LogFileName = "";
        public bool AutoSave = false;
        public bool AutoScroll = true;
        public bool FilterZeroChar = true;
        public TextFormat DefaultTextFormat = TextFormat.PlainText; //Text, HEX, Auto (change non-readable to <HEX>)
        public TimeFormat DefaultTimeFormat = TimeFormat.LongTime;

        private delegate void SetTextCallback(string text);

        private readonly Form mainForm;
        private readonly TextBox _textBox;
        private List<string> _logBuffer = new List<string>();
        private StringBuilder _unfinishedString = new StringBuilder();
        private int selStart = 0;
        private volatile bool textChanged = false;
        private Timer formTimer;

        public List<string> Channels = new List<string> { "" };
        private readonly object _textOutThreadLock = new object();

        private byte _prevChannel = 0;

        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged()
        {
            textChanged = true;
            mainForm?.Invoke((MethodInvoker)delegate
            {

                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Text"));
            });
        }

        public string Text
        {
            get
            {
                return ToString();
            }
        }

        public TextLogger(Form mainForm, TextBox textBox = null)
        {
            this.mainForm = mainForm;
            _textBox = textBox;
        }

        public void TimerStart(int delay)
        {
            if (mainForm != null && _textBox != null)
            {
                formTimer = new Timer();
                formTimer.Tick += new EventHandler(FormTimer_Tick);
                formTimer.Interval = delay;
                formTimer.Start();
            }
        }

        public void TimerStop()
        {
            formTimer?.Stop();
            formTimer?.Dispose();
        }

        private void FormTimer_Tick(object sender, EventArgs e)
        {
            UpdateDisplay();
        }

        public enum TextFormat
        {
            Default,
            PlainText,
            Hex,
            AutoReplaceHex
        }

        public enum TimeFormat
        {
            Default,
            ShortTime,
            LongTime,
            ShortDate,
            LongDate,
            Full
        }

        public bool AddText(string text, byte channel = 0, TextFormat textTextFormat = TextFormat.Default, TimeFormat timeFormat = TimeFormat.Default)
        {
            return AddText(text, DateTime.MinValue, channel, textTextFormat, timeFormat);
        }

        public bool AddText(string text, byte channel = 0, TimeFormat timeFormat = TimeFormat.Default, TextFormat textTextFormat = TextFormat.Default)
        {
            return AddText(text, DateTime.MinValue, channel, textTextFormat, timeFormat);
        }

        public bool AddText(string text, DateTime logTime, byte channel = 0, TextFormat textFormat = TextFormat.Default, TimeFormat timeFormat = TimeFormat.Default)
        {
            if (text == null || text.Length <= 0) return true;

            var tmpStr = new StringBuilder();
            if (channel != _prevChannel)
            {
                if (_unfinishedString.Length > 0) AddText(Environment.NewLine, logTime, _prevChannel, textFormat, timeFormat);
                _prevChannel = channel;
            }

            if (logTime != DateTime.MinValue)
            {
                if (timeFormat == TimeFormat.Default)
                    timeFormat = DefaultTimeFormat;

                if (timeFormat == TimeFormat.Full)
                    tmpStr.Append(logTime.ToShortDateString() + " " + logTime.ToLongTimeString() + "." + logTime.Millisecond.ToString("D3") + " ");

                else if (timeFormat == TimeFormat.LongTime)
                    tmpStr.Append(logTime.ToLongTimeString() + "." + logTime.Millisecond.ToString("D3") + " ");

                else if (timeFormat == TimeFormat.LongDate)
                    tmpStr.Append(logTime.ToLongDateString() + " ");

                else if (timeFormat == TimeFormat.ShortTime)
                    tmpStr.Append(logTime.ToShortTimeString() + " ");

                else if (timeFormat == TimeFormat.ShortDate)
                    tmpStr.Append(logTime.ToShortDateString() + " ");
            }

            if (channel >= Channels.Count)
            {
                channel = 0;
            }

            if (!string.IsNullOrEmpty(Channels[channel]))
            {
                tmpStr.Append(Channels[channel] + " ");
            }

            if (textFormat == TextFormat.Default)
            {
                textFormat = DefaultTextFormat;
            }

            if (textFormat == TextFormat.PlainText)
            {
                tmpStr.Append(text);
            }
            else if (textFormat == TextFormat.Hex)
            {
                tmpStr.Append(Accessory.ConvertStringToHex(text));
                tmpStr.Append("\r\n");
            }
            else if (textFormat == TextFormat.AutoReplaceHex)
            {
                tmpStr.Append(ReplaceUnprintable(text));
                tmpStr.Append("\r\n");
            }

            string[] inputStrings = ConvertTextToStringArray(tmpStr.ToString(), ref _unfinishedString);
            return AddTextToBuffer(inputStrings);
        }

        private bool AddTextToBuffer(string[] text)
        {
            if (text == null || text.Length <= 0)
            {
                return false;
            }
            lock (_textOutThreadLock)
            {
                if (FilterZeroChar)
                {
                    for (var i = 0; i < text.Length; i++)
                    {
                        text[i] = Accessory.FilterZeroChar(text[i], true);
                    }
                }

                if (AutoSave && !string.IsNullOrEmpty(LogFileName))
                {
                    for (var i = 0; i < text.Length; i++)
                    {
                        File.AppendAllText(LogFileName, text[i] + Environment.NewLine);
                    }
                }

                if (noScreenOutput)
                {
                    return true;
                }

                if (_textBox != null) selStart = _textBox.SelectionStart;

                _logBuffer.AddRange(text);
                if (_logBuffer.Count >= LinesLimit)
                {
                    while (_logBuffer.Count >= LinesLimit)
                    {
                        if (_textBox != null) selStart -= _logBuffer[0].Length;
                        _logBuffer.RemoveAt(0);
                    }
                }
                if (_textBox != null && selStart < 0) selStart = 0;

                OnPropertyChanged();
            }

            return true;
        }

        public void UpdateDisplay()
        {
            if (_textBox != null && textChanged)
            {
                mainForm?.Invoke((MethodInvoker)delegate
                {
                    //var posLength = _textBox.SelectionLength;
                    _textBox.Text = ToString();
                    if (AutoScroll)
                    {
                        _textBox.SelectionStart = _textBox.Text.Length;
                        _textBox.ScrollToCaret();
                    }
                    else
                    {
                        _textBox.SelectionStart = selStart;
                        //_textBox.SelectionLength = posLength;
                        _textBox.ScrollToCaret();
                    }

                    textChanged = false;
                });
            }
        }

        public override string ToString()
        {
            var tmpTxt = new StringBuilder();
            foreach (var str in _logBuffer)
            {
                tmpTxt.Append(str + Environment.NewLine);
            }

            tmpTxt.Append(_unfinishedString.ToString());

            return tmpTxt.ToString();
        }

        private string[] ConvertTextToStringArray(string data, ref StringBuilder nonComplete)
        {
            var divider = new HashSet<char>
            {
                '\r',
                '\n'
            };

            var stringCollection = new List<string>();

            foreach (var t in data)
            {
                if (divider.Contains(t))
                {
                    if (nonComplete.Length > 0) stringCollection.Add(nonComplete.ToString());
                    nonComplete.Clear();
                }
                else
                {
                    if (!divider.Contains(t)) nonComplete.Append(t);
                }
            }
            return stringCollection.ToArray();
        }

        private string ReplaceUnprintable(string text, bool leaveCrLf = true)
        {
            var str = new StringBuilder();

            for (int i = 0; i < text.Length; i++)
            {
                var c = text[i];
                if (char.IsControl(c) && !(leaveCrLf && (c == '\r' || c == '\n')))
                {
                    str.Append("<" + Accessory.ConvertStringToHex(c.ToString()) + ">");
                    if (c == '\n') str.Append("\n");
                }
                else
                {
                    str.Append(c);
                }
            }

            return str.ToString();
        }

        public void Clear()
        {
            _logBuffer.Clear();
            selStart = 0;
            _unfinishedString.Clear();
            OnPropertyChanged();
        }

        public void Dispose()
        {
            TimerStop();
        }
    }
}