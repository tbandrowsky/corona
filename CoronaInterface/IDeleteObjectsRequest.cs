using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IDeleteObjectsRequest : ICoronaBaseRequest
    {
        IEnumerable<string> ObjectIds { get; }
    }
}