using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

using LiteDB;

namespace MqttBroker
{
    public class LiteDbLocal : ILocalDb
    {
        private readonly LiteDatabase _db;
        private readonly ILiteCollection<DeviceRecord> _sensors;

        public LiteDbLocal(string dbFileName, string collectionName)
        {
            _db = new LiteDatabase(dbFileName);
            _sensors = _db.GetCollection<DeviceRecord>(collectionName);
            _sensors.EnsureIndex(x => x.Time);
        }

        private bool Disposed { get; set; }

        public async Task<long> AddRecord(DeviceRecord record)
        {
            if (record == null) return -1;

            return _sensors.Insert(record);
        }

        public async Task<bool> RemoveRecord(long id)
        {
            return _sensors.Delete(id);
        }

        public async Task<bool> UpdateRecord(DeviceRecord record)
        {
            if (record == null) return false;

            return _sensors.Update(record);
        }

        public string GetDeviceMacByDeviceName(string deviceName)
        {
            var deviceMac = _sensors.FindOne(x => x.DeviceName.Equals(deviceName, StringComparison.Ordinal)).DeviceMac;

            return deviceMac;
        }

        public IEnumerable<string> GetDeviceNamesByDeviceMac(string deviceMac)
        {
            var deviceNames = _sensors.Find(x => x.DeviceMac == deviceMac).Select(x => x.DeviceName);

            return deviceNames;
        }

        public IEnumerable<long> GetIdList(string deviceName)
        {
            var results = _sensors.Find(x => deviceName.Equals(x.DeviceName, StringComparison.Ordinal));
            return results.Select(n => n.DeviceRecordId).ToList();
        }

        public DeviceRecord GetRecordById(long id)
        {
            var record = _sensors.FindById(id);

            return record;
        }

        public IEnumerable<DeviceRecord> GetRecordsByDevice(string deviceName)
        {
            var records = _sensors.Find(x => x.DeviceName.Equals(deviceName, StringComparison.Ordinal));

            return records;
        }

        public IEnumerable<DeviceRecord> GetRecordsRange(List<string> deviceNameList, DateTime startTime, DateTime endTime)
        {
            var records = _sensors.Find(
                x => deviceNameList.Contains(x.DeviceName)
                     && x.Time > startTime
                     && x.Time < endTime);

            return records;
        }

        public IEnumerable<DeviceRecord> GetRecordsRange(string deviceName, DateTime startTime, DateTime endTime)
        {
            var records = _sensors.Find(
                x => x.DeviceName == deviceName
                     && x.Time > startTime
                     && x.Time < endTime);

            return records;
        }

        public IEnumerable<DeviceRecord> GetRecordsRange(DateTime startTime, DateTime endTime)
        {
            var records = _sensors.Find(
                x => x.Time > startTime
                     && x.Time < endTime);
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
        private void Dispose(bool disposing)
        {
            if (Disposed) return;

            if (disposing)
                //db.Shrink();
                //db.Rebuild();
                _db.Dispose();
            Disposed = true;
        }
    }
}