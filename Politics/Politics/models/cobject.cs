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

    public class cobjectitem
    {
        public string field_name { get; set; } = string.Empty;

        public string field_value { get; set; } = string.Empty;
    }

    public class cobject : INotifyPropertyChanged
    {
        private string _object_id = string.Empty;
        public string object_id { get => _object_id; set { _object_id = value; OnPropertyChanged(); } }


        private string _class_name = string.Empty;
        public string class_name { get => _class_name; set { _class_name = value; OnPropertyChanged(); } }

        private string _title = string.Empty;
        public string title { get => _title; set { _title = value; OnPropertyChanged(); } }

        private ObservableCollection<cobjectitem> _items = new ObservableCollection<cobjectitem>();
        public ObservableCollection<cobjectitem> items { get => _items; set { _items = value; OnPropertyChanged(); } }

        public event PropertyChangedEventHandler? PropertyChanged;
        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
