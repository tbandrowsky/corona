using System;
using System.Collections.Generic;
using System.Text;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace CoronaInterface
{
    public interface ICoronaBaseRequest
    {

        [JsonProperty("token")]
        string Token { get; set; }
        [JsonProperty("data")]
        JToken Data { get; set; }
    }

    public class CoronaBaseRequest : ICoronaBaseRequest
    {
        [JsonProperty("token")]
        public string Token { get; set; } = string.Empty;
        [JsonProperty("data")]
        public JToken Data { get; set; } = new JObject();
    }
}
