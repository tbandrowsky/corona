using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Text;

namespace CoronaInterface
{
    public interface ICoronaBaseResponse
    {
        [JsonProperty("success")]
        bool Success { get; set; }
        [JsonProperty("message")]
        string Message { get; set; }
        [JsonProperty("token")]
        string Token { get; set; }
        [JsonProperty("executiontimeseconds")]
        double ExecutionTimeSeconds { get; set; }
        [JsonProperty("data")]
        JToken Data { get; set; }

    }
}
