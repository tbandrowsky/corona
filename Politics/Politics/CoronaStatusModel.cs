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
        private string _color = "";
        private string _topic = "";
        private string _message = "";
        private DateTime _startTime;
        private double _elapsedSeconds;
        public string Color
        {
            get => _color;
            set
            {
                _color = value;
                OnPropertyChanged(nameof(Color));
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
            }
        }

        public double ElapsedSeconds
        {
            get => _elapsedSeconds;
            set
            {
                _elapsedSeconds = value;
                OnPropertyChanged(nameof(ElapsedSeconds));
            }
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
        private Dictionary<string, CoronaMessage> ActiveMessages { get; set; } = new Dictionary<string, CoronaMessage>();
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

        public void StartMessage(string color, string topic, string message, DateTime added)
        {
            var new_message = new CoronaMessage
            {
                Color = color,
                Topic = topic,
                Message = message,
                StartTime = added
            };

            App.CurrentApp
                .DispatcherQueue.TryEnqueue(() =>
                {
                    ActiveMessages[topic] = new_message;
                    Messages.Add(new_message);
                    MessageReceived?.Invoke(new_message);
                });
        }

        public void StopMessage(string topic, string message, double elapsed_seconds)
        {
            CoronaMessage? existingMessage = null;

            var queue = App.CurrentApp.DispatcherQueue;

            App.CurrentApp
                .DispatcherQueue.TryEnqueue(() =>
                {
                    if (ActiveMessages.TryGetValue(topic, out existingMessage))
                    {
                        existingMessage.Message = message;
                        existingMessage.ElapsedSeconds = elapsed_seconds;
                        ActiveMessages.Remove(topic);
                    }
                    if (existingMessage != null)
                    {
                        MessageReceived?.Invoke(existingMessage);
                    }
                });
        }

        public void LogUserCommandStart(string commandName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("#c0c0c0", commandName, message, requestTime);
        }

        public void LogUserCommandStop(string commandName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage(commandName, message, elapsedSeconds);
        }

        public void LogCommandStart(string commandName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("#c0c0c0", commandName, message, requestTime);
        }

        public void LogCommandStop(string commandName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage(commandName, message, elapsedSeconds);
        }

        public void LogJobStart(string apiName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("#c0c0c0", apiName, message, requestTime);
        }

        public void LogJobStop(string apiName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage(apiName, message, elapsedSeconds);
        }

        public void LogJobSectionStart(string apiName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("#c0c0c0", apiName, message, requestTime);
        }

        public void LogJobSectionStop(string apiName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage(apiName, message, elapsedSeconds);
        }

        public void LogFunctionStart(string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogFunctionStop(string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogBaseBlockStart(int indent, string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogBaseBlockStop(int indent, string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogTableStart(string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogTableStop(string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogJsonStart(string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogJsonStop(string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogPocoStart(string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogPocoStop(string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogBlockStart(string functionName, string message, DateTime requestTime, string file = "", int line = 0)
        {
            StartMessage("#c0c0c0", functionName, message, requestTime);
        }

        public void LogBlockStop(string functionName, string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StopMessage(functionName, message, elapsedSeconds);
        }

        public void LogInformation(string message, string file = "", int line = 0)
        {
            StartMessage("#c0c0c0", "information", message, DateTime.Now);
            StopMessage("information", message, 0);
        }

        public void LogActivity(string message, DateTime time, string file = "", int line = 0)
        {
            StartMessage("#c0c0c0", "activity", message, DateTime.Now);
            StopMessage("activity", message, 0);
        }

        public void LogActivity(string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StartMessage("#c0c0c0", "activity", message, DateTime.Now);
            StopMessage("activity", message, 0);
        }

        public void LogPut(string message, double elapsedSeconds, string file = "", int line = 0)
        {
            StartMessage("#c0c0c0", "activity", message, DateTime.Now);
            StopMessage("activity", message, 0);
        }

        public void LogAdapter(string message)
        {
            StartMessage("#c0c0c0", "activity", message, DateTime.Now);
            StopMessage("activity", message, 0);
        }

        public void LogWarning(string message, string file = "", int line = 0)
        {
            StartMessage("#c0c0c0", "warning", message, DateTime.Now);
            StopMessage("warning", message, 0);
        }

        public void LogException(string message, string file = "", int line = 0)
        {
            StartMessage("#c0c0c0", "activity", message, DateTime.Now);
            StopMessage("activity", message, 0);
        }

        public void LogJson<T>(T src, int indent = 2)
        {
        }

    }
}
