using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Politics.models
{
    public class donation : INotifyPropertyChanged
    {
        public candidate _candidate = new candidate();
        public donor _donor = new donor();
        public candidate candidate
        {
            get => _candidate;
            set
            {
                _candidate = value;
                OnPropertyChanged(nameof(candidate));
            }
        }

        public donor donor
        {
            get => _donor;
            set
            {
                _donor = value;
                OnPropertyChanged(nameof(donor));
            }
        }

        public string _committee_id = string.Empty;
        public string committee_id
        {
            get => _committee_id;
            set
            {
                _committee_id = value;
                OnPropertyChanged(nameof(committee_id));
            }
        }

        public string _transaction_date = string.Empty;
        public string transaction_date
        {
            get => _transaction_date;
            set
            {
                _transaction_date = value;
                OnPropertyChanged(nameof(transaction_date));
            }
        }

        public decimal _transaction_amount = 0.0M;
        public decimal transaction_amount
        {
            get => _transaction_amount;
            set
            {
                _transaction_amount = value;
                OnPropertyChanged(nameof(transaction_amount));
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
