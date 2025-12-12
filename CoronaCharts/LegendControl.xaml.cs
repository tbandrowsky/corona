using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace CoronaCharts
{
    public class LegendItem
    {
        public string Label { get; set; }
        public Color Color { get; set; }
        public string Api { get; set; }
        public string Topic { get; set; }
        public bool IsSelected { get; set; }
    }

    public class LegendByApi : ObservableCollection<LegendItem>
    {
        public string Api { get; set; }
    }

    public sealed partial class LegendControl : UserControl
    {
        public LegendControl()
        {
            InitializeComponent();
            DataContext = this;
            GroupedLegend.Source = LegendByApiItems;
            GroupedLegend.IsSourceGrouped = true;
        }

        public void Add(LegendItem li)
        {
            foreach (var group in LegendByApiItems)
            {
                if (group.Api == li.Api)
                {
                    group.Add(li);
                    return;
                }
            }
            var lba = new LegendByApi
            {
                Api = li.Api
            };
            lba.Add(li);
            LegendByApiItems.Add(lba);
        }
        public LegendItem Find(string api, string topic)
        {
            var item = LegendByApiItems.SelectMany(g => g).FirstOrDefault(i => i.Api == api && i.Topic == topic);
            return item;
        }

        public event EventHandler<LegendItem> NavigateLegend;

        public ObservableCollection<LegendByApi> LegendByApiItems { get; } = new ObservableCollection<LegendByApi>();

        public event EventHandler<LegendItem> LegendItemChecked;

        private void CheckBox_Checked(object sender, RoutedEventArgs e)
        {
            if (sender is CheckBox checkBox && checkBox.DataContext is LegendItem legendItem)
            {
                LegendItemChecked?.Invoke(this, legendItem);
            }
        }

        private void CheckBox_Unchecked(object sender, RoutedEventArgs e)
        {
            if (sender is CheckBox checkBox && checkBox.DataContext is LegendItem legendItem)
            {
                LegendItemChecked?.Invoke(this, legendItem);
            }
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button button && button.DataContext is LegendItem legendItem)
            {
               NavigateLegend?.Invoke(this, legendItem);
            }
        }
    }
}
