using Newtonsoft.Json.Linq;

namespace CoronaInterface
{
    public interface ICreateUserRequest
    {
        JToken Data { get; set; }
    }
}