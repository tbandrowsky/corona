using Microsoft.Graphics.Canvas;
using Microsoft.Graphics.Canvas.Geometry;
using Microsoft.Graphics.Canvas.UI.Xaml;
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
using System.Reflection.Metadata;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace CoronaCharts
{
    public sealed partial class LineChart : UserControl
    {

        public LineChart()
        {
            this.InitializeComponent();
        }

        private string _title;

        public string Title
        {
            get => _title;
            set
            {
                _title = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Title)));
            }
        }

        TimeChartSeriesCollection _series;

        public TimeChartSeriesCollection Series
        {
            get => _series;
            set
            {
                _series = value;
                OnPropertyChanged(nameof(Series));
                Canvas.Invalidate();
            }
        }

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

            DateTime minDate = DateTime.MaxValue, maxDate = DateTime.MinValue;

            double minValue = 0.0, maxValue = 0.0;

            foreach (var s in Series.Series)
            {
                if (s.Points.Count == 0)
                    continue;

                var list = s.Points.Where(a => a.Time > DateTime.MinValue)
                    .ToList();

                list.Sort((a, b) => a.Time.CompareTo(b.Time));

                if (list.Count == 0)
                    continue;

                var firstPoint = list.First();
                var lastPoint = list.Last();

                minDate = firstPoint.Time;
                maxDate = lastPoint.Time;
                minValue = Math.Min(minValue, firstPoint.Value);
                maxValue = Math.Max(maxValue, lastPoint.Value);
                for (var i = 1; i < list.Count; i++)
                {
                    minValue = Math.Min(minValue, list[i].Value);
                    maxValue = Math.Max(maxValue, list[i].Value);
                }
            }

            double date_range = maxDate.Subtract(minDate).TotalDays;
            double xscale = ActualWidth / date_range;
            double yscale = ActualHeight / (maxValue - minValue);

            foreach (var s in Series.Series)
            {
                if (s.Points.Count == 0)
                    continue;

                var list = s.Points.Where(a => a.Time > DateTime.MinValue)
                    .ToList();
                list.Sort((a, b) => a.Time.CompareTo(b.Time));

                if (list.Count == 0)
                    continue;

                string series_color = s.FillColor;

                CanvasPathBuilder pathBuilder = new CanvasPathBuilder(args.DrawingSession);

                float x = (float)(list[0].Time.Subtract(minDate).TotalDays * xscale);
                float y = (float)(ActualHeight - (list[0].Value - minValue) * yscale);

                pathBuilder.BeginFigure(x, y);

                for (int i = 1; i < list.Count; i++)
                {
                    x = (float)(list[i].Time.Subtract(minDate).TotalDays * xscale);
                    y = (float)(ActualHeight - (list[i].Value - minValue) * yscale);
                    pathBuilder.AddLine(x, y);
                }

                pathBuilder.EndFigure(CanvasFigureLoop.Open);

                var path = CanvasGeometry.CreatePath(pathBuilder);

                var fillBrush = new Microsoft.Graphics.Canvas.Brushes.CanvasSolidColorBrush(args.DrawingSession, series_color.ToColor());
                args.DrawingSession.DrawGeometry(path, fillBrush);
            }
        }
    }
}
