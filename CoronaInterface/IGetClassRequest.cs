using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IGetClassRequest : ICoronaBaseRequest
    {
        string ClassName { get; set; }
    }
}