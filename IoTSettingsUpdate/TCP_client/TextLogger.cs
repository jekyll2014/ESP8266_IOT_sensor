using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Text;
using System.Threading;
using System.Windows.Forms;

using Timer = System.Windows.Forms.Timer;

namespace TextLoggerHelper
{
    public class TextLogger : IDisposable, INotifyPropertyChanged
    {
        // ring buffer for strings
        // string length limit

        public enum TextFormat
        {
            Default,
            PlainText,
            Hex,
            AutoReplaceHex
        }

        public enum TimeFormat
        {
            None,
            Default,
            ShortTime,
            LongTime
        }

        public enum DateFormat
        {
            None,
            Default,
            ShortDate,
            LongDate,
        }

        private readonly object _textOutThreadLock = new object();

        public bool NoScreenOutput = false;
        public int LineLimit = 0;
        public int CharLimit = 0;
        public int LineTimeLimit = 100;
        public string LogFileName = "";
        public bool AutoSave = false;
        public bool AutoScroll = true;
        public bool FilterZeroChar = true;
        public TextFormat DefaultTextFormat = TextFormat.AutoReplaceHex; //Text, HEX, Auto (change non-readable to <HEX>)
        public TimeFormat DefaultTimeFormat = TimeFormat.LongTime;
        public DateFormat DefaultDateFormat = DateFormat.ShortDate;
        public Dictionary<byte, string> Channels = new Dictionary<byte, string>();
        public event PropertyChangedEventHandler PropertyChanged;

        private Form _mainForm;
        private TextBox _textBox;
        private int _selStart, _selLength;
        private volatile bool _textChanged;
        private Timer _refreshTimer;
        private byte _prevChannel;
        private DateTime _lastEvent = DateTime.Now;

        protected void OnPropertyChanged()
        {
            _textChanged = true;
            _mainForm?.Invoke((MethodInvoker)delegate
           {
               PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Text"));
           });
        }

        public string Text { get; private set; } = "";

        public TextLogger(Form mainForm, TextBox textBox = null)
        {
            this._mainForm = mainForm;
            this._textBox = textBox;
        }

        public void RefreshStart(int delay)
        {
            if (_mainForm != null && _textBox != null)
            {
                _refreshTimer = new Timer();
                _refreshTimer.Tick += RefreshTimerTick;
                _refreshTimer.Interval = delay;
                _refreshTimer.Start();
            }
        }

        public void RefreshStop()
        {
            if (_refreshTimer == null) return;

            _refreshTimer.Tick -= RefreshTimerTick;
            _refreshTimer?.Stop();
            _refreshTimer?.Dispose();
        }

        public bool AddText(string text, byte channel)
        {
            return AddText(text, channel, DateTime.MinValue, TextFormat.Default, TimeFormat.Default, DateFormat.Default);
        }

        public bool AddText(string text, byte channel, DateTime eventTime)
        {
            return AddText(text, channel, eventTime, TextFormat.Default, TimeFormat.Default, DateFormat.Default);
        }

        public bool AddText(string text, byte channel, DateTime eventTime, TimeFormat timeFormat)
        {
            return AddText(text, channel, eventTime, TextFormat.Default, timeFormat, DateFormat.Default);
        }

        public bool AddText(string text, byte channel, DateTime eventTime, DateFormat dateFormat)
        {
            return AddText(text, channel, eventTime, TextFormat.Default, TimeFormat.Default, dateFormat);
        }

        public bool AddText(string text, byte channel, DateTime eventTime, TextFormat textTextFormat, TimeFormat timeFormat)
        {
            return AddText(text, channel, eventTime, textTextFormat, timeFormat, DateFormat.Default);
        }

        public bool AddText(string text, byte channel, DateTime eventTime, TextFormat textTextFormat, DateFormat dateFormat)
        {
            return AddText(text, channel, eventTime, textTextFormat, TimeFormat.Default, dateFormat);
        }

        public bool AddText(string text, byte channel, DateTime eventTime, TimeFormat timeFormat, DateFormat dateFormat)
        {
            return AddText(text, channel, eventTime, TextFormat.Default, timeFormat, dateFormat);
        }

