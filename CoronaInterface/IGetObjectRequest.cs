using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Text;

namespace CoronaInterface
{
    public interface IGetObjectRequest : ICoronaBaseRequest
    {
        [JsonProperty("class_name")]

        public string ClassName { get; set; }


        [JsonProperty("object_id")]

        public long ObjectId { get; set; }

        [JsonProperty("include_children")]

        public bool IncludeChildren { get; set; }
    }

    public class GetObjectRequest : CoronaBaseRequest, IGetObjectRequest
    {
        [JsonProperty("class_name")]
        public string ClassName { get; set; } = string.Empty;
        [JsonProperty("object_id")]
        public long ObjectId { get; set; } = 0;
        [JsonProperty("include_children")]
        public bool IncludeChildren { get; set; } = true;
    }
}
