namespace CoronaInterface
{
    public interface ISetSendCodeResponse
    {
        bool Success { get; }
        string? ErrorMessage { get; }
    }
}