using System;
using System.Collections.Generic;
using System.Runtime.Serialization;

namespace ChartPlotMQTT
{
    [DataContract]
    public class ValueItem
    {
        [DataMember] public string ValueType = "";
        [DataMember] public float Value;
    }

    [DataContract]
    public class SensorData
    {
        [DataMember] public int Id;
        [DataMember] public string DeviceName = "";
        [DataMember] public DateTime Time;
        [DataMember] public List<ValueItem> ValueList = new List<ValueItem>();
    }
}
