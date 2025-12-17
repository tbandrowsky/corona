using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Text;

namespace CoronaInterface
{
    public class CoronaError
    {
        public string class_name { get; set; }
        public string field_name { get; set; }
        public string message { get; set; }
        public string filename { get; set; }
        public double line_number { get; set; }
        public double count { get; set; }
    }
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
        [JsonProperty("errors")]
        IList<CoronaError> Errors { get; set; }
    }
}
