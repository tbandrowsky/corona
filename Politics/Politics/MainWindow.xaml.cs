using CoronaInterface;
using CoronaLib;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Diagnostics.Contracts;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading;
using Windows.Foundation;
using Windows.Foundation.Collections;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace Politics
{
    /// <summary>
    /// An empty window that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainWindow : Window, ISystemMonitoring
    {
        private CoronaDatabase coronaDatabase = new CoronaDatabase();
        private CoronaStatusModel coronaStatusModel = new CoronaStatusModel();

        public CoronaDatabase CoronaDatabase { get => coronaDatabase; }
        public CoronaStatusModel CoronaStatusModel { get => coronaStatusModel; }

        /// <summary>
        /// Invoked when the application is launched.
        /// </summary>
        /// <param name="args">Details about the launch request and process.</param>
        /// 

        public string DatabasePath => Windows.Storage.ApplicationData.Current.LocalFolder.Path;
        public string ConfigPath => System.IO.Path.Combine(System.IO.Path.GetDirectoryName(Environment.ProcessPath), "configuration");

        public MainWindow()
        {
            InitializeComponent();
            CoronaSystem cm = new CoronaSystem();
            cm.SystemMonitoring = this;
            DispatcherQueue.EnsureSystemDispatcherQueue();
            MessageQueue = DispatcherQueue.GetForCurrentThread();
        }

        public DispatcherQueue MessageQueue { get; private set; }

        public static void CreateDatabase(MainWindow app)
        {
            DatabaseConfiguration? config = new DatabaseConfiguration();

            string config_file_name = System.IO.Path.Combine(app.ConfigPath, "config.json");
            string config_content = File.ReadAllText(config_file_name);

            config = JsonConvert.DeserializeObject<DatabaseConfiguration>(config_content);

            if (config != null)
            {
                config.DatabasePath = app.DatabasePath;

                if (app.coronaDatabase != null)
                {
                    app.coronaDatabase.Dispose();
                }

                app.coronaDatabase = new CoronaDatabase();
                app.coronaDatabase.CreateDatabase(config);
            }
        }

        public void LogUserCommandStart(string commandName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", commandName, message, requestTime);
        }

        public void LogUserCommandStop(string commandName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            coronaStatusModel.StopMessage(commandName, message, elapsedSeconds);
        }

        public void LogCommandStart(string commandName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", commandName, message, requestTime);
        }

        public void LogCommandStop(string commandName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            coronaStatusModel.StopMessage(commandName, message, elapsedSeconds);
        }

        public void LogJobStart(string apiName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", apiName, message, requestTime);
        }

        public void LogJobStop(string apiName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            coronaStatusModel.StopMessage(apiName, message, elapsedSeconds);
        }

        public void LogJobSectionStart(string apiName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", apiName, message, requestTime);
        }

        public void LogJobSectionStop(string apiName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            coronaStatusModel.StopMessage(apiName, message, elapsedSeconds);
        }

        public void LogFunctionStart(string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogFunctionStop(string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            coronaStatusModel.StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogBaseBlockStart(int indent, string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogBaseBlockStop(int indent, string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            coronaStatusModel.StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogTableStart(string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogTableStop(string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            coronaStatusModel.StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogJsonStart(string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogJsonStop(string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            coronaStatusModel.StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogPocoStart(string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogPocoStop(string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            coronaStatusModel.StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogBlockStart(string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogBlockStop(string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            coronaStatusModel.StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogInformation(string message, string file = "", int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", "information", message, DateTime.Now);
            coronaStatusModel.StopMessage("information", message, 0);
        }

        public void LogActivity(string message, DateTime time, string file = "", int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", "activity", message, DateTime.Now);
            coronaStatusModel.StopMessage("activity", message, 0);
        }

        public void LogActivity(string message, double elapsedSeconds, string file = "", int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", "activity", message, DateTime.Now);
            coronaStatusModel.StopMessage("activity", message, 0);
        }

        public void LogPut(string message, double elapsedSeconds, string file = "", int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", "activity", message, DateTime.Now);
            coronaStatusModel.StopMessage("activity", message, 0);
        }

        public void LogAdapter(string message)
        {
            coronaStatusModel.StartMessage("#c0c0c0", "activity", message, DateTime.Now);
            coronaStatusModel.StopMessage("activity", message, 0);
        }

        public void LogWarning(string message, string file = "", int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", "warning", message, DateTime.Now);
            coronaStatusModel.StopMessage("warning", message, 0);
        }

        public void LogException(string message, string file = "", int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", "activity", message, DateTime.Now);
            coronaStatusModel.StopMessage("activity", message, 0);
        }

        public void LogJson<T>(T src, int indent = 2)
        {
        }


    }
}
