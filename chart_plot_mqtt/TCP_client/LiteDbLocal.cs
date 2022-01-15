using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;

using LiteDB;

namespace ChartPlotMQTT
{
    public class LiteDbLocal : IDisposable
    {
        private readonly LiteDatabase _db;
        private readonly ILiteCollection<DeviceRecord> _sensors;

        [DataContract]
        public class SensorRecord
        {
            [DataMember] public long SensorRecordId { get; set; }
            [DataMember] public string SensorName { get; set; }
            [DataMember] public float Value { get; set; }
            public long? DeviceRecordId { get; set; }
        }

        [DataContract]
        public class DeviceRecord
        {
            [DataMember] public int DeviceRecordId { get; set; }
            [DataMember] public string DeviceMac { get; set; }
            [DataMember] public string DeviceName { get; set; }
            [DataMember] public string FwVersion { get; set; }
            [DataMember] public DateTime Time { get; set; }
            [DataMember] public List<SensorRecord> SensorValueList { get; set; } = new List<SensorRecord>();
        }

        public LiteDbLocal(string dbFileName, string collectionName)
        {
            _db = new LiteDatabase(dbFileName);
            _sensors = _db.GetCollection<DeviceRecord>(collectionName);
            _sensors.EnsureIndex(x => x.Time);
        }

        public LiteDbLocal(ConnectionString dbConnectionString, string collectionName)
        {
            _db = new LiteDatabase(dbConnectionString);
            _sensors = _db.GetCollection<DeviceRecord>(collectionName);
            _sensors.EnsureIndex(x => x.Time);
        }

        public bool Disposed { get; private set; }

        public int AddRecord(DeviceRecord record)
        {
            if (record == null) return -1;

            try
            {
                return _sensors.Insert(record);
            }
            catch (Exception e)
            {
                Console.WriteLine(e);
                throw;
            }
        }

        public void RemoveRecord(int id)
        {
            _sensors.Delete(id);
        }

        public bool UpdateRecord(DeviceRecord record)
        {
            if (record == null) return false;

            return _sensors.Update(record);
        }

        public List<int> GetRecordsIdList(string deviceName)
        {
            var list = new List<int>();
            if (_sensors.Count() > 0)
            {
                var results = _sensors.Find(x => deviceName.Equals(x.DeviceName, StringComparison.Ordinal));
                foreach (var n in results) list.Add(n.DeviceRecordId);
            }
            return list;
        }

        public DeviceRecord GetRecordById(int id)
        {
            var record = _sensors.FindById(id);

            return record;
        }

        public List<DeviceRecord> GetRecordsByDevice(string deviceName)
        {
            var records = _sensors.Find(x => x.DeviceName.Equals(deviceName, StringComparison.Ordinal)).ToList();

            return records;
        }

        public List<DeviceRecord> GetRecordsRange(List<string> deviceNameList, DateTime startTime, DateTime endTime)
        {
            var records = _sensors.Find(
                x => deviceNameList.Contains(x.DeviceName)
                     && x.Time > startTime
                     && x.Time < endTime).ToList();

            return records;
        }

        public List<DeviceRecord> GetRecordsRange(string deviceName, DateTime startTime, DateTime endTime)
        {
            var records = _sensors.Find(
                x => x.DeviceName == deviceName
                     && x.Time > startTime
                     && x.Time < endTime).ToList();

            return records;
        }

        public List<DeviceRecord> GetRecordsRange(DateTime startTime, DateTime endTime)
        {
            var records = _sensors.Find(
                x => x.Time > startTime
                     && x.Time < endTime).ToList();
            if (records == null || !records.Any()) return null;

            return records;
        }

        public List<string> GetDeviceList()
        {
            var list = new List<string>();
            if (_sensors.Count() <= 0) return list;

            var results = _sensors.Find(x => true).Distinct();
            foreach (var n in results)
                if (!list.Contains(n.DeviceName)) list.Add(n.DeviceName);
            return list;
        }

        // Public implementation of Dispose pattern callable by consumers.
        public void Dispose()
        {
            // Dispose of unmanaged resources.
            Dispose(true);
            // Suppress finalization.
            GC.SuppressFinalize(this);
        }

        // Protected implementation of Dispose pattern.
        private void Dispose(bool disposing)
        {
            if (Disposed) return;

            if (disposing)
            {
                //db.Shrink();
                //db.Rebuild();
                _db.Dispose();
                Disposed = true;
            }
        }
    }
}