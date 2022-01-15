using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace MqttBroker
{
    internal interface ILocalDb : IDisposable
    {
        Task<long> AddRecord(DeviceRecord record);

        Task<bool> RemoveRecord(long id);

        Task<bool> UpdateRecord(DeviceRecord record);

        string GetDeviceMacByDeviceName(string deviceName);

        IEnumerable<string> GetDeviceNamesByDeviceMac(string deviceMac);

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