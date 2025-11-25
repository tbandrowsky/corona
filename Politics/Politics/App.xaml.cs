using CoronaLib;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.UI.Xaml.Shapes;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.ApplicationModel;
using Windows.ApplicationModel.Activation;
using Windows.Foundation;
using Windows.Foundation.Collections;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace Politics
{
    /// <summary>
    /// Provides application-specific behavior to supplement the default Application class.
    /// </summary>
    public partial class App : Application, CoronaInterface.ISystemMonitoring
    {
        private Window? _window;

        private CoronaDatabase coronaDatabase = new CoronaDatabase();
        private  CoronaStatusModel coronaStatusModel = new CoronaStatusModel();

        public CoronaDatabase CoronaDatabase { get => coronaDatabase; }
        public CoronaStatusModel CoronaStatusModel { get => coronaStatusModel; }

        /// <summary>
        /// Initializes the singleton application object.  This is the first line of authored code
        /// executed, and as such is the logical equivalent of main() or WinMain().
        /// </summary>
        public App()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Invoked when the application is launched.
        /// </summary>
        /// <param name="args">Details about the launch request and process.</param>
        protected override void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs args)
        {
            _window = new MainWindow();
            _window.Activate();
        }

        public void LogUserCommandStart(string commandName, string message, DateTime requestTime, string file = null, int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", commandName, message, requestTime);
        }

        public void LogUserCommandStop(string commandName, string message, double elapsedSeconds, string file = null, int line = 0)
        {
            coronaStatusModel.StopMessage(commandName, message, elapsedSeconds);
        }

        public void LogCommandStart(string commandName, string message, DateTime requestTime, string file = null, int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", commandName, message, requestTime);
        }

        public void LogCommandStop(string commandName, string message, double elapsedSeconds, string file = null, int line = 0)
        {
            coronaStatusModel.StopMessage(commandName, message, elapsedSeconds);
        }

        public void LogJobStart(string apiName, string message, DateTime requestTime, string file = null, int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", apiName, message, requestTime);
        }

        public void LogJobStop(string apiName, string message, double elapsedSeconds, string file = null, int line = 0)
        {
            coronaStatusModel.StopMessage(apiName, message, elapsedSeconds);
        }

        public void LogJobSectionStart(string apiName, string message, DateTime requestTime, string file = null, int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", apiName, message, requestTime);
        }

        public void LogJobSectionStop(string apiName, string message, double elapsedSeconds, string file = null, int line = 0)
        {
            coronaStatusModel.StopMessage( apiName, message, elapsedSeconds);
        }

        public void LogFunctionStart(string functionName, string message, DateTime requestTime, string file = null, int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogFunctionStop(string functionName, string message, double elapsedSeconds, string file = null, int line = 0)
        {
            coronaStatusModel.StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogBaseBlockStart(int indent, string functionName, string message, DateTime requestTime, string file = null, int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogBaseBlockStop(int indent, string functionName, string message, double elapsedSeconds, string file = null, int line = 0)
        {
            coronaStatusModel.StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogTableStart(string functionName, string message, DateTime requestTime, string file = null, int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogTableStop(string functionName, string message, double elapsedSeconds, string file = null, int line = 0)
        {
            coronaStatusModel.StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogJsonStart(string functionName, string message, DateTime requestTime, string file = null, int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogJsonStop(string functionName, string message, double elapsedSeconds, string file = null, int line = 0)
        {
            coronaStatusModel.StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogPocoStart(string functionName, string message, DateTime requestTime, string file = null, int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogPocoStop(string functionName, string message, double elapsedSeconds, string file = null, int line = 0)
        {
            coronaStatusModel.StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogBlockStart(string functionName, string message, DateTime requestTime, string file = null, int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogBlockStop(string functionName, string message, double elapsedSeconds, string file = null, int line = 0)
        {
            coronaStatusModel.StopMessage(functionName, message, elapsedSeconds );
        }

        public void LogInformation(string message, string file = null, int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", "information", message, DateTime.Now);
            coronaStatusModel.StopMessage("information", message, 0);
        }

        public void LogActivity(string message, DateTime time, string file = null, int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", "activity", message, DateTime.Now);
            coronaStatusModel.StopMessage("activity", message, 0);
        }

        public void LogActivity(string message, double elapsedSeconds, string file = null, int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", "activity", message, DateTime.Now);
            coronaStatusModel.StopMessage("activity", message, 0);
        }

        public void LogPut(string message, double elapsedSeconds, string file = null, int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", "activity", message, DateTime.Now);
            coronaStatusModel.StopMessage("activity", message, 0);
        }

        public void LogAdapter(string message)
        {
            coronaStatusModel.StartMessage("#c0c0c0", "activity", message, DateTime.Now);
            coronaStatusModel.StopMessage("activity", message, 0);
        }

        public void LogWarning(string message, string file = null, int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", "warning", message, DateTime.Now);
            coronaStatusModel.StopMessage("warning", message, 0);
        }

        public void LogException(string message, string file = null, int line = 0)
        {
            coronaStatusModel.StartMessage("#c0c0c0", "activity", message, DateTime.Now);
            coronaStatusModel.StopMessage("activity", message, 0);
        }

        public void LogJson<T>(T src, int indent = 2)
        {
        }
    }
}
