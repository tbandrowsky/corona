using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IPutObjectsRequest
    {
        IEnumerable<object> Objects { get; }
    }
}