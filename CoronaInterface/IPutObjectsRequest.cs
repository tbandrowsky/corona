using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IPutObjectsRequest : ICoronaBaseRequest
    {
        
    }

    public class PutObjectsRequest : CoronaBaseRequest, IPutObjectsRequest
    {

    }
}