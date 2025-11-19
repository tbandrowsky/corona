using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Text;

namespace CoronaInterface
{
    public interface ICoronaBaseResponse
    {
        bool Success { get; set; }
        string Message { get; set; }
        double ExecutionTimeSeconds { get; set; }
        JToken Data { get; set; }

    }
}
