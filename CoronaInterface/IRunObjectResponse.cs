using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Text;

namespace CoronaInterface
{
    public interface IRunObjectResponse : ICoronaBaseResponse
    {
        SysObject SysObject { get; set; }
        Dictionary<string, CoronaClass> Classes { get; set; }
        JObject ChildClasses { get; set; }
    }
}
