using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using System;
using System.Collections.ObjectModel;
using System.ComponentModel;

namespace Politics
{
    public sealed partial class CoronaErrorControl : UserControl, INotifyPropertyChanged
    {
        public CoronaErrorControl()
        {
            this.InitializeComponent();
        }

        // INotifyPropertyChanged implementation
        public event PropertyChangedEventHandler? PropertyChanged;
        private void OnPropertyChanged(string propertyName) =>
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));

        // Backing fields
        private CoronaInterface.ICoronaBaseResponse _baseResponse;
        private Windows.UI.Color _statusColorBackground = Colors.AliceBlue;
        private Visibility _statusDisplay = Visibility.Collapsed;
        private Windows.UI.Color _statusBorder = Colors.Aquamarine;
        private string _message = string.Empty;
        private double _elapsedSeconds = 0.0;
        private string _elapsedMS = string.Empty;
        public ObservableCollection<CoronaInterface.CoronaError> Errors { get; } = new ObservableCollection<CoronaInterface.CoronaError>();

        public CoronaInterface.ICoronaBaseResponse BaseResponse
        {
            get => _baseResponse;
            set
            {
                _baseResponse = value;
                if (_baseResponse.Success)
                {
                    StatusColorBackground = Colors.Green;
                    StatusBorder = Colors.Black;
                } 
                else
                {
                    StatusColorBackground = Colors.DarkRed;
                    StatusBorder = Colors.Black;
                }
                StatusDisplay = Visibility.Visible;
                Message = _baseResponse.Message;
                ElapsedSeconds = _baseResponse.ExecutionTimeSeconds;
                ElapsedMS = $"{_baseResponse.ExecutionTimeSeconds * 1000:0.##} ms";
                // Update the Errors collection
                Errors.Clear();
                foreach (var error in _baseResponse.Errors)
                {
                    Errors.Add(error);
                }
            }
        }

        // Normal CLR properties (x:Bind targets)
        public Windows.UI.Color StatusColorBackground
        {
            get => _statusColorBackground;
            set
            {
                if (!Equals(_statusColorBackground, value))
                {
                    _statusColorBackground = value;
                    OnPropertyChanged(nameof(StatusColorBackground));
                }
            }
        }

        public string ElapsedMS
        {
            get => _elapsedMS;
            set
            {
                if (_elapsedMS!= value)
                {
                    _elapsedMS = value;
                    OnPropertyChanged(nameof(ElapsedMS));
                }
            }
        }

        public Visibility StatusDisplay
        {
            get => _statusDisplay;
            set
            {
                if (_statusDisplay != value)
                {
                    _statusDisplay = value;
                    OnPropertyChanged(nameof(StatusDisplay));
                }
            }
        }

        public Windows.UI.Color StatusBorder
        {
            get => _statusBorder;
            set
            {
                if (!Equals(_statusBorder, value))
                {
                    _statusBorder = value;
                    OnPropertyChanged(nameof(StatusBorder));
                }
            }
        }

        public string Message
        {
            get => _message;
            set
            {
                if (_message != value)
                {
                    _message = value ?? string.Empty;
                    OnPropertyChanged(nameof(Message));
                }
            }
        }

        public double ElapsedSeconds
        {
            get => _elapsedSeconds;
            set
            {
                if (_elapsedSeconds != value)
                {
                    _elapsedSeconds = 0.0;
                    OnPropertyChanged(nameof(ElapsedSeconds));
                }
            }
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            StatusDisplay = Visibility.Collapsed;
        }
    }
}
