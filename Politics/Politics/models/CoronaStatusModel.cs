using CoronaInterface;
using Microsoft.UI.Dispatching;
using ObservableCollections;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Politics
{

    public class CoronaMessage : INotifyPropertyChanged
    {
        private string _api = "";
        private string _topic = "";
        private string _message = "";
        private DateTime _startTime;
        private double _elapsedSeconds;
        public string Api
        {
            get => _api;
            set
            {
                _api = value;
                OnPropertyChanged(nameof(Api));
            }
        }

        public string Topic
        {
            get => _topic;
            set
            {
                _topic = value;
                OnPropertyChanged(nameof(Topic));
            }
        }

        public string Message
        {
            get => _message;
            set
            {
                _message = value;
                OnPropertyChanged(nameof(Message));
            }
        }

        public DateTime StartTime
        {
            get => _startTime;
            set
            {
                _startTime = value;
                OnPropertyChanged(nameof(StartTime));
                OnPropertyChanged(nameof(StartTimeString));
            }
        }

        public string StartTimeString
        {
            get => _startTime.Year > 1900 ? string.Format("{0:M/d h:mm:ss tt}", _startTime) : "";
        }

        public double ElapsedSeconds
        {
            get => _elapsedSeconds;
            set
            {
                _elapsedSeconds = value;
                OnPropertyChanged(nameof(ElapsedSeconds));
                OnPropertyChanged(nameof(ElapsedMillisecondsString));
            }
        }

        public string ElapsedMillisecondsString
        {
            get => _elapsedSeconds > 0 ? string.Format("{0:.2}", _elapsedSeconds * 1000) : "";
        }

        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public event PropertyChangedEventHandler? PropertyChanged;

    }

    public delegate void MessageReceivedEvent(CoronaMessage message);

    public class CoronaStatusModel : ISystemMonitoring
    {
        public ObservableCollection<CoronaMessage> Messages{ get; } = new();

        public MessageReceivedEvent? MessageReceived;

        private CoronaMessage? currentMessage;
        public CoronaMessage? CurrentMessage 
        {
            get => currentMessage;
            set
            {
                currentMessage  = value;
            }
        }

        public void StartMessage(string api, string topic, string message)
        {
            StartMessage(api, topic, message, DateTime.Now);
        }

        public void StartMessage(string api, string topic, string message, DateTime added)
        {
            var new_message = new CoronaMessage
            {
                Api = api,
                Topic = topic,
                Message = message,
                StartTime = added
            };

            App.CurrentApp
                ?.DispatcherQueue.TryEnqueue(() =>
                {
                    Messages.Add(new_message);
                    MessageReceived?.Invoke(new_message);
                });
        }

        public void StopMessage(string api, string topic, string message, double elapsed_seconds)
        {
            var new_message = new CoronaMessage
            {
                Api = api,
                Topic = topic,
                Message = message,
                ElapsedSeconds = elapsed_seconds
            };

            if (App.CurrentApp == null)
                return;

            App.CurrentApp
                .DispatcherQueue.TryEnqueue(() =>
                {
                    Messages.Add(new_message);
                    MessageReceived?.Invoke(new_message);
                });
        }

        public void LogUserCommandStart(string commandName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("UserCommand", commandName, message, requestTime);
        }

        public void LogUserCommandStop(string commandName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage("UserCommand", commandName, message, elapsedSeconds);
        }

        public void LogCommandStart(string commandName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("Command", commandName, message, requestTime);
        }

        public void LogCommandStop(string commandName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage("Command", commandName, message, elapsedSeconds);
        }

        public void LogJobStart(string apiName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("Job", apiName, message, requestTime);
        }

        public void LogJobStop(string apiName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage("Job", apiName, message, elapsedSeconds);
        }

        public void LogJobSectionStart(string apiName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("JobSection", apiName, message, requestTime);
        }

        public void LogJobSectionStop(string apiName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage("JobSection", apiName, message, elapsedSeconds);
        }

        public void LogFunctionStart(string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("Function", functionName, message, requestTime);
        }

        public void LogFunctionStop(string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage("Function", functionName, message, elapsedSeconds);
        }

        public void LogBaseBlockStart(int indent, string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("Block", functionName, message, requestTime);
        }

        public void LogBaseBlockStop(int indent, string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage("Block", functionName, message, elapsedSeconds);
        }

        public void LogTableStart(string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("Table", functionName, message, requestTime);
        }

        public void LogTableStop(string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage("Table", functionName, message, elapsedSeconds);
        }

        public void LogJsonStart(string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("Json", functionName, message, requestTime);
        }

        public void LogJsonStop(string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage("Json", functionName, message, elapsedSeconds);
        }

        public void LogPocoStart(string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("Poco", functionName, message, requestTime);
        }

        public void LogPocoStop(string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage("Poco", functionName, message, elapsedSeconds);
        }

        public void LogBlockStart(string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("Block", functionName, message, requestTime);
        }

        public void LogBlockStop(string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage("Block", functionName, message, elapsedSeconds);
        }

        public void LogInformation(string message, string file = "", int line = 0)
        {
            StartMessage("Information", "", message);
        }

        public void LogActivity(string message, DateTime time, string file = "", int line = 0)
        {
            StartMessage("Activity", "", message, DateTime.Now);
        }

        public void LogActivity(string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage("Activity", "", message, elapsedSeconds);
        }

        public void LogPut(string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StartMessage("Put", "", message, DateTime.Now);
        }

        public void LogAdapter(string message)
        {
            StartMessage("Adapter", "", message, DateTime.Now);
        }

        public void LogWarning(string message, string file = "", int line = 0)
        {
            StartMessage("Warning", "warning", message, DateTime.Now);
        }

        public void LogException(string message, string file = "", int line = 0)
        {
            StartMessage("Exception", "exception", message, DateTime.Now);
        }

        public void LogJson<T>(T src, int indent = 2)
        {
        }

    }
}
