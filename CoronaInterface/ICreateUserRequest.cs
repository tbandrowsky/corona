using Newtonsoft.Json.Linq;

namespace CoronaInterface
{
    public interface ICreateUserRequest
    {
        SysUser User { get; set; }
    }
}