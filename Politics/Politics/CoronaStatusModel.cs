using Microsoft.UI.Dispatching;
using ObservableCollections;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
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
    public class CoronaMessageGroup
    {
        public string Key { get; init; }
        public ObservableCollection<CoronaMessage> Items { get; set;  } = new();
        public CoronaMessageGroup(string key) => Key = key;
    }

    public delegate void MessageReceivedEvent(CoronaMessage message);

    public class CoronaStatusModel 
    {
        private Dictionary<string, CoronaMessage> ActiveMessages { get; set; } = new Dictionary<string, CoronaMessage>();
        public ObservableCollection<CoronaMessageGroup> GroupedMessages { get; } = new();

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

            var queue = App.CurrentApp.DispatcherQueue;

            queue?.TryEnqueue(() =>
            {
                ActiveMessages[topic] = new_message;
                foreach (var fg in GroupedMessages)
                {
                    if (fg.Key == topic)
                    {
                        fg.Items.Add(new_message);
                        return;
                    }
                }
                var cmg = new CoronaMessageGroup(topic);
                cmg.Items.Add(new_message); 
                GroupedMessages.Add(cmg);
                MessageReceived?.Invoke(new_message);
            });
        }

        public void StopMessage(string topic, string message, double elapsed_seconds)
        {
            CoronaMessage? existingMessage = null;

            var queue = App.CurrentApp.DispatcherQueue;

            queue?.TryEnqueue(() =>
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
    }
}
