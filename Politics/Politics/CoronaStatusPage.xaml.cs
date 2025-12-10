using CommunityToolkit.HighPerformance.Buffers;
using CoronaCharts;
using Microsoft.Graphics.Canvas.Brushes;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Collections.Generic;
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
    public sealed partial class CoronaStatusPage : Page
    {
        public CoronaStatusPage()
        {
            InitializeComponent();
        }

        TimeChartSeriesCollection historyChartModel;
        SummaryChartSeriesCollection performanceChartModel;
        SummaryChartSeriesCollection distributionChartModel;

        private void Page_Loaded(object sender, RoutedEventArgs e)
        {
            App.CurrentApp.CoronaStatusModel.MessageReceived += OnMessageReceived;
            legendControl.LegendItemChecked += LegendControl_LegendItemChecked;
            Refresh(true);
        }

        private void LegendControl_LegendItemChecked(object? sender, LegendItem e)
        {
            GlobalPalette.Current.Select(e);
            Refresh(false);
        }

        public void Refresh(bool update_legend)
        {
            // Preload colors
            foreach (var message in App.CurrentApp.CoronaStatusModel.Messages)
            {
                GlobalPalette.Current.GetApiColor(message.Api, message.Topic, message.Message.FirstWord());
            }

            historyChartModel = new TimeChartSeriesCollection();
            historyChartModel.Series = App.CurrentApp.CoronaStatusModel.Messages
                .Where(a => GlobalPalette.Current.IsSelected(a))
                .GroupBy(a => a.Api + "\n" + a.Topic)
                .Select(g => new TimeChartSeries
                {
                    Name = g.Key,
                    Points = g.Select(m => new TimePoint
                    {
                        Time = m.StartTime,
                        Value = m.ElapsedSeconds
                    }).ToList()
                }).ToList();

            performanceChartModel = new SummaryChartSeriesCollection();
            performanceChartModel.Series = App.CurrentApp.CoronaStatusModel.Messages
                .Where( a => a.Api == "Function" && a.ElapsedSeconds > 0)
                .Where(a => GlobalPalette.Current.IsSelected(a))
                .GroupBy(a => new { a.Topic, Message = a.Message.FirstWord() })
                .Select(g => new SummaryChartSeries
                {
                    Name = g.Key.Topic,
                    Value = g.Average(m => m.ElapsedSeconds),
                    FillColor = GlobalPalette.Current.GetApiColor("Function", g.Key.Topic, g.Key.Message),
                    BorderColor = GlobalPalette.Current.GetApiColor("Function", g.Key.Topic, g.Key.Message)
                }).ToList();

            distributionChartModel = new SummaryChartSeriesCollection();
            distributionChartModel.Series = App.CurrentApp.CoronaStatusModel.Messages
                .Where(a => a.Api == "Function" && a.ElapsedSeconds > 0)
                .Where(a => GlobalPalette.Current.IsSelected(a))
                .GroupBy(a => new { a.Topic, Message = a.Message.FirstWord() })
                .Select(g => new SummaryChartSeries
                {
                    Name = g.Key.Topic,
                    Value = g.Sum(m => m.ElapsedSeconds),
                    FillColor = GlobalPalette.Current.GetApiColor("Function", g.Key.Topic, g.Key.Message),
                    BorderColor = GlobalPalette.Current.GetApiColor("Function", g.Key.Topic, g.Key.Message)
                }).ToList();

            performanceChart.Series = performanceChartModel;
            distributionChart.Series = distributionChartModel;
            activitygrid.Refresh();

            if (update_legend)
            {
                var legendItems = GlobalPalette.Current.GetLegend();
                foreach (var li in legendItems)
                {
                    var existingli = legendControl.Find(li.Api, li.Topic);
                    if (existingli != null)
                    {
                        li.IsSelected = existingli.IsSelected;
                    }
                }

                legendControl.Items.Clear();
                foreach (var item in legendItems)
                {
                    legendControl.Items.Add(item);
                }
            }
        }

        private void OnMessageReceived(CoronaMessage message)
        {
            DispatcherQueue.TryEnqueue(() =>
                {
                    GlobalPalette.Current.GetApiColor(message.Api, message.Topic, message.Message.FirstWord());
                    Refresh(true);
                }
            );
        }

    }
}
