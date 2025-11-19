using System;
using System.Collections.Generic;
using System.Text;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace CoronaInterface
{
    public interface ICoronaBaseRequest
    {
        string Token { get; set; }
        JToken Data { get; set; }
    }
}
