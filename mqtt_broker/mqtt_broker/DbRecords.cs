using System;
using System.Collections.Generic;
using System.Runtime.Serialization;

namespace DbRecords
{
    [DataContract]
    public class SensorRecord
    {
        [DataMember] public long Id { get; set; }
        [DataMember] public string SensorName { get; set; }
        [DataMember] public float Value { get; set; }
    }

    [DataContract]
    public class DeviceRecord
    {
        [DataMember] public long Id { get; set; }
        [DataMember] public byte[] DeviceMAC { get; set; }
        [DataMember] public string DeviceName { get; set; }
        [DataMember] public DateTime Time { get; set; }
        [DataMember] public List<SensorRecord> SensorValueList { get; set; }
    }

    interface ILocalDb : IDisposable
    {
        long AddRecord(DeviceRecord record);

        bool RemoveRecord(long id);

        bool UpdateRecord(DeviceRecord record);

        byte[] GetDeviceMacByDeviceName(string deviceName);

        IEnumerable<string> GetDeviceNamesByDeviceMac(byte[] deviceMac);

        List<long> GetIdList(string deviceName);

        DeviceRecord GetRecordById(long id);

        IEnumerable<DeviceRecord> GetRecordsByDevice(string deviceName);

        IEnumerable<DeviceRecord> GetRecordsRange(List<string> deviceNameList, DateTime startTime, DateTime endTime);

        IEnumerable<DeviceRecord> GetRecordsRange(string deviceName, DateTime startTime, DateTime endTime);

        IEnumerable<DeviceRecord> GetRecordsRange(DateTime startTime, DateTime endTime);

        IEnumerable<string> GetDeviceList();

        IEnumerable<string> GetDeviceList(DateTime startTime, DateTime endTime);
    }
}