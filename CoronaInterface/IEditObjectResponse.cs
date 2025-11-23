using System;
using System.Collections.Generic;
using System.Text;
using System.Text.Json.Nodes;

namespace CoronaInterface
{
    public interface IEditObjectResponse : ICoronaBaseResponse
    {
        SysObject SysObject { get; set; }
        Dictionary<string, CoronaClass> Classes { get; set; }
        JsonObject ChildClasses { get; set; }

    }
}
