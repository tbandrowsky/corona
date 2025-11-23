namespace CoronaInterface
{
    public interface IPutClassResponse : ICoronaBaseResponse
    {
        CoronaClass CoronaClass { get; set; }
    }
}