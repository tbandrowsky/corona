namespace CoronaInterface
{
    public interface ICoronaDatabase : IDisposable
    {
        bool CreateDatabase(IDatabaseConfiguration configuration);
        bool OpenDatabase(IDatabaseConfiguration configuration);
        ILoginResult LoginLocal(string username, string email);
        ILoginResult LoginUser(string username, string password);
        ILoginResult LoginUserSso(string username, string email, string code);
        ICreateUserResponse CreateUser(ICreateUserRequest request);
        ISetSendCodeResponse SetSendCode(ISetSendCodeRequest request);
        IConfirmUserCodeResponse ConfirmUserCode(IConfirmUserCodeRequest request);
        ILoginResult UserSetTeam(IUserSetTeamRequest request);
        IGetClassResponse GetClass(IGetClassRequest request);
        IPutClassResponse PutClass(IPutClassRequest request);
        IGetObjectResponse GetObject(IGetObjectRequest request);
        IEditObjectResponse EditObject(IEditObjectRequest request);
        IRunObjectResponse RunObject(IRunObjectRequest request);
        IPutObjectsResponse PutObjects(IPutObjectsRequest request);
        IDeleteObjectsResponse DeleteObjects(IDeleteObjectsRequest request);
        IQueryClassResponse QueryClass(IQueryClassRequest request);
        IQueryResponse Query(IQueryRequest request);

    }
}
