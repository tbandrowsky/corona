using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Dynamic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using Windows.ApplicationModel.Chat;

namespace Politics.models
{

    public class cobjectitem
    {
        public string field_name { get; set; } = string.Empty;

        public string field_value { get; set; } = string.Empty;
    }

    public class cobject : INotifyPropertyChanged
    {

        private JObject _base_object;
        public JObject BaseObject { get => _base_object; set { _base_object = value; OnPropertyChanged(); } }

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

        public object this[string propertyName]
        {
            get
            {
                if (TryGetMember(propertyName, out object result))
                {
                    return result;
                }
                return string.Empty;
            }
            set
            {
                if (!TrySetMember(propertyName, value))
                {
                    ;
                }
            }
        }

        // Implement the TryGetMember method of the DynamicObject class for dynamic member calls.
        public bool TryGetMember(string name, out object result)
        {
            if (name == "BaseObject")
            {
                result = BaseObject;
            } 
            else if (name == "object_id")
            {
                result = object_id;
            }
            else if (name == "class_name")
            {
                result = class_name;
            }
            else if (name == "title")
            {
                result = title;
            }
            else if (name == "items")
            {
                result = items;
            }
            else if (BaseObject != null && BaseObject.TryGetValue(name, out JToken? token))
            {
                result = token?.ToObject<object>();
            }
            else
            {
                result = null;
            }

            return result != null;
        }

        public bool TrySetMember(string name, object? value)
        {
            if (name == "BaseObject")
            {
                BaseObject = value as JObject;
            }
            else if (name == "object_id")
            {
                object_id = value as string;
            }
            else if (name == "class_name")
            {
                class_name = value as string;
            }
            else if (name == "title")
            {
                title = value as string;
            }
            else if (name == "items")
            {
                items = value as ObservableCollection<cobjectitem>;
            }
            else if (BaseObject != null)
            {
                BaseObject[name] = JToken.FromObject(value);
            }
            else
            {
                return false;
            }

            return true;
        }

    }
}
