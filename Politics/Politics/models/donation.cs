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

        public string _transaction_date = string.Empty;
        public string TransactionDate
        {
            get => _transaction_date;
            set
            {
                _transaction_date = value;
                OnPropertyChanged(nameof(TransactionDate));
            }
        }

        public decimal _transaction_amount = 0.0M;
        public decimal TransactionAmount { get =>_transaction_amount; 
            set 
            { 
                _transaction_amount = value; 
                OnPropertyChanged(nameof(TransactionAmount)); 
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
