namespace CoronaInterface
{
    public interface IDeleteObjectsResponse
    {
        bool Success { get; }
        string? ErrorMessage { get; }
        int ObjectsDeleted { get; }
    }
}