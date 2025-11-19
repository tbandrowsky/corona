namespace CoronaInterface
{
    public interface IPutClassResponse
    {
        bool Success { get; }
        string? ErrorMessage { get; }
    }
}