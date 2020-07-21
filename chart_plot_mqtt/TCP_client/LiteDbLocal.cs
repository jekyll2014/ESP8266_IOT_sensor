using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;

using LiteDB;

namespace ChartPlotMQTT
{
    public class LiteDbLocal : IDisposable
    {
        private readonly LiteDatabase db;
        private readonly ILiteCollection<SensorDataRec> sensors;

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
            sensors = db.GetCollection<SensorDataRec>(collectionName);
            sensors.EnsureIndex(x => x.Time);
        }

        public bool Disposed { get; private set; }

        public int AddRecord(SensorDataRec record)
        {
            if (record == null) return -1;

            try
            {
                return sensors.Insert(record);
            }
            catch (Exception e)
            {
                Console.WriteLine(e);
                throw;
            }
        }

        public void RemoveRecord(int id)
        {
            sensors.Delete(id);
        }

        public bool UpdateRecord(SensorDataRec record)
        {
            if (record == null) return false;

            return sensors.Update(record);
        }

        public List<int> GetRecordsIdList(string deviceName)
        {
            var list = new List<int>();
            if (sensors.Count() > 0)
            {
                var results = sensors.Find(x => deviceName.Equals(x.DeviceName, StringComparison.Ordinal));
                foreach (var n in results) list.Add(n.Id);
            }
            return list;
        }

        public SensorDataRec GetRecordById(int id)
        {
            var record = sensors.FindById(id);

            return record;
        }

        public IEnumerable<SensorDataRec> GetRecordsByDevice(string deviceName)
        {
            var records = sensors.Find(x => x.DeviceName.Equals(deviceName, StringComparison.Ordinal));

            return records;
        }

        public IEnumerable<SensorDataRec> GetRecordsRange(List<string> deviceNameList, DateTime startTime, DateTime endTime)
        {
            var records = sensors.Find(
                x => deviceNameList.Contains(x.DeviceName)
                     && x.Time > startTime
                     && x.Time < endTime);

            return records;
        }

        public IEnumerable<SensorDataRec> GetRecordsRange(string deviceName, DateTime startTime, DateTime endTime)
        {
            var records = sensors.Find(
                x => x.DeviceName == deviceName
                     && x.Time > startTime
                     && x.Time < endTime);

            return records;
        }

        public IEnumerable<SensorDataRec> GetRecordsRange(DateTime startTime, DateTime endTime)
        {
            var records = sensors.Find(
                x => x.Time > startTime
                     && x.Time < endTime);
            if (records == null || records.Count() <= 0) return null;

            return records;
        }

        public List<string> GetDeviceList()
        {
            var list = new List<string>();
            if (sensors.Count() <= 0) return list;

            var results = sensors.Find(x => true);
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
            // GC.SuppressFinalize(this);
        }

        // Protected implementation of Dispose pattern.
        private void Dispose(bool disposing)
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