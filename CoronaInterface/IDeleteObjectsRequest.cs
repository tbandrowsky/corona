using Newtonsoft.Json;
using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IDeleteObjectsRequest : ICoronaBaseRequest
    {
        [JsonProperty("class_name")]
        string ClassName { get; set; }

        [JsonProperty("object_id")]
        string ObjectId { get; set; }
    }

    public class DeleteObjectsRequest : CoronaBaseRequest, IDeleteObjectsRequest
    {
        [JsonProperty("class_name")]
        public string ClassName { get; set; }

        [JsonProperty("object_id")]
        public string ObjectId { get; set; }
    }
}