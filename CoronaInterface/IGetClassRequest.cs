using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IGetClassRequest
    {
        string ClassName { get; }
        IDictionary<string, object>? Options { get; }
    }
}