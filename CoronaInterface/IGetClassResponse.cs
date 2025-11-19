using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IGetClassResponse
    {
        bool Success { get; }
        string? ErrorMessage { get; }
        IDictionary<string, object>? ClassDefinition { get; }
    }
}