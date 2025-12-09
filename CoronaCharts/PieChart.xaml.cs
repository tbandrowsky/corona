using ABI.Windows.AI.MachineLearning;
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
using System.Numerics;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace CoronaCharts
{
    public sealed partial class PieChart : UserControl
    {

        public PieChart()
        {
            this.InitializeComponent();
            this.HorizontalAlignment = HorizontalAlignment.Stretch;
            this.VerticalAlignment = VerticalAlignment.Stretch;

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

        public float BarSpacing = 32;


        public event PropertyChangedEventHandler PropertyChanged;

        void OnPropertyChanged(string name)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

        private void Canvas_CreateResources(CanvasControl sender, Microsoft.Graphics.Canvas.UI.CanvasCreateResourcesEventArgs args)
        {
        }

        void Canvas_Draw(Microsoft.Graphics.Canvas.UI.Xaml.CanvasControl sender, Microsoft.Graphics.Canvas.UI.Xaml.CanvasDrawEventArgs args)
        {
            if (Series == null || Series.Series.Count == 0)
                return;

            args.DrawingSession.Clear(Microsoft.UI.Colors.White);
            double maxValue = Series.Series.Max(s => s.Value);
            double totalValue = Series.Series.Sum(s => s.Value);
            double angularScale = 360.0 / totalValue;

            if (maxValue > 0 && totalValue > 0)
            {
                double start_angle = 0;

                foreach (var item in Series.Series)
                {
                    var series_color = Series.Palette[Series.Series.IndexOf(item) % Series.Palette.Count];
                    double angle = angularScale * item.Value;
                    CanvasPathBuilder pathBuilder = new CanvasPathBuilder(args.DrawingSession);
                    float cx = (float)(ActualWidth / 2);
                    float cy = (float)(ActualHeight / 2);

                    pathBuilder.BeginFigure(cx, cy);

                    pathBuilder.AddArc(
                        new Vector2((float)ActualWidth / 2, (float)ActualHeight / 2),
                        (float)(ActualWidth / 2),
                        (float)(ActualHeight / 2),
                        (float)(start_angle),
                        (float)(angle)
                    );

                    pathBuilder.AddLine(cx, cy);
                    pathBuilder.EndFigure(CanvasFigureLoop.Closed);

                    var path = CanvasGeometry.CreatePath(pathBuilder);

                    var fillBrush = new Microsoft.Graphics.Canvas.Brushes.CanvasSolidColorBrush(args.DrawingSession, series_color.ToColor());
                    args.DrawingSession.FillGeometry(path, fillBrush);
                    var borderBrush = new Microsoft.Graphics.Canvas.Brushes.CanvasSolidColorBrush(args.DrawingSession, series_color.ToColor());
                    args.DrawingSession.DrawGeometry(path, borderBrush);

                    start_angle += (float)(item.Value * angularScale);
                }
            }
        }
    }
}
