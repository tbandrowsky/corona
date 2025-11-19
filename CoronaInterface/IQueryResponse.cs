using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IQueryResponse
    {
        bool Success { get; }
        string? ErrorMessage { get; }
        IEnumerable<object>? Results { get; }
    }
}