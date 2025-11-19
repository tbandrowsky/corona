namespace CoronaInterface
{
    public interface IConfirmUserCodeRequest
    {
        string Username { get; }
        string Code { get; }
    }
}