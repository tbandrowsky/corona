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
        SummaryChartSeriesCollection functionChartModel;
        SummaryChartSeriesCollection distributionChartModel;

        private void Page_Loaded(object sender, RoutedEventArgs e)
        {
            App.CurrentApp.CoronaStatusModel.MessageReceived += OnMessageReceived;
        }

        (ICanvasBrush border, ICanvasBrush fill) CreateBrushes(Windows.UI.Color color)
        {
            var border = new CanvasSolidColorBrush(performanceChart.CanvasDevice, Windows.UI.Color.FromArgb(255, (byte)(color.R * 0.7), (byte)(color.G * 0.7), (byte)(color.B * 0.7)));
            var fill = new CanvasSolidColorBrush(performanceChart.CanvasDevice, color);
            return (border, fill);
        }

        private void OnMessageReceived(CoronaMessage message)
        {
            historyChartModel = new TimeChartSeriesCollection();
            historyChartModel.Series = App.CurrentApp.CoronaStatusModel.Messages
                .GroupBy(a => a.Api + "\n" + a.Topic )
                .Select( g => new TimeChartSeries
                {
                    Name = g.Key,
                    Points = g.Select( m => new TimePoint
                    {
                        Time = m.StartTime,
                        Value = m.ElapsedSeconds
                    } ).ToList()
                } ).ToList();

            performanceChart.Series.Series = App.CurrentApp.CoronaStatusModel.Messages
                .GroupBy(a => a.Api + "\n" + a.Topic)
                .Select(g => new SummaryChartSeries
                {
                    Name = g.Key,
                    Value = g.Average(m => m.ElapsedSeconds)
                }).ToList();

            distributionChart.Series.Series = App.CurrentApp.CoronaStatusModel.Messages
                .GroupBy(a => a.Api + "\n" + a.Topic)
                .Select(g => new SummaryChartSeries
                {
                    Name = g.Key,
                    Value = g.Average(m => m.ElapsedSeconds)
                }).ToList();

        }

    }
}
