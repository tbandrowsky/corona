using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace CoronaInterface
{
    public interface ICreateUserRequest
    {
        [JsonProperty("user")]

        SysUser User { get; set; }
    }

    public class CreateUserRequest : ICreateUserRequest
    {
        [JsonProperty("user")]

        public SysUser User { get; set; } = new SysUser();
    }
}