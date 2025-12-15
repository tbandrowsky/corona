using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IDeleteObjectsRequest : ICoronaBaseRequest
    {
        IEnumerable<string> ObjectIds { get; }
    }

    public class DeleteObjectsRequest : CoronaBaseRequest, IDeleteObjectsRequest
    {
        public IEnumerable<string> ObjectIds { get; set; } = new List<string>();
    }
}