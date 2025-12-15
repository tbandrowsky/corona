using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IGetClassRequest : ICoronaBaseRequest
    {
        string ClassName { get; set; }
    }

    public class GetClassRequest : CoronaBaseRequest, IGetClassRequest
    {
        public string ClassName { get; set; } = string.Empty;
    }
}