        public bool AddText(string text, byte channel, TextFormat textTextFormat)
        {
            return AddText(text, channel, DateTime.MinValue, textTextFormat, TimeFormat.Default, DateFormat.Default);
        }

        public bool AddText(string text, byte channel, TimeFormat timeFormat)
        {
            return AddText(text, channel, DateTime.MinValue, TextFormat.Default, timeFormat, DateFormat.Default);
        }

        public bool AddText(string text, byte channel, DateFormat dateFormat)
        {
            return AddText(text, channel, DateTime.MinValue, TextFormat.Default, TimeFormat.Default, dateFormat);
        }

        public bool AddText(string text, byte channel, TextFormat textTextFormat, TimeFormat timeFormat)
        {
            return AddText(text, channel, DateTime.MinValue, textTextFormat, timeFormat, DateFormat.Default);
        }

        public bool AddText(string text, byte channel, TextFormat textTextFormat, DateFormat dateFormat)
        {
            return AddText(text, channel, DateTime.MinValue, textTextFormat, TimeFormat.Default, dateFormat);
        }

        public bool AddText(string text, byte channel, TimeFormat timeFormat, DateFormat dateFormat)
        {
            return AddText(text, channel, DateTime.MinValue, TextFormat.Default, timeFormat, dateFormat);
        }

        public bool AddText(string text, byte channel, DateTime logTime, TextFormat textFormat,
            TimeFormat timeFormat = TimeFormat.Default, DateFormat dateFormat = DateFormat.Default)
        {
            if (text == null || text.Length <= 0) return true;

            var result = false;
            LockWithTimeout(_textOutThreadLock, () =>
            {
                var continueString = false;
                if (channel != _prevChannel)
                {
                    _prevChannel = channel;
                }
                else if (LineTimeLimit > 0)
                {
                    var t = (int)logTime.Subtract(_lastEvent).TotalMilliseconds;
                    if (t <= LineTimeLimit)
                        continueString = true;
                }

                _lastEvent = logTime;
                var tmpStr = new StringBuilder();
                if (!continueString)
                {
                    tmpStr.Append(Environment.NewLine);
                    if (logTime != DateTime.MinValue)
                    {
                        if (dateFormat == DateFormat.Default)
                            dateFormat = DefaultDateFormat;

                        if (dateFormat == DateFormat.LongDate)
                            tmpStr.Append(logTime.ToLongDateString() + " ");
                        else if (dateFormat == DateFormat.ShortDate)
                            tmpStr.Append(logTime.ToShortDateString() + " ");

                        if (timeFormat == TimeFormat.Default)
                            timeFormat = DefaultTimeFormat;

                        if (timeFormat == TimeFormat.LongTime)
                            tmpStr.Append(logTime.ToLongTimeString() + "." + logTime.Millisecond.ToString("D3") + " ");

                        else if (timeFormat == TimeFormat.ShortTime)
                            tmpStr.Append(logTime.ToShortTimeString() + " ");
                    }

                    if (Channels.ContainsKey(channel))
                    {
                        if (!string.IsNullOrEmpty(Channels[channel])) tmpStr.Append(Channels[channel] + " ");
                    }
                }

                if (textFormat == TextFormat.Default)
                    textFormat = DefaultTextFormat;

                if (FilterZeroChar)
                    text = Accessory.FilterZeroChar(text);

                if (textFormat == TextFormat.PlainText)
                {
                    tmpStr.Append(text);
                }
                else if (textFormat == TextFormat.Hex)
                {
                    tmpStr.Append(Accessory.ConvertStringToHex(text));
                }
                else if (textFormat == TextFormat.AutoReplaceHex)
                {
                    tmpStr.Append(ReplaceUnprintable(text));
                }

                result = AddTextToBuffer(tmpStr.ToString());
            }, 5000);

            return result;
        }

        public override string ToString()
        {
            return Text;
        }

        public void Clear()
        {
            Text = "";
            _selStart = 0;
            OnPropertyChanged();
        }

        private bool _disposed = false;

