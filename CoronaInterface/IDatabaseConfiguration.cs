namespace CoronaInterface
{

    /*
     * 
     *   "SendGrid": {
    "ApiKey": "CONNECTION_SENDGRID",
    "SenderName": "Todd Bandrowsky",
    "SenderEmail": "todd.bandrowsky@countryvideogames.com",
    "UserConfirmationTitle": "Your Revolution Enlistment"
  },
  "Connections": {
    "AdventureWorks2016": "Driver={ODBC Driver 17 for SQL Server};Server=BANDROWSKY-RIPP\\DEV01;Database=AdventureWorks2016;Trusted_Connection=yes;TrustServerCertificate=yes"
  },
  "Servers": [
    {
      "listen_point": "http://localhost:5678/azure_simple/",
      "application_name": "azure_simple",
      "schema_filename": "azure_schema.json",
      "database_filename": "azure_database.cdb",
      "database_threads": 20,
      "database_recreate": true,
      "sys_user_name": "system",
      "sys_user_password": "systempassword1!",
      "sys_user_email": "todd.bandrowsky@gmail.com",
      "new_user_default_team": "ignored",
      "onboard_email_filename": "onboarding_email_template.html",
      "recovery_email_filename": "account_recovery_email_template.html"
    },
     */


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
        string DatabaseFilename { get; }
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
        string DatabasePath { get; }
        string? ConnectionString { get; }
        bool UseEncryption { get; }
    }
}