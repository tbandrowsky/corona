using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IQueryRequest : ICoronaBaseRequest
    {
        QueryBody QueryBody { get; set; }
    }

    public class QueryRequest : CoronaBaseRequest, IQueryRequest
    {
        public QueryBody QueryBody { get; set; } = new QueryBody();
    }
}