        ~TextLogger() => Dispose(false);

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
        protected virtual void Dispose(bool disposing)
        {
            if (_disposed)
            {
                return;
            }

            if (disposing)
            {
                RefreshStop();
            }

            Text = null;
            _mainForm = null;
            _textBox = null;
            _disposed = true;
        }

        private bool AddTextToBuffer(string text)
        {
            if (text == null || text.Length <= 0) return false;
            if (AutoSave && !string.IsNullOrEmpty(LogFileName))
            {
                try
                {
                    File.AppendAllText(LogFileName, text);
                }
                catch (Exception e)
                {
                    MessageBox.Show(e.Message);
                }
            }

            if (!NoScreenOutput)
            {
                Text += text;

                var textSizeReduced = 0;
                if (CharLimit > 0 && Text.Length > CharLimit)
                {
                    textSizeReduced = Text.Length - CharLimit;
                }

                if (LineLimit > 0)
                {
                    if (GetLinesCount(Text, LineLimit, out var pos))
                    {
                        if (pos > textSizeReduced)
                            textSizeReduced = pos;
                    }
                }

                if (textSizeReduced > 0)
                {
                    Text = Text.Substring(textSizeReduced);
                }

                if (_textBox != null && !AutoScroll)
                {
                    _mainForm?.Invoke((MethodInvoker)delegate
                   {
                       _selStart = _textBox.SelectionStart;
                       _selLength = _textBox.SelectionLength;
                   });
                    _selStart -= textSizeReduced;
                    if (_selStart < 0)
                    {
                        _selLength += _selStart;
                        _selStart = 0;
                        if (_selLength < 0) _selLength = 0;
                    }
                }

                OnPropertyChanged();
            }

            return true;
        }

        private void UpdateDisplay()
        {
            if (_textBox != null && _textChanged)
                _mainForm?.Invoke((MethodInvoker)delegate
               {
                   _textBox.Text = Text;
                   if (AutoScroll)
                   {
                       _textBox.SelectionStart = _textBox.Text.Length;
                       _textBox.ScrollToCaret();
                   }
                   else
                   {
                       _textBox.SelectionStart = _selStart;
                       _textBox.SelectionLength = _selLength;
                       _textBox.ScrollToCaret();
                   }

                   _textChanged = false;
               });
        }

        private static bool GetLinesCount(string data, int lineLimit, out int pos)
        {
            var divider = new HashSet<char>
            {
                '\r',
                '\n'
            };

            var lineCount = 0;
            pos = 0;
            for (var i = data.Length - 1; i >= 0; i--)
            {
                if (divider.Contains(data[i])) // check 2 divider 
                {
                    lineCount++;
                    if (i - 1 >= 0 && divider.Contains(data[i - 1])) i--;
                }

                if (lineCount >= lineLimit)
                {
                    pos = i + 1;
                    return true;
                }
            }
            return false;
        }

        private static string ReplaceUnprintable(string text, bool leaveCrLf = true)
        {
            var str = new StringBuilder();

            for (var i = 0; i < text.Length; i++)
            {
                var c = text[i];
                if (char.IsControl(c) && !(leaveCrLf && (c == '\r' || c == '\n' || c == '\t')))
                {
                    str.Append("<0x" + Accessory.ConvertStringToHex(c.ToString()).Trim() + ">");
                    if (c == '\n') str.Append("\n");
                }
                else
                {
                    str.Append(c);
                }
            }

            return str.ToString();
        }

        private void RefreshTimerTick(object sender, EventArgs e)
        {
            UpdateDisplay();
        }

        public static void LockWithTimeout(object lockObj, Action doAction, int millisecondsTimeout = 15000)
        {
            bool lockWasTaken = false; var temp = lockObj;
            try
            {
                Monitor.TryEnter(temp, millisecondsTimeout, ref lockWasTaken);
                if (lockWasTaken)
                {
                    doAction();
                }
                else
                {
                    //throw new Exception("Could not get lock");
                    lockObj = new object();
                }
            }
            finally { if (lockWasTaken) { Monitor.Exit(temp); } }
        }

    }
}