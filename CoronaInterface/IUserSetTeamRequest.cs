using System;
using System.Collections.Generic;
using System.Text;

namespace CoronaInterface
{
    public interface IUserSetTeamRequest : ICoronaBaseRequest
    {
        public string TeamName { get; set; }
    }

    public class UserSetTeamRequest : CoronaBaseRequest, IUserSetTeamRequest
    {
        public string TeamName { get; set; } = string.Empty;
    }
}
