using System;
using System.Collections.Generic;
using System.Web.Http;

namespace MqttBroker
{
    public class SensorsController : ApiController
    {
        [Route("api/Sensors")]
        public IEnumerable<string> Get()
        {
            return Program.RecordsDb.GetDeviceList();
        }

        [HttpGet]
        [ActionName("GetDeviceList")]
        public IEnumerable<string> GetDeviceList()
        {
            return Program.RecordsDb.GetDeviceList();
        }

        [HttpGet]
        [ActionName("GetDeviceList")]
        public IEnumerable<string> GetDeviceListMinutes(int minutes)
        {
            return Program.RecordsDb.GetDeviceList(DateTime.Now.AddMinutes(-minutes), DateTime.Now);
        }

        [HttpGet]
        [ActionName("GetDeviceList")]
        public IEnumerable<string> GetDeviceListDays(int days)
        {
            return Program.RecordsDb.GetDeviceList(DateTime.Now.AddDays(-days), DateTime.Now);
        }

        [HttpGet]
        [ActionName("GetDeviceList")]
        public IEnumerable<string> GetDeviceList(DateTime startTime, DateTime endTime)
        {
            return Program.RecordsDb.GetDeviceList(startTime, endTime);
        }

        // GET api/<controller>/<action>
        [HttpGet]
        [ActionName("GetLatestRecords")]
        public IEnumerable<LiteDbLocal.SensorDataRec> GetLatestRecordsMinutes(int minutes)
        {
            return Program.RecordsDb.GetRecordsRange(DateTime.Now.AddMinutes(-minutes), DateTime.Now);
        }

        [HttpGet]
        [ActionName("GetLatestRecords")]
        public IEnumerable<LiteDbLocal.SensorDataRec> GetLatestRecordsDays(int days)
        {
            return Program.RecordsDb.GetRecordsRange(DateTime.Now.AddDays(-days), DateTime.Now);
        }

        [HttpGet]
        [ActionName("GetLatestRecords")]
        public IEnumerable<LiteDbLocal.SensorDataRec> GetLatestRecords(string deviceName, int minutes)
        {
            return Program.RecordsDb.GetRecordsRange(deviceName, DateTime.Now.AddMinutes(-minutes), DateTime.Now);
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public IEnumerable<LiteDbLocal.SensorDataRec> GetRecords(string deviceName)
        {
            return Program.RecordsDb.GetRecordsByDevice(deviceName);
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public IEnumerable<LiteDbLocal.SensorDataRec> GetRecords(string deviceName, DateTime startTime,
            DateTime endTime)
        {
            return Program.RecordsDb.GetRecordsRange(deviceName, startTime, endTime);
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public IEnumerable<LiteDbLocal.SensorDataRec> GetRecords(DateTime startTime, DateTime endTime)
        {
            return Program.RecordsDb.GetRecordsRange(startTime, endTime);
        }

        // GET api/<controller>/<action>/5
        [HttpGet]
        [ActionName("GetRecord")]
        public LiteDbLocal.SensorDataRec GetRecordById(int id)
        {
            return Program.RecordsDb.GetRecordById(id);
        }

        // POST api/<controller>
        /*public static void Post([FromBody] string value)
        {
        }

        // PUT api/<controller>/5
        public static void Put(int id, [FromBody] string value)
        {
        }

        // DELETE api/<controller>/5
        public static void Delete(int id)
        {
        }*/
    }
}