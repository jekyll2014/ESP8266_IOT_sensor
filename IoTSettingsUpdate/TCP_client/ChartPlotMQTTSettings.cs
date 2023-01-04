// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
namespace IoTSettingsUpdate
{
    public class IoTSettingsUpdateSettings
    {
        //[JsonProperty(Required = Required.Always)]
        public int CodePage = 866;
        public string StringsDivider = "";
        public int ReplyTimeout = 5000;

        public string DefaultAddress = "192.168.202.123";
        public int DefaultPort = 23;

        public string DefaultComName = "";
        public int DefaultComSpeed = 115200;
    }
}
