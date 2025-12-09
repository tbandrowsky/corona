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
            Refresh();
        }

        string[] pallet =
        {
            "#1f77b4",
            "#ff7f0e",
            "#2ca02c",
            "#d62728",
            "#9467bd",
            "#8c564b",
            "#e377c2",
            "#7f7f7f",
            "#bcbd22",
            "#17becf"
        };

        public void Refresh()
        {
            historyChartModel = new TimeChartSeriesCollection();
            historyChartModel.Series = App.CurrentApp.CoronaStatusModel.Messages
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
                .GroupBy(a => a.Api + "\n" + a.Topic)
                .Select(g => new SummaryChartSeries
                {
                    Name = g.Key,
                    Value = g.Average(m => m.ElapsedSeconds)
                }).ToList();

            distributionChartModel = new SummaryChartSeriesCollection();
            distributionChartModel.Series = App.CurrentApp.CoronaStatusModel.Messages
                .GroupBy(a => a.Api + "\n" + a.Topic)
                .Select(g => new SummaryChartSeries
                {
                    Name = g.Key,
                    Value = g.Average(m => m.ElapsedSeconds)
                }).ToList();

            historyChart.Series = historyChartModel;
            performanceChart.Series = performanceChartModel;
            distributionChart.Series = distributionChartModel;
        }

        private void OnMessageReceived(CoronaMessage message)
        {
            Refresh();
        }

    }
}
