using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IGetClassResponse : ICoronaBaseResponse
    {
        CoronaClass CoronaClass { get; set; }
    }
}