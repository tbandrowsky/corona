using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IQueryClassResponse
    {
        bool Success { get; }
        string? ErrorMessage { get; }
        IEnumerable<object>? Results { get; }
    }
}