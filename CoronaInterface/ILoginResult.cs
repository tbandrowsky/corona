using System.Text.Json;
using System.Text.Json.Nodes;

namespace CoronaInterface
{
    public interface ILoginResult : ICoronaBaseResponse
    {
        SysUser User { get; set; }
    }
}
