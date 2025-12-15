namespace CoronaInterface
{
    public interface IConfirmUserCodeRequest : ICoronaBaseRequest
    {
        public string Email { get; set; }
        public string Code { get; set; }
    }

    public class ConfirmUserCodeRequest : CoronaBaseRequest, IConfirmUserCodeRequest
    {
        public string Email { get; set; } = string.Empty;
        public string Code { get; set; } = string.Empty;
    }
}