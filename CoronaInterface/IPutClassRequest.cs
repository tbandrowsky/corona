using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Text;

namespace CoronaInterface
{
    public interface IPutClassRequest : ICoronaBaseRequest
    {
        [JsonProperty("data")]

        CoronaClass CoronaClass { get; set; }
    }

    public class PutClassRequest : CoronaBaseRequest, IPutClassRequest
    {
        [JsonProperty("data")]

        public CoronaClass CoronaClass { get; set; } = new CoronaClass();
    }
}