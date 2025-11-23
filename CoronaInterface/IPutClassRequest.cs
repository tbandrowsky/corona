using System;
using System.Collections.Generic;
using System.Text;

namespace CoronaInterface
{
    public interface IPutClassRequest : ICoronaBaseRequest
    {
        CoronaClass CoronaClass { get; set; }
    }
}
