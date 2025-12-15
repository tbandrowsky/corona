using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IQueryClassRequest : ICoronaBaseRequest
    {
    }

    public class QueryClassRequest : CoronaBaseRequest, IQueryClassRequest
    {
        public string ClassName { get; set; } = string.Empty;   
    }
}