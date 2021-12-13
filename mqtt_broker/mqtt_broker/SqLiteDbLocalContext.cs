using System;
using System.Collections.Generic;
using System.Linq;
using System.Data.SQLite;
using DbRecords;
using System.Data.Entity;

namespace SQLiteDb
{
    public class SqLiteDbLocalContext : DbContext, ILocalDb
    {
        private readonly SQLiteConnection _db;
        public DbSet<DeviceRecord> DeviceRecords { get; set; }

        public SqLiteDbLocalContext(string dbFileName, string collectionName) : base("DbConnection")
        {
            _db = new SQLiteConnection($"Data Source=c:\\{dbFileName}.sql");
            _db.Open();
            Database.CreateIfNotExists();
        }

        public long AddRecord(DeviceRecord record)
        {
            if (record == null) return -1;

            var result = DeviceRecords.Add(record).Id;
            SaveChanges();
            return result;
        }

        public bool RemoveRecord(long id)
        {
            var rec = DeviceRecords.First(n=>n.Id == id);

            if (rec != null)
            {
                DeviceRecords.Remove(rec);
                SaveChanges();

                return true;
            }

            return false;
        }

        public bool UpdateRecord(DeviceRecord record)
        {
            if (record == null) return false;

            var rec = DeviceRecords.First(n => n.Id == record.Id);

            if (rec != null)
            {
                rec.DeviceMAC = record.DeviceMAC;
                rec.DeviceName = record.DeviceName;
                rec.Time = record.Time;
                rec.SensorValueList = record.SensorValueList;
                SaveChanges();

                return true;
            }

            return false;
        }

        public byte[] GetDeviceMacByDeviceName(string deviceName)
        {
            var deviceMac = DeviceRecords.First(x => x.DeviceName.Equals(deviceName, StringComparison.Ordinal)).DeviceMAC;

            return deviceMac;
        }

        public IEnumerable<string> GetDeviceNamesByDeviceMac(byte[] deviceMac)
        {
            var deviceNames = DeviceRecords.Where(x => x.DeviceMAC == deviceMac).Select(x => x.DeviceName);

            return deviceNames;
        }

        public List<long> GetIdList(string deviceName)
        {
            var results = DeviceRecords.Where(x => deviceName.Equals(x.DeviceName, StringComparison.Ordinal)).Select(n => n.Id);
            return results.ToList();
        }

        public DeviceRecord GetRecordById(long id)
        {
            var record = DeviceRecords.First(n=>n.Id==id);

            return record;
        }

        public IEnumerable<DeviceRecord> GetRecordsByDevice(string deviceName)
        {
            var records = DeviceRecords.Where(x => x.DeviceName.Equals(deviceName, StringComparison.Ordinal));

            return records;
        }

        public IEnumerable<DeviceRecord> GetRecordsRange(List<string> deviceNameList, DateTime startTime, DateTime endTime)
        {
            var records = DeviceRecords.Where(
                x => deviceNameList.Contains(x.DeviceName)
                     && x.Time > startTime
                     && x.Time < endTime);

            return records;
        }

        public IEnumerable<DeviceRecord> GetRecordsRange(string deviceName, DateTime startTime, DateTime endTime)
        {
            var records = DeviceRecords.Where(
                x => x.DeviceName == deviceName
                     && x.Time > startTime
                     && x.Time < endTime);

            return records;
        }

        public IEnumerable<DeviceRecord> GetRecordsRange(DateTime startTime, DateTime endTime)
        {
            var records = DeviceRecords.Where(
                x => x.Time > startTime
                     && x.Time < endTime);

            return records;
        }

        public IEnumerable<string> GetDeviceList()
        {
            var results = DeviceRecords.Select(x => x.DeviceName).Distinct();
            return results;
        }

        public IEnumerable<string> GetDeviceList(DateTime startTime, DateTime endTime)
        {
            var results = DeviceRecords.Where(x => x.Time > startTime
                                                  && x.Time < endTime).Select(x => x.DeviceName).Distinct();
            return results;
        }
    }
}