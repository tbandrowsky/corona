using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IQueryRequest : ICoronaBaseRequest
    {
        IQueryBody QueryBody { get; set; }
    }
}