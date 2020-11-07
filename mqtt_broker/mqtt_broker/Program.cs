using MqttBroker.Properties;

using MQTTnet;
using MQTTnet.Protocol;
using MQTTnet.Server;

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IdentityModel.Selectors;
using System.IdentityModel.Tokens;
using System.IO;
using System.ServiceModel;
using System.Text;
using System.Threading.Tasks;
using System.Web.Http;
using System.Web.Http.SelfHost;
using System.Web.Script.Serialization;

using static System.String;

namespace MqttBroker
{
    internal class Program
    {
        private static bool _verboseLog;
        private static bool _dbNotStarted;
        public static LiteDbLocal RecordsDb;

        private static async Task Main()
        {
            var mqttStarted = true;
            var restStarted = true;
            _verboseLog = Settings.Default.VerboseLog;

            LogToScreen("MQTTBroker");

            try
            {
                RecordsDb = new LiteDbLocal(Settings.Default.DbFileName, Settings.Default.ClientID);
                LogToScreen("DataBase name: " + Settings.Default.DbFileName);
            }
            catch (Exception e)
            {
                LogToScreen("MQTT server database connection error: " + e);
                _dbNotStarted = true;
            }

            // Setup client validator.
            var optionsBuilder = new MqttServerOptionsBuilder()
                .WithConnectionValidator(MqttConnectHandler)
                .WithSubscriptionInterceptor(MqttSubscribeHandler)
                .WithApplicationMessageInterceptor(MqttPublishHandler)
                .WithConnectionBacklog(100)
                .WithDefaultEndpointPort(Settings.Default.MQTTPort);

            var mqttServer = new MqttFactory().CreateMqttServer();
            //mqttServer.ApplicationMessageReceivedHandler = new DataReceiveHandler();
            mqttServer.ClientDisconnectedHandler = new MqttDisconnectHandler();
            mqttServer.ClientUnsubscribedTopicHandler = new MqttUnsubscribeHandler();
            try
            {
                await mqttServer.StartAsync(optionsBuilder.Build()).ConfigureAwait(true);
                LogToScreen("MQTT: port: " + Settings.Default.MQTTPort);
            }
            catch (Exception e)
            {
                LogToScreen("MQTT: server start error: " + e);
                mqttStarted = false;
            }

            // REST API start
            var restConfig = new HttpSelfHostConfiguration(Settings.Default.RESTHost + ":" + Settings.Default.RESTPort);
            restConfig.Routes.MapHttpRoute(
                "API Default",
                "api/{controller}/{action}/{id}",
                new { id = RouteParameter.Optional });
            restConfig.MapHttpAttributeRoutes();
            restConfig.IncludeErrorDetailPolicy = IncludeErrorDetailPolicy.Always;
            restConfig.TransferMode = TransferMode.StreamedResponse;
            restConfig.ClientCredentialType = HttpClientCredentialType.Basic;
            restConfig.UserNamePasswordValidator = new RestUserValidator();
            /*var f = restConfig.Formatters;
            f.JsonFormatter.UseDataContractJsonSerializer = true;
            f.JsonFormatter.SerializerSettings.DateFormatHandling = DateFormatHandling.MicrosoftDateFormat;*/
            var restServer = new HttpSelfHostServer(restConfig);

            try
            {
                restServer.OpenAsync().Wait();
                LogToScreen("REST: started at: " + Settings.Default.RESTHost + ":" + Settings.Default.RESTPort);
            }
            catch (Exception e)
            {
                LogToScreen("REST: server start error: " + e);
                restStarted = false;
            }

            if (!mqttStarted)
            {
                LogToScreen("Can't start MQTT server.");
            }
            if (!restStarted)
            {
                LogToScreen("Can't start REST server.");
                restServer.Dispose();
                restConfig.Dispose();
                restServer.Dispose();
            }
            if (!mqttStarted || !restStarted)
            {
                LogToScreen("Closing application...");
                RecordsDb?.Dispose();
                return;
            }

            var resp = "";
            while (resp?.ToUpperInvariant() != "EXIT")
            {
                LogToScreen("Enter \"exit\" to quit.");
                resp = Console.ReadLine();
            }

            await restServer.CloseAsync().ConfigureAwait(false);
            await mqttServer.StopAsync().ConfigureAwait(false);
            RecordsDb?.Dispose();
            restServer.Dispose();
            restConfig.Dispose();
            restServer.Dispose();
        }

