// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
namespace MqttBroker
{
    public class MqttBrokerSettings
    {
        //[JsonProperty(Required = Required.Always)]
        public int MQTTPort = 1883;
        public string User = "";
        public string Password = "";
        public string ClientID = "PlotterServer";
        public string DbFileName = "localDb1";
        public string RESTHost = "http://localhost";
        public int RESTPort = 8080;
        public bool VerboseLog = true;
        public bool UseLiteDb = true;
        public string DeviceNameProperty = "DeviceName";
        public string DeviceMacProperty = "DeviceMAC";
        public string FwVersionProperty = "FW Version";
    }
}
