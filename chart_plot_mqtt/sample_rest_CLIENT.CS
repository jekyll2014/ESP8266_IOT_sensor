/*
To post use something like this:
await HttpHelper.Post<Setting>($"/api/values/{id}", setting);

Example for delete:
await HttpHelper.Delete($"/api/values/{id}");

Example to get list:
List<ClaimTerm> claimTerms = await HttpHelper.Get<List<ClaimTerm>>("/api/values/");

Example to get only one:
ClaimTerm processedClaimImage = await HttpHelper.Get<ClaimTerm>($"/api/values/{id}");
*/

using Newtonsoft.Json;
using System;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using System.Configuration;

namespace WebApp
{
    public static class HttpHelper
    {
        // In my case this is https://localhost:44366/
        private static readonly string apiBasicUri = ConfigurationManager.AppSettings["apiBasicUri"];

        public static async Task Post<T>(string url, T contentValue)
        {
            using (var client = new HttpClient())
            {
                client.BaseAddress = new Uri(apiBasicUri);
                var content = new StringContent(JsonConvert.SerializeObject(contentValue), Encoding.UTF8, "application/json");
                var result = await client.PostAsync(url, content);
                result.EnsureSuccessStatusCode();
            }
        }

        public static async Task Put<T>(string url, T stringValue)
        {
            using (var client = new HttpClient())
            {
                client.BaseAddress = new Uri(apiBasicUri);
                var content = new StringContent(JsonConvert.SerializeObject(stringValue), Encoding.UTF8, "application/json");
                var result = await client.PutAsync(url, content);
                result.EnsureSuccessStatusCode();
            }
        }

        public static async Task<T> Get<T>(string url)
        {
            using (var client = new HttpClient())
            {
                client.BaseAddress = new Uri(apiBasicUri);
                var result = await client.GetAsync(url);
                result.EnsureSuccessStatusCode();
                string resultContentString = await result.Content.ReadAsStringAsync();
                T resultContent = JsonConvert.DeserializeObject<T>(resultContentString);
                return resultContent;
            }
        }

        public static async Task Delete(string url)
        {
            using (var client = new HttpClient())
            {
                client.BaseAddress = new Uri(apiBasicUri);
                var result = await client.DeleteAsync(url);
                result.EnsureSuccessStatusCode();
            }
        }
    }
}