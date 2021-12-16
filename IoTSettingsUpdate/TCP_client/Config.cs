using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

using System;
using System.Collections.Generic;
using System.IO;


namespace JsonDictionaryCore
{
    public class JsonDictionarySettings
    {
        public string Command { get; set; } = "";
        public string DefaultValue { get; set; } = "";
        public string CustomValue { get; set; } = "";
        public string OkReplyString { get; set; } = "";
        public string ErrorReplyString { get; set; } = "";
        public string ParameterName { get; set; } = "";
    }

    public class Config
    {
        private readonly string _configFileName;
        private List<JsonDictionarySettings> _configStorage;

        public List<JsonDictionarySettings> ConfigStorage
        {
            get => _configStorage ?? new List<JsonDictionarySettings>();
            set => _configStorage = value;
        }

        public Config(string file)
        {
            _configFileName = file;
            LoadConfig();
        }

        public bool LoadConfig(string fileName = null)
        {
            if (string.IsNullOrEmpty(fileName))
                fileName = _configFileName;

            try
            {
                var json = JArray.Parse(File.ReadAllText(fileName));
                ConfigStorage = GetSection<List<JsonDictionarySettings>>(json, "");
            }
            catch(Exception e)
            {
                return false;
            }

            return true;
        }

        public T GetSection<T>(JArray json, string sectionName = null) where T : class
        {
            if (string.IsNullOrEmpty(_configFileName))
                return default;

            if (string.IsNullOrEmpty(sectionName))
            {
                return json?.ToObject<T>() ??
                       throw new InvalidOperationException($"Cannot find section {sectionName}");
            }

            return json[sectionName]?.ToObject<T>() ??
                   throw new InvalidOperationException($"Cannot find section {sectionName}");
        }

        public bool SaveConfig(string fileName = null)
        {
            if (string.IsNullOrEmpty(_configFileName))
                fileName = _configFileName;

            try
            {
                File.WriteAllText(fileName,
                    JsonConvert.SerializeObject(ConfigStorage, Formatting.Indented));
            }
            catch
            {
                return false;
            }

            return true;
        }
    }
}
