using Newtonsoft.Json.Linq;

namespace CoronaInterface
{
    public interface ICreateUserRequest
    {
        SysUser User { get; set; }
    }

    public class CreateUserRequest : ICreateUserRequest
    {
        public SysUser User { get; set; } = new SysUser();
    }
}