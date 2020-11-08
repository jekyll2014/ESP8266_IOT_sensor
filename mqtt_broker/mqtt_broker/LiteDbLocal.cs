using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;

using LiteDB;

namespace MqttBroker
{
    public class LiteDbLocal : IDisposable
    {
        private readonly LiteDatabase db;
        private readonly ILiteCollection<SensorDataRec> _sensors;

        [DataContract]
        public class ValueItemRec
        {
            [DataMember] public string ValueType { get; set; }
            [DataMember] public float Value { get; set; }
        }

        [DataContract]
        public class SensorDataRec
        {
            [DataMember] public int Id { get; set; }
            [DataMember] public string DeviceName { get; set; }
            [DataMember] public DateTime Time { get; set; }
            [DataMember] public List<ValueItemRec> ValueList { get; set; }
        }

        public LiteDbLocal(string dbFileName, string collectionName)
        {
            db = new LiteDatabase(dbFileName);
            _sensors = db.GetCollection<SensorDataRec>(collectionName);
            _sensors.EnsureIndex(x => x.Time);
        }

        private bool Disposed { get; set; }

        public int AddRecord(SensorDataRec record)
        {
            if (record == null) return -1;

            return _sensors.Insert(record);
        }

        public void RemoveRecord(int id)
        {
            _sensors.Delete(id);
        }

        public bool UpdateRecord(SensorDataRec record)
        {
            if (record == null) return false;

            return _sensors.Update(record);
        }

        public List<int> GetIdList(string deviceName)
        {
            var results = _sensors.Find(x => deviceName.Equals(x.DeviceName, StringComparison.Ordinal));
            return results.Select(n => n.Id).ToList();
        }

        public SensorDataRec GetRecordById(int id)
        {
            var record = _sensors.FindById(id);

            return record;
        }

        public IEnumerable<SensorDataRec> GetRecordsByDevice(string deviceName)
        {
            var records = _sensors.Find(x => x.DeviceName.Equals(deviceName, StringComparison.Ordinal));

            return records;
        }

        public IEnumerable<SensorDataRec> GetRecordsRange(List<string> deviceNameList, DateTime startTime, DateTime endTime)
        {
            var records = _sensors.Find(
                x => deviceNameList.Contains(x.DeviceName)
                     && x.Time > startTime
                     && x.Time < endTime);

            return records;
        }

        public IEnumerable<SensorDataRec> GetRecordsRange(string deviceName, DateTime startTime, DateTime endTime)
        {
            var records = _sensors.Find(
                x => x.DeviceName == deviceName
                     && x.Time > startTime
                     && x.Time < endTime);

            return records;
        }

        public IEnumerable<SensorDataRec> GetRecordsRange(DateTime startTime, DateTime endTime)
        {
            var records = _sensors.Find(
                x => x.Time > startTime
                     && x.Time < endTime);
            if (records == null || !records.Any()) return null;

            return records;
        }

        public IEnumerable<string> GetDeviceList()
        {
            var results = _sensors.FindAll().Select(x => x.DeviceName).Distinct();
            return results;
        }

        public IEnumerable<string> GetDeviceList(DateTime startTime, DateTime endTime)
        {
            var results = _sensors.Find(x => x.Time > startTime
                                            && x.Time < endTime).Select(x => x.DeviceName).Distinct();
            return results;
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
        protected void Dispose(bool disposing)
        {
            if (Disposed) return;

            if (disposing)
                //db.Shrink();
                //db.Rebuild();
                db.Dispose();
            Disposed = true;
        }
    }
}