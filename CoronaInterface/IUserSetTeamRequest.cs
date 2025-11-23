using System;
using System.Collections.Generic;
using System.Text;

namespace CoronaInterface
{
    public interface IUserSetTeamRequest : ICoronaBaseRequest
    {
        public string TeamName { get; set; }
    }
}
