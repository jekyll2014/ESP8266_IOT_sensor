using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

using System;
using System.Collections.Generic;
using System.IO;

namespace IoTSettingsUpdate
{
    public class CommandConfig<T> where T : class
    {
        private readonly string _configFileName;
        private List<T> _configStorage;

        public List<T> ConfigStorage
        {
            get => _configStorage ?? new List<T>();
            set => _configStorage = value;
        }

        public CommandConfig(string file)
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
                ConfigStorage = GetSection<List<T>>(json, "");
            }
            catch (Exception e)
            {
                //MessageBox.Show($"Error loading file: {e.Message}");

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
