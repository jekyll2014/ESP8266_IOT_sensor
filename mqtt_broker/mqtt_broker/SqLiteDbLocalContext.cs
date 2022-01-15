using System;
using System.Collections.Generic;
using System.Linq;
using System.Data.Entity;
using System.Data.SQLite;
using System.IO;
using System.Threading.Tasks;

namespace MqttBroker
{
    public class SqLiteDbLocalContext : DbContext, ILocalDb
    {
        public virtual DbSet<SensorRecord> SensorRecord { get; set; }
        public virtual DbSet<DeviceRecord> DeviceRecords { get; set; }

        public SqLiteDbLocalContext(string dbFileName) : base(
            new SQLiteConnection()
            {
                ConnectionString =
                    new SQLiteConnectionStringBuilder()
                    { DataSource = dbFileName, ForeignKeys = true }
                        .ConnectionString
            }, true)
        {
            if (!File.Exists(dbFileName))
            {
                Database.CreateIfNotExists();
                Database.ExecuteSqlCommand("CREATE TABLE \"DeviceRecords\" (" +
                                           "\"DeviceRecordId\"	INTEGER UNIQUE," +
                                           "\"DeviceMac\"	TEXT," +
                                           "\"DeviceName\"	TEXT," +
                                           "\"FwVersion\"	TEXT," +
                                           "\"Time\"	TEXT," +
                                           "\"SensorValueList\"	INTEGER," +
                                           "PRIMARY KEY(\"DeviceRecordId\" AUTOINCREMENT))");
                Database.ExecuteSqlCommand("CREATE TABLE \"SensorRecords\" (" +
                                                   "\"SensorRecordId\"INTEGER NOT NULL UNIQUE," +
                                                   "\"SensorName\"TEXT," +
                                                   "\"Value\"INTEGER," +
                                                   "\"DeviceRecordId\"INTEGER," +
                                                   "PRIMARY KEY(\"SensorRecordId\" AUTOINCREMENT))");
                SaveChanges();
            }
        }

        public async Task<long> AddRecord(DeviceRecord record)
        {
            if (record == null) return -1;

            var result = DeviceRecords.Add(record).DeviceRecordId;
            await SaveChangesAsync();
            return result;
        }

        public async Task<bool> RemoveRecord(long id)
        {
            var rec = DeviceRecords.First(n => n.DeviceRecordId == id);

            if (rec != null)
            {
                DeviceRecords.Remove(rec);
                await SaveChangesAsync();

                return true;
            }

            return false;
        }

        public async Task<bool> UpdateRecord(DeviceRecord record)
        {
            if (record == null) return false;

            var rec = DeviceRecords.First(n => n.DeviceRecordId == record.DeviceRecordId);

            if (rec != null)
            {
                rec.DeviceMac = record.DeviceMac;
                rec.DeviceName = record.DeviceName;
                rec.Time = record.Time;
                rec.SensorValueList = record.SensorValueList;
                await SaveChangesAsync();

                return true;
            }

            return false;
        }

        public string GetDeviceMacByDeviceName(string deviceName)
        {
            var deviceMac = DeviceRecords.Include(n => n.SensorValueList).First(x => x.DeviceName.Equals(deviceName, StringComparison.Ordinal)).DeviceMac;

            return deviceMac;
        }

        public IEnumerable<string> GetDeviceNamesByDeviceMac(string deviceMac)
        {
            var deviceNames = DeviceRecords.Include(n => n.SensorValueList).Where(x => x.DeviceMac == deviceMac).Select(x => x.DeviceName);

            return deviceNames;
        }

        public List<long> GetIdList(string deviceName)
        {
            var results = DeviceRecords.Include(n => n.SensorValueList).Where(x => deviceName.Equals(x.DeviceName, StringComparison.Ordinal)).Select(n => n.DeviceRecordId);
            return results.ToList();
        }

        public DeviceRecord GetRecordById(long id)
        {
            var record = DeviceRecords.Include(n => n.SensorValueList).First(n => n.DeviceRecordId == id);

            return record;
        }

        public IEnumerable<DeviceRecord> GetRecordsByDevice(string deviceName)
        {
            var records = DeviceRecords.Include(n => n.SensorValueList).Where(x => x.DeviceName.Equals(deviceName, StringComparison.Ordinal));

            return records;
        }

        public IEnumerable<DeviceRecord> GetRecordsRange(List<string> deviceNameList, DateTime startTime, DateTime endTime)
        {
            var records = DeviceRecords.Include(n => n.SensorValueList).Where(
                x => deviceNameList.Contains(x.DeviceName)
                     && x.Time > startTime
                     && x.Time < endTime);

            return records;
        }

        public IEnumerable<DeviceRecord> GetRecordsRange(string deviceName, DateTime startTime, DateTime endTime)
        {
            var records = DeviceRecords.Include(n => n.SensorValueList).Where(
                x => x.DeviceName == deviceName
                     && x.Time > startTime
                     && x.Time < endTime);

            return records;
        }

        public IEnumerable<DeviceRecord> GetRecordsRange(DateTime startTime, DateTime endTime)
        {
            var records = DeviceRecords.Include(n => n.SensorValueList).Where(
                x => x.Time > startTime
                     && x.Time < endTime);

            return records;
        }

        public IEnumerable<string> GetDeviceList()
        {
            var results = DeviceRecords.Include(n => n.SensorValueList).Select(x => x.DeviceName).Distinct();
            return results;
        }

        public IEnumerable<string> GetDeviceList(DateTime startTime, DateTime endTime)
        {
            var results = DeviceRecords.Include(n => n.SensorValueList).Where(x => x.Time > startTime
                                                    && x.Time < endTime).Select(x => x.DeviceName).Distinct();
            return results;
        }
    }
}