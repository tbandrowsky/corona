using Newtonsoft.Json;
using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IGetClassRequest : ICoronaBaseRequest
    {
        [JsonProperty("class_name")]

        string ClassName { get; set; }
    }

    public class GetClassRequest : CoronaBaseRequest, IGetClassRequest
    {
        [JsonProperty("class_name")]

        public string ClassName { get; set; } = string.Empty;
    }
}