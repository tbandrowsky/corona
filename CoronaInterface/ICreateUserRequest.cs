namespace CoronaInterface
{
    public interface ICreateUserRequest
    {
        string Username { get; }
        string Password { get; }
        string? Email { get; }
    }
}