namespace CoronaInterface
{
    public interface ICreateUserResponse
    {
        bool Success { get; }
        string? ErrorMessage { get; }
        string? UserId { get; }
    }
}