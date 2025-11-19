namespace CoronaInterface
{
    public interface ISetSendCodeRequest
    {
        string Username { get; }
        string Destination { get; } // e.g. email or phone
    }
}