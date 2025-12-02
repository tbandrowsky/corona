using CoronaLib;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using ObservableCollections;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI;
using static System.Net.WebRequestMethods;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace Politics
{
    public sealed partial class CoronaSchemaLoad : UserControl
    {
       
        public CoronaSchemaLoad()
        {
            InitializeComponent();
            Loaded += CoronaSchemaLoad_Loaded;
            SchemaGrid.LoadingRow += (s, e) =>
            {
                var item = e.Row.DataContext as CoronaMessage;
                if (item != null)
                {
                    switch (item.Api)
                    {
                        case "UserCommand":
                            e.Row.Background = new SolidColorBrush(Color.FromArgb(255, 10, 60, 20));
                            e.Row.Foreground = new SolidColorBrush(Colors.White);
                            break;
                        case "Command":
                            e.Row.Background = new SolidColorBrush(Color.FromArgb(255, 10, 60, 20));
                            e.Row.Foreground = new SolidColorBrush(Colors.White);
                            break;
                        case "Job":
                            e.Row.Background = new SolidColorBrush(Color.FromArgb(255,0,80,20));
                            e.Row.Foreground = new SolidColorBrush(Colors.White);
                            break;
                        case "JobSection":
                            e.Row.Background = new SolidColorBrush(Color.FromArgb(255,10,100, 20));
                            e.Row.Foreground = new SolidColorBrush(Colors.White);
                            break;
                        case "Function":
                            e.Row.Background = new SolidColorBrush(Colors.DarkSlateBlue);
                            e.Row.Foreground = new SolidColorBrush(Colors.White);
                            break;
                        case "Information":
                            e.Row.Background = new SolidColorBrush(Colors.DarkGray);
                            e.Row.Foreground = new SolidColorBrush(Colors.Black);
                            break;
                        case "Exception":
                            e.Row.Background = new SolidColorBrush(Colors.DarkRed);
                            e.Row.Foreground = new SolidColorBrush(Colors.White);
                            break;
                        case "Warning":
                            e.Row.Background = new SolidColorBrush(Colors.DarkOrange);
                            e.Row.Foreground = new SolidColorBrush(Colors.LightGoldenrodYellow);
                            break;
                        default:
                            e.Row.Background = new SolidColorBrush(Colors.DarkGray);
                            break;
                    }
                }
            };
        }

        private void CoronaSchemaLoad_Loaded(object sender, RoutedEventArgs e)
        {
            DataContext = this;
            App.CurrentApp.CoronaStatusModel.MessageReceived += OnMessageReceived;
            SchemaGrid.ItemsSource = App.CurrentApp.CoronaStatusModel.Messages;
        }

        private void OnMessageReceived(CoronaMessage message)
        {
        }
    }
}
