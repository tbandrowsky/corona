using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IQueryClassRequest
    {
        string ClassName { get; }
        IDictionary<string, object>? Filters { get; }
        int? Limit { get; }
    }
}