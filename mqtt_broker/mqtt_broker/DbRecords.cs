using System;
using System.Collections.Generic;
using System.Runtime.Serialization;

namespace MqttBroker
{
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
        [DataMember] public long DeviceRecordId { get; set; }
        [DataMember] public string DeviceMac { get; set; }
        [DataMember] public string DeviceName { get; set; }
        [DataMember] public string FwVersion { get; set; }
        [DataMember] public DateTime Time { get; set; }
        [DataMember] public List<SensorRecord> SensorValueList { get; set; } = new List<SensorRecord>();
    }
}