using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IQueryRequest
    {
        string QueryText { get; }
        IDictionary<string, object>? Parameters { get; }
    }
}