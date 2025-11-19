namespace CoronaInterface
{
    public interface ILoginResult
    {
        bool Success { get; }
        string? ErrorMessage { get; }
    }
}