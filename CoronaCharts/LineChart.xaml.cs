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
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        void OnPropertyChanged(string name)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

        public float CanvasWidth = 1000;
        public float CanvasHeight = 1000;

        CanvasRenderTarget offscreen;

        public void Refresh()
        {
            if (offscreen == null || Series == null || Series.Series.Count == 0)
                return;

            using (var chumpy = offscreen.CreateDrawingSession())
            {
                chumpy.Clear(Microsoft.UI.Colors.White);

                DateTime minDate = DateTime.MaxValue, maxDate = DateTime.MinValue;

                double minValue = 0.0, maxValue = 0.0;

                foreach (var s in Series.Series) 
                {
                    if (s.Points.Count == 0)
                        continue;
                    s.Points.Sort((a, b) => a.Time.CompareTo(b.Time));
                    minDate = s.Points.First().Time;
                    maxDate = s.Points.Last().Time;
                    minValue = Math.Min(minValue, s.Points[0].Value);
                    maxValue = Math.Max(maxValue, s.Points[0].Value);
                    for (var i = 1; i < s.Points.Count; i++)
                    {
                        minValue = Math.Min(minValue, s.Points[i].Value);
                        maxValue = Math.Max(maxValue, s.Points[i].Value);
                    }
                }

                double date_range = maxDate.Subtract(minDate).TotalDays;
                double xscale = CanvasWidth / date_range;
                double yscale = CanvasHeight / (maxValue - minValue);

                foreach (var s in Series.Series)
                {
                    if (s.Points.Count == 0)
                        continue;

                    CanvasPathBuilder pathBuilder = new CanvasPathBuilder(chumpy);

                    float x = (float)(s.Points[0].Time.Subtract(minDate).TotalDays * xscale);
                    float y = (float)(CanvasHeight - (s.Points[0].Value - minValue) * yscale);

                    pathBuilder.BeginFigure(x, y);

                    for (int i = 1; i < s.Points.Count; i++)
                    {
                        x = (float)(s.Points[i].Time.Subtract(minDate).TotalDays * xscale);
                        y = (float)(CanvasHeight - (s.Points[i].Value - minValue) * yscale);
                        pathBuilder.AddLine(x, y);
                    }

                    pathBuilder.EndFigure(CanvasFigureLoop.Open);

                    var path = CanvasGeometry.CreatePath(pathBuilder);

                    if (s.BorderBrush != null)
                        chumpy.DrawGeometry(path, s.BorderBrush);
                }

            }
        }

        private void Canvas_CreateResources(CanvasControl sender, Microsoft.Graphics.Canvas.UI.CanvasCreateResourcesEventArgs args)
        {
            offscreen = new CanvasRenderTarget(sender, CanvasWidth, CanvasHeight, sender.Dpi);
        }

        void Canvas_Draw(Microsoft.Graphics.Canvas.UI.Xaml.CanvasControl sender, Microsoft.Graphics.Canvas.UI.Xaml.CanvasDrawEventArgs args)
        {

            args.DrawingSession.DrawEllipse(155, 115, 80, 30, Microsoft.UI.Colors.Black, 3);
            args.DrawingSession.DrawText("Hello, Win2D World!", 100, 100, Microsoft.UI.Colors.Yellow);
        }
    }
}
