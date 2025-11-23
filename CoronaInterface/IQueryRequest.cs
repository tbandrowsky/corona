using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IQueryRequest : ICoronaBaseRequest
    {
        QueryBody QueryBody { get; set; }
    }
}