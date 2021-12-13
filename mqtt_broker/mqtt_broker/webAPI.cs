using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Web.Http;

using DbRecords;

namespace MqttBroker
{
    public class SensorsController : ApiController
    {
        [Route("api/Sensors")]
        public static async Task<IEnumerable<string>> Get()
        {
            Program.LogToScreen("REST request received: GetDeviceList() by default");
            var result = await Task
                .Run(() => Program.RecordsDb.GetDeviceList())
                .ConfigureAwait(true);

            return result;
        }

        [HttpGet]
        [ActionName("GetNamesByMAC")]
        public async Task<IEnumerable<string>> GetNameByMAC(string deviceMac)
        {
            var macBytes = Program.MacFromString(deviceMac);
            Program.LogToScreen("REST request received: GetNameByMAC(" + deviceMac + ")");
            var result = await Task
                .Run(() => Program.RecordsDb.GetDeviceNamesByDeviceMac(macBytes))
                .ConfigureAwait(true);

            return result;
        }

        [HttpGet]
        [ActionName("GetMACByName")]
        public async Task<string> GetMACByName(string deviceName)
        {
            Program.LogToScreen("REST request received: GetMACByName(" + deviceName + ")");
            var result = await Task
                .Run(() => Program.MacToString(Program.RecordsDb.GetDeviceMacByDeviceName(deviceName)))
                .ConfigureAwait(true);

            return result;
        }

        [HttpGet]
        [ActionName("GetDeviceList")]
        public async Task<IEnumerable<string>> GetDeviceList()
        {
            Program.LogToScreen("REST request received: GetDeviceList()");
            var result = await Task
                .Run(() => Program.RecordsDb.GetDeviceList())
                .ConfigureAwait(true);

            return result;
        }

        [HttpGet]
        [ActionName("GetDeviceList")]
        public async Task<IEnumerable<string>> GetDeviceListMinutes(int minutes)
        {
            Program.LogToScreen("REST request received: GetDeviceListMinutes(" + minutes + " days)");
            var result = await Task
                .Run(() => Program.RecordsDb.GetDeviceList(DateTime.Now.AddMinutes(-minutes), DateTime.Now))
                .ConfigureAwait(true);

            return result;
        }

        [HttpGet]
        [ActionName("GetDeviceList")]
        public async Task<IEnumerable<string>> GetDeviceListDays(int days)
        {
            Program.LogToScreen("REST request received: GetDeviceListDays(" + days + " days)");
            var result = await Task
                .Run(() => Program.RecordsDb.GetDeviceList(DateTime.Now.AddDays(-days), DateTime.Now))
                .ConfigureAwait(true);

            return result;
        }

        [HttpGet]
        [ActionName("GetDeviceList")]
        public async Task<IEnumerable<string>> GetDeviceList(DateTime startTime, DateTime endTime)
        {
            Program.LogToScreen("REST request received: GetDeviceList(from " + startTime + " till " + endTime);
            var result = await Task
                .Run(() => Program.RecordsDb.GetDeviceList(startTime, endTime))
                .ConfigureAwait(true);

            return result;
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public async Task<IEnumerable<DeviceRecord>> GetRecordsLastSeconds(int seconds)
        {
            Program.LogToScreen("REST request received: GetRecords(" + seconds + " seconds)");
            var result = await Task
                .Run(() => Program.RecordsDb.GetRecordsRange(DateTime.Now.AddSeconds(-seconds), DateTime.Now))
                .ConfigureAwait(true);

            return result;
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public async Task<IEnumerable<DeviceRecord>> GetRecordsLastMinutes(int minutes)
        {
            Program.LogToScreen("REST request received: GetRecords(" + minutes + " minutes)");
            var result = await Task
                .Run(() => Program.RecordsDb.GetRecordsRange(DateTime.Now.AddMinutes(-minutes), DateTime.Now))
                .ConfigureAwait(true);

            return result;
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public async Task<IEnumerable<DeviceRecord>> GetRecordsLastHours(int hours)
        {
            Program.LogToScreen("REST request received: GetRecords(" + hours + " hours)");
            var result = await Task
                .Run(() => Program.RecordsDb.GetRecordsRange(DateTime.Now.AddHours(-hours), DateTime.Now))
                .ConfigureAwait(true);

            return result;
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public async Task<IEnumerable<DeviceRecord>> GetRecordsLastDays(int days)
        {
            Program.LogToScreen("REST request received: GetRecords(" + days + " days)");
            var result = await Task
                .Run(() => Program.RecordsDb.GetRecordsRange(DateTime.Now.AddDays(-days), DateTime.Now))
                .ConfigureAwait(true);

            return result;
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public async Task<IEnumerable<DeviceRecord>> GetRecordsForDeviceLastMinutes(string deviceName, int minutes)
        {
            Program.LogToScreen("REST request received: GetRecords(device " + deviceName + ", " + minutes + " minutes)");
            var result = await Task
                .Run(() => Program.RecordsDb.GetRecordsRange(deviceName, DateTime.Now.AddMinutes(-minutes), DateTime.Now))
                .ConfigureAwait(true);

            return result;
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public async Task<IEnumerable<DeviceRecord>> GetRecordsForDevice(string deviceName)
        {
            Program.LogToScreen("REST request received: GetRecords(device " + deviceName + ")");
            var result = await Task
                .Run(() => Program.RecordsDb.GetRecordsByDevice(deviceName))
                .ConfigureAwait(true);

            return result;
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public async Task<IEnumerable<DeviceRecord>> GetRecordsForDeviceTimeRange(string deviceName, DateTime startTime,
            DateTime endTime)
        {
            Program.LogToScreen("REST request received: GetRecords(device " + deviceName + ", from " + startTime + " till " + endTime);
            var result = await Task
                .Run(() => Program.RecordsDb.GetRecordsRange(deviceName, startTime, endTime))
                .ConfigureAwait(true);

            return result;
        }

        [HttpGet]
        [ActionName("GetRecords")]
        public async Task<IEnumerable<DeviceRecord>> GetRecordsTimeRange(DateTime startTime, DateTime endTime)
        {
            Program.LogToScreen("REST request received: GetRecords(from " + startTime + " till " + endTime);
            var result = await Task
                .Run(() => Program.RecordsDb.GetRecordsRange(startTime, endTime))
                .ConfigureAwait(true);

            return result;
        }

        // GET api/<controller>/<action>/5
        [HttpGet]
        [ActionName("GetRecord")]
        public async Task<DeviceRecord> GetRecordById(int id)
        {
            Program.LogToScreen("REST request received: GetRecords(# " + id + ")");
            var result = await Task
                .Run(() => Program.RecordsDb.GetRecordById(id))
                .ConfigureAwait(true);

            return result;
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