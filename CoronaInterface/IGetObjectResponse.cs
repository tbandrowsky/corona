namespace CoronaInterface
{
    public interface IGetObjectResponse
    {
        bool Success { get; }
        string? ErrorMessage { get; }
        object? ObjectData { get; }
    }
}