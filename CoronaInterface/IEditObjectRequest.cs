using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Dynamic;
using System.Text;

namespace CoronaInterface
{
    public interface IEditObjectRequest : ICoronaBaseRequest
    {

        [JsonProperty("include_children")]
        bool IncludeChildren {  get; set; }
    }

    public class EditObjectRequest : CoronaBaseRequest, IEditObjectRequest
    {
        [JsonProperty("include_children")]
        public bool IncludeChildren { get; set; } = false;
    }
}
