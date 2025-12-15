using System;
using System.Collections.Generic;
using System.Text;

namespace CoronaInterface
{
    public interface IPutClassRequest : ICoronaBaseRequest
    {
        CoronaClass CoronaClass { get; set; }
    }

    public class PutClassRequest : CoronaBaseRequest, IPutClassRequest
    {
        public CoronaClass CoronaClass { get; set; } = new CoronaClass();
    }
}