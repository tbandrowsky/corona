using Newtonsoft.Json;

namespace CoronaInterface
{
    public interface IConfirmUserCodeRequest : ICoronaBaseRequest
    {
        [JsonProperty("user_name")]
        public string UserName { get; set; }

        [JsonProperty("validation_code")]
        public string ValidationCode { get; set; }
    }

    public class ConfirmUserCodeRequest : CoronaBaseRequest, IConfirmUserCodeRequest
    {
        [JsonProperty("user_name")]
        public string UserName { get; set; } = string.Empty;

        [JsonProperty("validation_code")]
        public string ValidationCode { get; set; } = string.Empty;
    }
}