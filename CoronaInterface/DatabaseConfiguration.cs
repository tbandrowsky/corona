using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;

namespace CoronaInterface
{
    public class SendGridOptions
    {
        [JsonProperty("api_key")]
        public string? ApiKey { get; set; }
        [JsonProperty("sender_name")]
        public string? SenderName { get; set; }
        [JsonProperty("sender_email")]
        public string? SenderEmail { get; set; }
        [JsonProperty("user_confirmation_title")]
        public string? UserConfirmationTitle { get; set; }
    }

    public class ConnectionBase
    {
        [JsonProperty("source_name")]
        public string? SourceName { get; set; }
    }

    public class DatabaseConnection : ConnectionBase
    {
        [JsonProperty("connection_string")]
        public string? ConnectionString { get; set; }
    }

    public class ApiConnection : ConnectionBase
    {

        [JsonProperty("api_key")]
        public string? ApiKey { get; set; }
        [JsonProperty("api_type")]
        public string? ApiType { get; set; }
    }

    public class WebConnection : ConnectionBase
    {
        [JsonProperty("url")]
        public string? Url { get; set; }
    }

    public class ServerConfiguration
    {
        [JsonProperty("listen_point")]
        public string? ListenPoint { get; set;  }

        [JsonProperty("application_name")]
        public string? ApplicationName { get; set; }

        [JsonProperty("schema_filename")]
        public string? SchemaFilename { get; set; }

        [JsonProperty("database_threads")]
        public int? DatabaseThreads { get; set; }

        [JsonProperty("database_recreate")]
        public bool? DatabaseRecreate { get; set; }

        [JsonProperty("sys_user_name")]
        public string? SysUserName { get; set; }

        [JsonProperty("sys_user_password")]
        public string? SysUserPassword { get; set; }

        [JsonProperty("sys_user_email")]
        public string? SysUserEmail { get; set; }

        [JsonProperty("new_user_default_team")]
        public string? NewUserDefaultTeam { get; set; }

        [JsonProperty("onboard_email_filename")]
        public string? OnboardEmailFilename { get; set; }

        [JsonProperty("recovery_email_filename")]
        public string? RecoveryEmailFilename { get; set; }
    }

    public class DatabaseConfiguration
    {
        [JsonProperty("DatabasePath")]
        public string DatabasePath { get; set; } = "";

        [JsonProperty("SendGrid")]
        public SendGridOptions SendGrid { get; set; } = new SendGridOptions();

        [JsonProperty("Servers")]
        public ServerConfiguration[]? Servers { get; set; }

        [JsonProperty("Connections")]
        public DatabaseConnection[]? Connections { get; set; }
    }
}
