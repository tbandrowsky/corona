namespace CoronaInterface
{
    public interface ISetSendCodeRequest : ICoronaBaseRequest
    {
        public string Email { get; set;  }
    }
}