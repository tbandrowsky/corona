using CoronaInterface;
using CoronaLib;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using Newtonsoft.Json.Linq;
using Politics.models;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Foundation.Collections;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace Politics
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class CoronaSearchPage : Page, INotifyPropertyChanged
    {
        public ObservableCollection<cobject> SearchResults { get; } = new ObservableCollection<cobject>();
        public Dictionary<String, CoronaClass> CoronaClasses { get; } = new Dictionary<string, CoronaClass>();
        public CoronaSearchPage()
        {
            this.InitializeComponent();
            this.DataContext = this;
        }

        private string _searchText;

        public string SearchText
        {
            get => _searchText;
            set 
            {
                _searchText = value;
                OnPropertyChanged(nameof(SearchText));
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        private async void SearchButton_Click(object sender, RoutedEventArgs e)
        {
            QueryRequest request = new QueryRequest();

            request.From?.Add(new QueryFrom
            {
                Name = "political_object",
                ClassName = "political_object",
                Filter = new Dictionary<string, string>
                {
                    { "full_text", SearchText }
                }
            });

            request.Stages?.Add(new QueryResult
            {
                Name = "results",
                Input = "political_object",
                Output = "results"
            });

            var login = App.CurrentApp.CoronaDatabase.LoginLocal(Environment.UserName, "local@localhost.com");

            if (login.Success)
            {
                request.Token = login.Token;
                var response = await App.CurrentApp.CoronaDatabase.QueryAsync(request);
                SearchResults.Clear();
                if (response.Success && response.Data != null)
                {
                    if (response.Data is JArray array)
                    {
                        foreach (var item in array)
                        {
                            cobject obj = new cobject();
                            obj.object_id = item["object_id"]?.ToString();
                            obj.class_name = item["class_name"]?.ToString();
                            CoronaClass cclass = null;
                            if (CoronaClasses.ContainsKey(obj.class_name))
                            {
                                cclass = CoronaClasses[obj.class_name];
                            }
                            else
                            {
                                GetClassRequest classrequest = new GetClassRequest
                                {
                                    ClassName = obj.class_name,
                                    Token = request.Token
                                };
                                var classResponse = App.CurrentApp.CoronaDatabase.GetClass(classrequest);
                                if (classResponse.Success && classResponse.CoronaClass != null)
                                {
                                    CoronaClasses[obj.class_name] = classResponse.CoronaClass;
                                    cclass = classResponse.CoronaClass;
                                }
                            }
                            if (cclass != null && cclass.CardFields != null)
                            {
                                foreach (var field in cclass.CardFields)
                                {
                                    cobjectitem itemobj = new cobjectitem();
                                    itemobj.field_name = field;
                                    itemobj.field_value = item[field]?.ToString();
                                    obj.items.Add(itemobj);
                                }
                            }
                            if (cclass != null && !string.IsNullOrEmpty(cclass.CardTitle))
                            {
                                obj.title = item[cclass.CardTitle]?.ToString();
                            }
                            SearchResults.Add(obj);
                        }
                    }
                    else
                    {
                        cobject obj = new cobject();
                        obj.object_id = response.Data["object_id"]?.ToString();
                        obj.class_name = response.Data["class_name"]?.ToString();
                        SearchResults.Add(obj);
                    }
                }
                ErrorControl.BaseResponse = response;
            }
            else
            {
                ErrorControl.BaseResponse = login;
            }
       }

        private void ResultsDataGrid_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {

        }
    }
}
