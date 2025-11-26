namespace CoronaInterface
{
    public class SendGridOptions
    {
        public string? ApiKey { get; set; }
        public string? SenderName { get; set; }
        public string? SenderEmail { get; set; }
        public string? UserConfirmationTitle { get; set; }
    }

    public class ConnectionBase
    {
        public string? SourceName { get; set; }
    }

    public class DatabaseConnection : ConnectionBase
    {
        public string? ConnectionString { get; set; }
    }

    public class ApiConnection : ConnectionBase
    {
        public string? ApiKey { get; set; }
        public string? ApiType { get; set; }
    }

    public class WebConnection
    {
        public string? Url { get; set; }
    }

    public class ServerConfiguration
    {
        public string? ListenPoint { get; set;  }
        public string? ApplicationName { get; set; }
        public string? SchemaFilename { get; set; }
        public int? DatabaseThreads { get; set; }
        public bool? DatabaseRecreate { get; set; }
        public string? SysUserName { get; set; }
        public string? SysUserPassword { get; set; }
        public string? SysUserEmail { get; set; }
        public string? NewUserDefaultTeam { get; set; }
        public string? OnboardEmailFilename { get; set; }
        public string? RecoveryEmailFilename { get; set; }
    }

    public class DatabaseConfiguration
    {
        public string? DatabasePath { get; set; }
        public SendGridOptions? SendGrid { get; set; }
        public IList<ServerConfiguration>? Servers { get; set; }
        public IList<ConnectionBase>? Connections { get; set; }
    }
}
