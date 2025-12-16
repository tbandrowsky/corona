using Newtonsoft.Json;
using System.Collections.Generic;

namespace CoronaInterface
{
    public interface IQueryRequest : ICoronaBaseRequest
    {
        [JsonProperty("from")]

        IList<QueryFrom>? From { get; set; }

        [JsonProperty("stages")]
        IList<QueryStage>? Stages { get; set; }

    }

    public class QueryRequest : CoronaBaseRequest, IQueryRequest
    {
        [JsonProperty("from")]

        public IList<QueryFrom>? From { get; set; } = new List<QueryFrom>();

        [JsonProperty("stages")]
        public IList<QueryStage>? Stages { get; set; } = new List<QueryStage>();

    }
}