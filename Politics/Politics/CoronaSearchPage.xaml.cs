using CoronaInterface;
using CoronaLib;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
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

        private void SearchButton_Click(object sender, RoutedEventArgs e)
        {
            QueryRequest request = new QueryRequest();

            request.QueryBody?.From?.Add(new QueryFrom
            {
                Name = "political_object",
                Filter = new Dictionary<string, string>
                {
                    { "full_text", SearchText }
                }
            });

            request.QueryBody?.Stages?.Add(new QueryResult
            {
                Name = "results",
                Output = "results"
            });
        }

        private void ResultsDataGrid_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {

        }
    }
}
