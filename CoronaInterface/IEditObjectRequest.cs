using System;
using System.Collections.Generic;
using System.Text;

namespace CoronaInterface
{
    public interface IEditObjectRequest : ICoronaBaseRequest
    {
        SysObject SysObject { get; set; }
        bool IncludeChildren {  get; set; }
    }
}
