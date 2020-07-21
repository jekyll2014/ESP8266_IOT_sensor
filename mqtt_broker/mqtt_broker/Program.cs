using MqttBroker.Properties;

using MQTTnet;
using MQTTnet.Client.Receiving;
using MQTTnet.Protocol;
using MQTTnet.Server;

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;
using System.Threading.Tasks;
using System.Web.Http;
using System.Web.Http.SelfHost;
using System.Web.Script.Serialization;

namespace MqttBroker
{
    internal class Program
    {
        private static bool _verboseLog;
        private static bool _dbNotStarted;
        public static LiteDbLocal RecordsDb;

        private static async Task Main(string[] args)
        {
            var mqttNotStarted = false;
            var restNotStarted = false;
            _verboseLog = Settings.Default.VerboseLog;

            Console.WriteLine("MQTTBroker");

            try
            {
                RecordsDb = new LiteDbLocal(Settings.Default.DbFileName, Settings.Default.ClientID);
                Console.WriteLine("DataBase name: " + Settings.Default.DbFileName);
            }
            catch (Exception e)
            {
                Console.WriteLine("MQTT server database connection error: " + e);
                _dbNotStarted = true;
            }


            // Setup client validator.
            var optionsBuilder = new MqttServerOptionsBuilder()
                .WithConnectionValidator(c =>
                {
                    var resultStr = new StringBuilder();

                    resultStr.AppendLine("Client: " + c.ClientId + Environment.NewLine +
                                         "connected as: " + c.Username);
                    if (c.ClientId.Length < 5)
                    {
                        resultStr.AppendLine("Result: identifier not valid (ID length < 5)");
                        c.ReasonCode = MqttConnectReasonCode.ClientIdentifierNotValid;
                        return;
                    }

                    if (!string.IsNullOrEmpty(Settings.Default.User) && c.Username != Settings.Default.User
                        || !string.IsNullOrEmpty(Settings.Default.Password) && c.Password != Settings.Default.Password)
                    {
                        resultStr.AppendLine("Result: username or password not valid");
                        c.ReasonCode = MqttConnectReasonCode.BadUserNameOrPassword;
                        return;
                    }

                    c.ReasonCode = MqttConnectReasonCode.Success;
                    Console.WriteLine(resultStr);
                })
                .WithSubscriptionInterceptor(c =>
                {
                    var resultStr = new StringBuilder();
                    resultStr.AppendLine("Client: " + c.ClientId + Environment.NewLine + "Subscribed to: " +
                                         c.TopicFilter.Topic);
                    Console.WriteLine(resultStr + Environment.NewLine);
                    c.AcceptSubscription = true;
                })
                .WithApplicationMessageInterceptor(c => { c.AcceptPublish = true; })
                .WithConnectionBacklog(100)
                .WithDefaultEndpointPort(Settings.Default.MQTTPort);

            var mqttServer = new MqttFactory().CreateMqttServer();
            try
            {
                await mqttServer.StartAsync(optionsBuilder.Build()).ConfigureAwait(false);
                Console.WriteLine("MQTT broker port: " + Settings.Default.MQTTPort);
            }
            catch (Exception e)
            {
                Console.WriteLine("MQTT server start error: " + e);
                mqttNotStarted = true;
            }

            mqttServer.ApplicationMessageReceivedHandler =
                new MqttApplicationMessageReceivedHandlerDelegate(async c =>
                {
                    if (c?.ApplicationMessage?.Payload == null || c.ApplicationMessage.Payload.Length == 0) return;

                    await Task.Run(() =>
                    {
                        Mqtt_DataReceived(c?.ApplicationMessage?.Payload, c?.ApplicationMessage?.Topic);
                    }).ConfigureAwait(true);
                });

            mqttServer.ClientDisconnectedHandler = new DisconnectHandler();

            mqttServer.ClientUnsubscribedTopicHandler = new UnsubscribeHandler();

            // REST API start
            var restConfig = new HttpSelfHostConfiguration(Settings.Default.RESTHost + ":" + Settings.Default.RESTPort);
            restConfig.Routes.MapHttpRoute(
                "API Default",
                "api/{controller}/{action}/{id}",
                new { id = RouteParameter.Optional });
            restConfig.MapHttpAttributeRoutes();

            var restServer = new HttpSelfHostServer(restConfig);

            try
            {
                restServer.OpenAsync().Wait();
                Console.WriteLine("REST service at: " + Settings.Default.RESTHost + ":" + Settings.Default.RESTPort);
            }
            catch (Exception e)
            {
                Console.WriteLine("REST server start error: " + e);
                restNotStarted = true;
            }

            if (mqttNotStarted && restNotStarted)
            {
                Console.WriteLine("Can't start MQTT and REST servers.");
                RecordsDb?.Dispose();
                restServer.Dispose();
                restConfig.Dispose();
                restServer.Dispose();
                return;
            }

            var resp = "";
            while (resp?.ToUpperInvariant() != "EXIT")
            {
                Console.WriteLine("Enter \"exit\" to quit.");
                resp = Console.ReadLine();
            }

            await restServer.CloseAsync().ConfigureAwait(false);
            await mqttServer.StopAsync().ConfigureAwait(false);
            RecordsDb?.Dispose();
            restServer.Dispose();
            restConfig.Dispose();
            restServer.Dispose();
        }

