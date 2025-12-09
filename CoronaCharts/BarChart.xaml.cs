using Microsoft.Graphics.Canvas;
using Microsoft.Graphics.Canvas.UI.Xaml;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
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

namespace CoronaCharts
{
    public sealed partial class BarChart : UserControl, INotifyPropertyChanged
    {
        public BarChart()
        {
            InitializeComponent();
        }

        private string _title;

        public string Title
        {
            get => _title;
            set {
                _title = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Title)));
            }
        }

        SummaryChartSeriesCollection _series;

        public SummaryChartSeriesCollection Series
        {
            get => _series;
            set
            {
                _series = value;
                OnPropertyChanged(nameof(Series));
                Canvas.Invalidate();
            }
        }

        public float BarSpacing = 8;

        public event PropertyChangedEventHandler PropertyChanged;

        void OnPropertyChanged(string name)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

        void Canvas_Draw(Microsoft.Graphics.Canvas.UI.Xaml.CanvasControl sender, Microsoft.Graphics.Canvas.UI.Xaml.CanvasDrawEventArgs args)
        {
            if (Series == null || Series.Series.Count == 0)
                return;

            args.DrawingSession.Clear(Microsoft.UI.Colors.White);
            double maxValue = Series.Series.Max(s => s.Value);
            double minValue = Series.Series.Min(s => s.Value);
            if (minValue > 0) minValue = 0;
            double rangeValue = maxValue - minValue;
            double barAvailableWidth = sender.ActualWidth - BarSpacing * (Series.Series.Count() + 1);
            if (barAvailableWidth > 0 && rangeValue > 0)
            {
                float barWidth = (float)(barAvailableWidth / Series.Series.Count());
                float x = (float)BarSpacing;
                float yscale = (float)(sender.ActualHeight / rangeValue);
                int color_index = 0;

                foreach (var item in Series.Series)
                {
                    float h = (float)(item.Value * yscale + minValue);
                    float y = (float)sender.ActualHeight - h;
                    string series_color = item.FillColor;
                    var fillBrush = new Microsoft.Graphics.Canvas.Brushes.CanvasSolidColorBrush(args.DrawingSession, series_color.ToColor());
                    args.DrawingSession.FillRectangle(x, y, barWidth, h, fillBrush);
                    var borderBrush = new Microsoft.Graphics.Canvas.Brushes.CanvasSolidColorBrush(args.DrawingSession, Microsoft.UI.Colors.Black);
                    args.DrawingSession.DrawRectangle(x, y, barWidth, h, borderBrush, 1);
                    x += barWidth + BarSpacing;
                }
            }
        }
    }
}
