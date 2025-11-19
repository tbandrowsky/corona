namespace CoronaInterface
{
    public interface ICoronaDatabase : IDisposable
    {
        ILoginResult CreateDatabase(IDatabaseConfiguration configuration);
        ILoginResult OpenDatabase(IDatabaseConfiguration configuration);
        ILoginResult LoginLocal(string username, string password);
        ILoginResult LoginUser(string username, string password);
        ILoginResult LoginUserSso(string username, string password);
        ICreateUserResponse CreateUser(ICreateUserRequest request);
        ISetSendCodeResponse SetSendCode(ISetSendCodeRequest request);
        IConfirmUserCodeResponse ConfirmUserCode(IConfirmUserCodeRequest request);
        IGetClassResponse GetClass(IGetClassRequest request);
        IPutClassResponse PutClass(IPutClassRequest request);
        IGetObjectResponse GetObject(IGetObjectRequest request);
        IEditObjectResponse EditObject(IEditObjectRequest request);
        IRunObjectResponse EditObject(IRunObjectRequest request);
        IPutObjectsResponse PutObjects(IPutObjectsRequest request);
        IDeleteObjectsResponse DeleteObjects(IDeleteObjectsRequest request);
        IQueryClassResponse QueryClass(IQueryClassRequest request);
        IQueryResponse Query(IQueryRequest request);

    }
}
