using System.Collections.Generic;

namespace IoTSettingsUpdate
{
    public class CommandSettings
    {
        public List<CommandSetting> Commands { get; set; } = new List<CommandSetting>();
    }

    public class CommandSetting
    {
        public string Command { get; set; } = "";
        public string DefaultValue { get; set; } = "";
        public string CustomValue { get; set; } = "";
        public string ErrorReplyString { get; set; } = "";
        public string ParameterName { get; set; } = "";
        public string ParameterDescription { get; set; } = "";
    }
}
