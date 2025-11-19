namespace CoronaInterface
{
    public interface IConfirmUserCodeResponse
    {
        bool Success { get; }
        string? ErrorMessage { get; }
    }
}