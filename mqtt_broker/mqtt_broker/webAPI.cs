using System;
using System.Collections.Generic;
using System.Web.Http;

namespace MqttBroker
{
    public class SensorsController : ApiController
    {
        [Route("api/Sensors")]
        public static IEnumerable<string> Get()
        {
            Console.WriteLine("REST request received: GetDeviceList() by default");
            return Program.RecordsDb.GetDeviceList();
        }

        [HttpGet]
        [ActionName("GetDeviceList")]
        public IEnumerable<string> GetDeviceList()
        {
            Console.WriteLine("REST request received: GetDeviceList()");
            return Program.RecordsDb.GetDeviceList();
        }

        [HttpGet]
        [ActionName("GetDeviceList")]
        public IEnumerable<string> GetDeviceListMinutes(int minutes)
        {
            Console.WriteLine("REST request received: GetDeviceListMinutes(" + minutes + " days)");
            return Program.RecordsDb.GetDeviceList(DateTime.Now.AddMinutes(-minutes), DateTime.Now);
        }

        [HttpGet]
        [ActionName("GetDeviceList")]
        public IEnumerable<string> GetDeviceListDays(int days)
        {
            Console.WriteLine("REST request received: GetDeviceListDays(" + days + " days)");
            return Program.RecordsDb.GetDeviceList(DateTime.Now.AddDays(-days), DateTime.Now);
        }

        [HttpGet]
        [ActionName("GetDeviceList")]
        public IEnumerable<string> GetDeviceList(DateTime startTime, DateTime endTime)
        {
            Console.WriteLine("REST request received: GetDeviceList(from " + startTime + " till " + endTime);
            return Program.RecordsDb.GetDeviceList(startTime, endTime);
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public IEnumerable<LiteDbLocal.SensorDataRec> GetRecordsLastSeconds(int seconds)
        {
            Console.WriteLine("REST request received: GetRecords(" + seconds + " seconds)");
            return Program.RecordsDb.GetRecordsRange(DateTime.Now.AddSeconds(-seconds), DateTime.Now);
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public IEnumerable<LiteDbLocal.SensorDataRec> GetRecordsLastMinutes(int minutes)
        {
            Console.WriteLine("REST request received: GetRecords(" + minutes + " minutes)");
            return Program.RecordsDb.GetRecordsRange(DateTime.Now.AddMinutes(-minutes), DateTime.Now);
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public IEnumerable<LiteDbLocal.SensorDataRec> GetRecordsLastHours(int hours)
        {
            Console.WriteLine("REST request received: GetRecords(" + hours + " hours)");
            return Program.RecordsDb.GetRecordsRange(DateTime.Now.AddHours(-hours), DateTime.Now);
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public IEnumerable<LiteDbLocal.SensorDataRec> GetRecordsLastDays(int days)
        {
            Console.WriteLine("REST request received: GetRecords(" + days + " days)");
            return Program.RecordsDb.GetRecordsRange(DateTime.Now.AddDays(-days), DateTime.Now);
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public IEnumerable<LiteDbLocal.SensorDataRec> GetRecordsForDeviceLastMinutes(string deviceName, int minutes)
        {
            Console.WriteLine("REST request received: GetRecords(device " + deviceName + ", " + minutes + " minutes)");
            return Program.RecordsDb.GetRecordsRange(deviceName, DateTime.Now.AddMinutes(-minutes), DateTime.Now);
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public IEnumerable<LiteDbLocal.SensorDataRec> GetRecordsForDevice(string deviceName)
        {
            Console.WriteLine("REST request received: GetRecords(device " + deviceName + ")");
            return Program.RecordsDb.GetRecordsByDevice(deviceName);
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public IEnumerable<LiteDbLocal.SensorDataRec> GetRecordsForDeviceTimeRange(string deviceName, DateTime startTime,
            DateTime endTime)
        {
            Console.WriteLine("REST request received: GetRecords(device " + deviceName + ", from " + startTime + " till " + endTime);
            return Program.RecordsDb.GetRecordsRange(deviceName, startTime, endTime);
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public IEnumerable<LiteDbLocal.SensorDataRec> GetRecordsTimeRange(DateTime startTime, DateTime endTime)
        {
            Console.WriteLine("REST request received: GetRecords(from " + startTime + " till " + endTime);
            return Program.RecordsDb.GetRecordsRange(startTime, endTime);
        }

        // GET api/<controller>/<action>/5
        [HttpGet]
        [ActionName("GetRecord")]
        public LiteDbLocal.SensorDataRec GetRecordById(int id)
        {
            Console.WriteLine("REST request received: GetRecords(# " + id + ")");
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