        private static void MqttConnectHandler(MqttConnectionValidatorContext c)
        {
            if (c.ClientId.Length < 5)
            {
                LogToScreen("MQTT: Client " + c.ClientId + " identifier not valid (ID length < 5)");
                c.ReasonCode = MqttConnectReasonCode.ClientIdentifierNotValid;
                return;
            }

            if (!IsNullOrEmpty(Settings.Default.User) && c.Username != Settings.Default.User || !IsNullOrEmpty(Settings.Default.Password) && c.Password != Settings.Default.Password)
            {
                LogToScreen("MQTT: Client " + c.ClientId + " username or password not valid");
                c.ReasonCode = MqttConnectReasonCode.BadUserNameOrPassword;
                return;
            }

            LogToScreen("MQTT: Client " + c.ClientId + " connected as: " + c.Username);

            c.ReasonCode = MqttConnectReasonCode.Success;
        }

        private static async void MqttPublishHandler(MqttApplicationMessageInterceptorContext c)
        {
            if (c == null || c.ApplicationMessage == null || c.ApplicationMessage.Payload == null || c.ApplicationMessage.Payload.Length == 0) return;
            if (_verboseLog)
            {
                LogToScreen("MQTT: Client " + c.ClientId + " posted to: " + c.ApplicationMessage.Topic);
            }
            await Task.Run(() => { Mqtt_DataReceived(c.ApplicationMessage.Payload, c.ApplicationMessage.Topic); }).ConfigureAwait(true);
            c.AcceptPublish = true;
        }

        private static void MqttSubscribeHandler(MqttSubscriptionInterceptorContext c)
        {
            LogToScreen("MQTT: Client " + c.ClientId + " subscribed to: " + c.TopicFilter.Topic);
            c.AcceptSubscription = true;
        }

        private class MqttDisconnectHandler : IMqttServerClientDisconnectedHandler
        {
            public Task HandleClientDisconnectedAsync(MqttServerClientDisconnectedEventArgs eventArgs)
            {
                LogToScreen("MQTT: Client " + eventArgs.ClientId + " disconnected" + Environment.NewLine);
                return null;
            }
        }

        private class MqttUnsubscribeHandler : IMqttServerClientUnsubscribedTopicHandler
        {
            public Task HandleClientUnsubscribedTopicAsync(MqttServerClientUnsubscribedTopicEventArgs eventArgs)
            {
                    LogToScreen("MQTT: Client " + eventArgs.ClientId + " unsubscribed from: " + eventArgs.TopicFilter + Environment.NewLine);

                return null;
            }
        }

        private static void Mqtt_DataReceived(byte[] payload, string topic)
        {
            if (!_verboseLog && _dbNotStarted) return;

            // parse strings to key/value set
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

            // save to log if needed
            if (_verboseLog)
            {
                var resultStr = new StringBuilder();

                foreach (var s in stringsSet)
                {
                    resultStr.AppendLine("\t[" + topic + "]<< " + s.Key + "=" + s.Value);
                }
                LogToScreen(resultStr + Environment.NewLine);
            }

            // save to DB if exists
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

                var v = "";
                var floatSeparatore = CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator;
                v = floatSeparatore == "," ? item.Value.Replace(".", floatSeparatore) : v.Replace(",", floatSeparatore);

                if (!float.TryParse(v, out var value)) continue;

                //add result to log
                newValue.Value = value;
                newValue.ValueType = item.Key;
                currentResult.ValueList.Add(newValue);
            }

            RecordsDb.AddRecord(currentResult);
        }

        private class RestUserValidator : UserNamePasswordValidator
        {
            public override void Validate(string userName, string password)
            {
                LogToScreen("REST connect request: " + userName);

                if (!IsNullOrEmpty(Settings.Default.User) && userName != Settings.Default.User
                    || !IsNullOrEmpty(Settings.Default.Password) && password != Settings.Default.Password)
                {
                    LogToScreen("REST Result: username or password not valid");
                    throw new SecurityTokenException("Unknown Username or Password");
                }
            }
        }

        private static void LogToScreen(string message)
        {
            try
            {
                Console.WriteLine(message);
            }
            catch (IOException IOException)
            {
                // TODO: Handle the System.IO.IOException
            }
        }
    }
}
