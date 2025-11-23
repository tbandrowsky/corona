namespace CoronaInterface
{
    public interface IConfirmUserCodeRequest : ICoronaBaseRequest
    {
        public string Email { get; set; }
        public string Code { get; set; }
    }
}