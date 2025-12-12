using CoronaCharts;
using CoronaInterface;
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
    public sealed partial class CoronaActivity : UserControl
    {
        public ObservableCollection<CoronaMessage> Messages { get; } = new ObservableCollection<CoronaMessage>();   

        public CoronaActivity()
        {
            InitializeComponent();
            Loaded += CoronaActivity_Loaded;
            SchemaGrid.LoadingRow += (s, e) =>
            {
                var item = e.Row.DataContext as CoronaMessage;
                if (item != null)
                {
                    GlobalPalette palette = GlobalPalette.Current;
                    string color = palette.GetApiColor(item.Api, item.Topic, item.Message.FirstWord());
                    Color c = color.ToColor();
                    e.Row.Background = new SolidColorBrush(c);

                    if (c.R + c.G + c.B < 300)
                    {
                        e.Row.Foreground = new SolidColorBrush(Colors.White);
                    }
                    else
                    {
                        e.Row.Foreground = new SolidColorBrush(Colors.Black);
                    }
                }
            };
        }

        private void CoronaActivity_Loaded(object sender, RoutedEventArgs e)
        {
            DataContext = this;
            foreach (var item in App.CurrentApp.CoronaStatusModel.Messages)
            {
                GlobalPalette.Current.GetApiColor(item.Api, item.Topic, item.Message.FirstWord());
                Messages.Add(item);
            }
            SchemaGrid.ItemsSource = Messages;
            Messages.CollectionChanged += (s, ev) =>
            {
                if (ev.NewItems != null && ev.NewItems.Count > 0)
                {
                    SchemaGrid.ScrollIntoView(ev.NewItems[ev.NewItems.Count - 1], null);
                }
            };
        }

        int messageIndex = 0;

        public void Navigate(LegendItem li)
        {
            if (messageIndex >= Messages.Count)
            {
                messageIndex = 0;
            }
            while ( messageIndex < Messages.Count)
            {
                var msg = Messages[messageIndex];
                if (msg.Api == li.Api && msg.Topic == li.Topic)
                {
                    SchemaGrid.ScrollIntoView(msg, null);
                    break;
                }
                messageIndex++;
            }
            if (messageIndex >= Messages.Count)
            {
                messageIndex = 0;
            }
            else
            {
                messageIndex++;
            }
        }

        public void Refresh()
        {
            Messages.Clear();
            foreach (var item in App.CurrentApp.CoronaStatusModel.Messages)
            {
                if (GlobalPalette.Current.IsSelected(item))
                {
                    Messages.Add(item);
                }
            }
        }
    }
}
