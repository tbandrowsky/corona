namespace CoronaInterface
{
    public interface ISendGrid
    {
        public string ApiKey { get; set; }
        public string SenderName { get; set; }
        public string SenderEmail { get; set; }
        public string UserConfirmationTitle { get; set; }
    }

    public interface IConnection
    {
        public string SourceName { get; set; }
        public string ConnectionString { get; set; }
    }

    public interface IServerConfiguration
    {
        string ListenPoint { get; }
        string ApplicationName { get; }
        string SchemaFilename { get; }
        int DatabaseThreads { get; }
        bool DatabaseRecreate { get; }
        string SysUserName { get; }
        string SysUserPassword { get; }
        string SysUserEmail { get; }
        string NewUserDefaultTeam { get; }
        string OnboardEmailFilename { get; }
        string RecoveryEmailFilename { get; }
    }

    public interface IDatabaseConfiguration
    {
        string DatabasePath { get; set;  }
        ISendGrid SendGrid { get; set; }
        IEnumerable<IServerConfiguration> Servers { get; set;  }
        IEnumerable<IConnection> Connections { get; set; }
    }
}