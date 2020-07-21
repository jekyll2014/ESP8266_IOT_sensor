using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace ChartPlotMQTT
{
    public class TextLogger
    {
        // ring buffer for strings
        // string complete timeout
        // string length limit
        // add suffixes for channels (in/out/error/...)
        // HEX/text format flag
        public bool noScreenOutput = false;
        public int LinesLimit = 500;
        public string LogFileName = "";
        public bool AutoSave = false;
        public bool AutoScroll = true;
        public bool FilterZeroChar = true;
        public Format DefaultFormat = Format.PlainText; //Tex5t, HEX, Auto (change non-readable to <HEX>)

        private delegate void SetTextCallback(string text);

        private TextBox _textBox;
        private List<string> _logBuffer = new List<string>();
        private StringBuilder unfinishedString = new StringBuilder();

        private List<string> channels = new List<string> { "" };
        private readonly object textOutThreadLock = new object();

        public TextLogger(TextBox textBox)
        {
            _textBox = textBox;
        }

        public enum Format
        {
            Default,
            PlainText,
            Hex,
            AutoReplaceHex
        }

        public bool AddText(string text, byte channel = 0, Format textFormat = Format.Default)
        {
            return AddText(text, DateTime.MinValue, channel, textFormat);
        }

        public bool AddText(string text, DateTime logTime, byte channel = 0, Format textFormat = Format.Default)
        {
            var tmpStr = new StringBuilder();
            if (logTime != DateTime.MinValue)
            {
                tmpStr.Append(logTime.ToShortTimeString() + logTime.Millisecond.ToString("D3") + " ");
            }

            if (channel >= channels.Count)
            {
                channel = 0;
            }

            if (channel > 0 && !string.IsNullOrEmpty(channels[channel]))
            {
                tmpStr.Append(channels[channel - 1] + " ");
            }

            if (tmpStr[tmpStr.Length - 1] == ' ')
            {
                tmpStr.Remove(tmpStr.Length - 1, 1);
            }

            tmpStr.Append(": ");

            if (textFormat == Format.Default)
            {
                textFormat = DefaultFormat;
            }

            if (textFormat == Format.PlainText)
            {
                tmpStr.Append(text);
            }
            else if (textFormat == Format.Hex)
            {
                tmpStr.Append(Accessory.ConvertStringToHex(text));
            }
            else if (textFormat == Format.AutoReplaceHex)
            {
                tmpStr.Append(replaceUnprintable(text));
            }

            string[] inputStrings = convertTextToStringArray(text, ref unfinishedString);
            return AddTextToBuffer(inputStrings);
        }

        private bool AddTextToBuffer(string[] text)
        {
            if (text == null || text.Length <= 0)
            {
                return false;
            }
            lock (textOutThreadLock)
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
                        File.AppendAllText(LogFileName, text[i]);
                    }
                }

                if (noScreenOutput)
                {
                    return false;
                }

                var pos = _textBox.SelectionStart;
                _logBuffer.AddRange(text);
                if (_logBuffer.Count >= LinesLimit)
                {
                    while (_logBuffer.Count >= LinesLimit)
                    {
                        _logBuffer.RemoveAt(0);
                    }
                }

                if (AutoScroll)
                {
                    _textBox.SelectionStart = _textBox.Text.Length;
                    _textBox.ScrollToCaret();
                }
                else
                {
                    _textBox.SelectionStart = pos;
                    _textBox.ScrollToCaret();
                }
            }

            return true;
        }

        public override string ToString()
        {
            var tmpTxt = new StringBuilder();
            foreach (var str in _logBuffer)
            {
                tmpTxt.AppendLine(str);
            }

            tmpTxt.AppendLine(unfinishedString.ToString());

            return tmpTxt.ToString();
        }

        private string[] convertTextToStringArray(string data, ref StringBuilder nonComplete)
        {
            var divider = new HashSet<char>();
            divider.Add('\r');
            divider.Add('\n');

            var stringCollection = new List<string>();

            var prevChar = ' ';
            foreach (var t in data)
            {
                if (divider.Contains(t))
                {
                    if ((t == '\r' && prevChar == '\n') || (t == '\n' && prevChar == '\r'))
                    {
                        break;
                    }

                    stringCollection.Add(nonComplete.ToString());
                    nonComplete.Clear();
                }
                else
                {
                    nonComplete.Append(t);
                }
            }
            return stringCollection.ToArray();
        }

        private string replaceUnprintable(string text, bool leaveCrLf=true)
        {
            var str = new StringBuilder();

            foreach (var c in text)
            {
                if (char.IsControl(c))
                {
                    str.Append("<" + Accessory.ConvertStringToHex(c.ToString()) + ">");
                }
                else
                {
                    str.Append(c);
                }
            }

            return str.ToString();
        }

    }
}
