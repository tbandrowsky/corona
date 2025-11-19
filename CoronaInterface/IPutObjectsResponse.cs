namespace CoronaInterface
{
    public interface IPutObjectsResponse
    {
        bool Success { get; }
        string? ErrorMessage { get; }
        int ObjectsInserted { get; }
    }
}