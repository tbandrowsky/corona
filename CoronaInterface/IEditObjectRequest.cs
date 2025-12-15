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

    public class EditObjectRequest : CoronaBaseRequest, IEditObjectRequest
    {
        public SysObject SysObject { get; set; } = new SysObject();
        public bool IncludeChildren { get; set; } = false;
    }
}
