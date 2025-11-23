namespace CoronaInterface
{
    public interface IConfirmUserCodeResponse : ICoronaBaseResponse
    {
        SysUser User { get; set; } 
    }
}