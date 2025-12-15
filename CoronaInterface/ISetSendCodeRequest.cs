namespace CoronaInterface
{
    public interface ISetSendCodeRequest : ICoronaBaseRequest
    {
        public string Email { get; set;  }
    }

    public class SetSendCodeRequest : CoronaBaseRequest, ISetSendCodeRequest
    {
        public string Email { get; set; } = string.Empty;
    }
}