        private class DisconnectHandler : IMqttServerClientDisconnectedHandler
        {
            public Task HandleClientDisconnectedAsync(MqttServerClientDisconnectedEventArgs eventArgs)
            {
                Console.WriteLine("Client: " + eventArgs.ClientId + Environment.NewLine + "Disconnected" +
                                  Environment.NewLine);
                return null;
            }
        }

        private class UnsubscribeHandler : IMqttServerClientUnsubscribedTopicHandler
        {
            public Task HandleClientUnsubscribedTopicAsync(MqttServerClientUnsubscribedTopicEventArgs eventArgs)
            {
                Console.WriteLine("Client: " + eventArgs.ClientId + Environment.NewLine + "Unsubscribed from: " +
                                  eventArgs.TopicFilter + Environment.NewLine);
                return null;
            }
        }

        private static void Mqtt_DataReceived(byte[] payload, string topic)
        {
            if (!_verboseLog && _dbNotStarted) return;

            var tmpStr = Encoding.UTF8.GetString(payload);

            Dictionary<string, string> stringsSet;
            var jss = new JavaScriptSerializer();
            try
            {
                stringsSet = jss.Deserialize<Dictionary<string, string>>(tmpStr);
            }
            catch (Exception)
            {
                return;
            }

            if (_verboseLog)
            {
                tmpStr = "";
                foreach (var s in stringsSet)
                {
                    tmpStr += "[" + topic + "]<< " + s.Key + "=" + s.Value + Environment.NewLine;
                }
            }

            Console.WriteLine(tmpStr);

            if (_dbNotStarted) return;

            var currentResult = new LiteDbLocal.SensorDataRec();
            if (stringsSet.TryGetValue("DeviceName", out var devName))
            {
                stringsSet.Remove("DeviceName");

                currentResult.ValueList = new List<LiteDbLocal.ValueItemRec>();
                currentResult.DeviceName = devName;
                currentResult.Time = DateTime.Now;
            }
            else
            {
                return;
            }

            foreach (var item in stringsSet)
            {
                var newValue = new LiteDbLocal.ValueItemRec();
                var v = item.Value.Replace(".", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
                v = v.Replace(",", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);
                if (!float.TryParse(v, out var value)) continue;

                //add result to log
                newValue.Value = value;
                newValue.ValueType = item.Key;
                currentResult.ValueList.Add(newValue);
            }

            currentResult.Id = RecordsDb.AddRecord(currentResult);
        }
    }
}
