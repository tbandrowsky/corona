using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace Politics.models
{
    public class candidate : INotifyPropertyChanged
    {
        private string _full_name = string.Empty;
        public string full_name { get => _full_name; set { _full_name = value; OnPropertyChanged(); } }

        private string _candidate_id = string.Empty;
        public string candidate_id { get => _candidate_id; set { _candidate_id = value; OnPropertyChanged(); } }

        private string _political_party = string.Empty;
        public string political_party { get => _political_party; set { _political_party = value; OnPropertyChanged(); } }

        private string _election_year = string.Empty;
        public string election_year { get => _election_year; set { _election_year = value; OnPropertyChanged(); } }

        private string _street1 = string.Empty;
        public string street1 { get => _street1; set { _street1 = value; OnPropertyChanged(); } }

        private string _street2 = string.Empty;
        public string street2 { get => _street2; set { _street2 = value; OnPropertyChanged(); } }

        private string _city = string.Empty;
        public string city { get => _city; set { _city = value; OnPropertyChanged(); } }

        private string _state = string.Empty;
        public string state { get => _state; set { _state = value; OnPropertyChanged(); } }

        private string _zip = string.Empty;
        public string zip { get => _zip; set { _zip = value; OnPropertyChanged(); } }


        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
