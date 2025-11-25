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
        private string _color;
        private string _topic;
        private string _message;
        private DateTime _startTime;
        private ObservableCollection<CoronaMessage> _children = new ObservableCollection<CoronaMessage>();
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

        public ObservableCollection<CoronaMessage> Children
        {
            get => _children;
            set
            {
                _children = value;
                OnPropertyChanged(nameof(Children));
            }
        }

        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
        public double ElapsedSeconds { get; set; }

        public event PropertyChangedEventHandler? PropertyChanged;

    }

    public class CoronaStatusModel : INotifyPropertyChanged
    {
        public ObservableStack<CoronaMessage> Messages { get; set; } = new ObservableStack<CoronaMessage>();

        private CoronaMessage? currentMessage;
        public CoronaMessage? CurrentMessage {
            get => currentMessage;
            set
            {
                currentMessage  = value;
                OnPropertyChanged(nameof(CurrentMessage));
            }
        }

        public event PropertyChangedEventHandler? PropertyChanged;

        public void StartMessage(string color, string topic, string message, DateTime added)
        {
            var new_message = new CoronaMessage
            {
                Color = color,
                Topic = topic,
                Message = message,
                StartTime = added
            };
            if (CurrentMessage == null)
            {
                CurrentMessage = new_message;
            }
            else if (CurrentMessage != null)
            {
                CurrentMessage.Children.Add(new_message);
                Messages.Push(CurrentMessage);
            }
        }

        public void StopMessage(string topic, string message, double elapsed_seconds)
        {
            if (CurrentMessage == null)
            {
                return;
            }
            if (CurrentMessage.Topic == topic)
            {
                CurrentMessage.Message = message;
                CurrentMessage.ElapsedSeconds = elapsed_seconds;
                CurrentMessage = Messages.Count > 0 ? Messages.Pop() : null;
            }
        }


        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }



    }
}
