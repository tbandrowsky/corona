using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace Politics.models
{
    public class donor : INotifyPropertyChanged
    {
        private string _full_name = string.Empty;
        public string full_name { get => _full_name; set { _full_name = value; OnPropertyChanged(); } }

        private string _employer = string.Empty;
        public string employer { get => _employer; set { _employer = value; OnPropertyChanged(); } }

        private string _occupation = string.Empty;
        public string occupation { get => _occupation; set { _occupation = value; OnPropertyChanged(); } }

        private string _candidate_id = string.Empty;
        public string candidate_id { get => _candidate_id; set { _candidate_id = value; OnPropertyChanged(); } }

        private string _city = string.Empty;
        public string city { get => _city; set { _city = value; OnPropertyChanged(); } }

        private string _state = string.Empty;
        public string state { get => _state; set { _state = value; OnPropertyChanged(); } }

        private string _zip = string.Empty;
        public string zip { get => _zip; set { _zip = value; OnPropertyChanged(); } }

        public event PropertyChangedEventHandler? PropertyChanged;

        private ObservableCollection<donation> _donations = new ObservableCollection<donation>();

        public ObservableCollection<donation> donations
        {
            get => _donations;
            set
            {
                _donations = value;
                OnPropertyChanged();
            }
        }

        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
