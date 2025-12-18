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
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Dynamic;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
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
    public sealed partial class SearchPage : Page, INotifyPropertyChanged
    {
        public SearchPage()
        {
            this.InitializeComponent();
            this.DataContext = this;
        }

        public ObservableCollection<dynamic> SearchResults { get; } = new ObservableCollection<dynamic>();
        public Dictionary<String, CoronaClass> CoronaClasses { get; } = new Dictionary<string, CoronaClass>();

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

        private ExpandoObject _selectedItem;

        public ExpandoObject SelectedItem
        {
            get => _selectedItem;
            set
            {
                _selectedItem = value;
                OnPropertyChanged();
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged([CallerMemberName] string propertyName ="")
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
                    if (response.Data is IList<object> array)
                    {
                        foreach (var item in array)
                        {
                            if (item is ExpandoObject eitem)
                            {
                                dynamic inbound = eitem;
                                IDictionary<string, object> ditem = (IDictionary<string, object>)eitem;

                                CoronaClass cclass = null;
                                if (CoronaClasses.ContainsKey(inbound.class_name))
                                {
                                    cclass = CoronaClasses[inbound.class_name];
                                }
                                else
                                {
                                    GetClassRequest classrequest = new GetClassRequest
                                    {
                                        ClassName = inbound.class_name,
                                        Token = request.Token
                                    };
                                    var classResponse = App.CurrentApp.CoronaDatabase.GetClass(classrequest);
                                    if (classResponse.Success && classResponse.CoronaClass != null)
                                    {
                                        CoronaClasses[inbound.class_name] = classResponse.CoronaClass;
                                        cclass = classResponse.CoronaClass;
                                    }
                                }
                                if (cclass != null && cclass.CardFields != null)
                                {
                                    List<dynamic> items = new List<dynamic>();
                                    foreach (var field in cclass.CardFields)
                                    {
                                        if (!ditem.ContainsKey(field))
                                            continue;
                                        dynamic itemobj = new ExpandoObject();
                                        itemobj.field_name = field;
                                        itemobj.field_value = ditem[field]?.ToString();
                                        items.Add(itemobj);
                                    }
                                    inbound.items = items;

                                }
                                if (cclass != null && !string.IsNullOrEmpty(cclass.CardTitle))
                                {
                                    inbound.title = ditem[cclass.CardTitle]?.ToString();
                                }
                                SearchResults.Add(inbound);
                            }
                        }
                    }
                    else if (response.Data is ExpandoObject expando)
                    {
                        dynamic inbound = expando;
                        SearchResults.Add(inbound);
                    }
                }
                ErrorControl.BaseResponse = response;
            }
            else
            {
                ErrorControl.BaseResponse = login;
            }
        }

        private void ListView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var selectedItem = e.AddedItems.OfType<dynamic>().FirstOrDefault();
            if (selectedItem != null)
            {
               switch (selectedItem.class_name)
                {
                    case "candidate":
                        DonorControl.Visibility = Visibility.Collapsed;
                        DonorControl.IsEnabled = false;
                        CandidateControl.Visibility = Visibility.Visible;
                        CandidateControl.IsEnabled = true;
                        CandidateControl.Candidate = selectedItem;
                        break;
                    case "donor":
                        DonorControl.Visibility = Visibility.Visible;
                        DonorControl.IsEnabled = true;
                        DonorControl.Donor = selectedItem;
                        CandidateControl.Visibility = Visibility.Collapsed;
                        CandidateControl.IsEnabled = false;
                        break;
                    default:
                        DonorControl.Visibility = Visibility.Collapsed;
                        CandidateControl.Visibility = Visibility.Collapsed;
                        DonorControl.IsEnabled = false;
                        CandidateControl.IsEnabled = false;
                        break;

                }
            }
        }

        private void ListView_ItemClick(object sender, ItemClickEventArgs e)
        {
            
        }
    }
}
