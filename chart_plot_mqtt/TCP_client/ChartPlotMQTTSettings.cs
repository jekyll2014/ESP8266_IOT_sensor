// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
namespace ChartPlotMQTT
{
    public class ChartPlotMQTTSettings
    {
        //[JsonProperty(Required = Required.Always)]
        public string DefaultAddress = "192.168.202.10";
        public int DefaultPort = 1883;
        public int CodePage = 866;
        public string DefaultSubscribeTopic = "sensors/#";
        public string DefaultPublishTopic = "commands/#";
        public bool AutoConnect = false;
        public bool AutoReConnect = false;
        public string DefaultLogin = "";
        public string DefaultPassword = "";
        public string ClientID = "ChartPlotter";
        public string DbFileName = "localDb.db";
        public bool AddTimeStamp = true;
        public int KeepTime = 60;
        public string DbPassword = "";
        public int RESTPort = 8080;
        public bool keepLocalDb = false;
    }
}
