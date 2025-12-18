using System;
using System.Collections.Generic;
using System.Dynamic;
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
        ExpandoObject Data { get; set; }
    }

    public class CoronaBaseRequest : ICoronaBaseRequest
    {
        [JsonProperty("token")]
        public string Token { get; set; } = string.Empty;
        [JsonProperty("data")]
        public ExpandoObject Data { get; set; } = new ExpandoObject();
    }
}
