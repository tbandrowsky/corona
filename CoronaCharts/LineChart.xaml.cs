using Microsoft.Graphics.Canvas;
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

        CanvasRenderTarget offscreen;

        public event PropertyChangedEventHandler PropertyChanged;

        void OnPropertyChanged(string name)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

        CanvasRenderTarget offscreen;

        void updateCanvas()
        {
            if (offscreen == null || Series == null || Series.Series.Count == 0)
                return;
            using (var chumpy = offscreen.CreateDrawingSession())
            {
                chumpy.Clear(Microsoft.UI.Colors.White);
                double maxValue = Series.Series.Max(s => s.Value);
                double minValue = Series.Series.Min(s => s.Value);
                if (minValue > 0) minValue = 0;
                double rangeValue = maxValue - minValue;
                double barAvailableWidth = CanvasWidth - BarSpacing * (Series.Series.Count() + 1);
                if (barAvailableWidth > 0 && rangeValue > 0)
                {
                    float barWidth = (float)(barAvailableWidth / Series.Series.Count());
                    float x = (float)BarSpacing;
                    float yscale = (float)(CanvasHeight / rangeValue);

                    foreach (var item in Series.Series)
                    {
                        float h = (float)(item.Value * yscale + minValue);
                        float y = (float)CanvasHeight - h;
                        var fillBrush = item.FillBrush ?? new Microsoft.Graphics.Canvas.Brushes.CanvasSolidColorBrush(chumpy, Microsoft.UI.Colors.Blue);
                        chumpy.FillRectangle(x, y, barWidth, h, fillBrush);
                        var borderBrush = item.BorderBrush ?? new Microsoft.Graphics.Canvas.Brushes.CanvasSolidColorBrush(chumpy, Microsoft.UI.Colors.Black);
                        chumpy.DrawRectangle(x, y, barWidth, h, borderBrush, 2);
                        x += barWidth + BarSpacing;
                    }
                }
            }
        }

        private void Canvas_CreateResources(CanvasControl sender, Microsoft.Graphics.Canvas.UI.CanvasCreateResourcesEventArgs args)
        {
            // Create a fixed-size render target (1024x1024 pixels)
            offscreen = new CanvasRenderTarget(sender, 1024, 1024, sender.Dpi);
        }

        void Canvas_Draw(Microsoft.Graphics.Canvas.UI.Xaml.CanvasControl sender, Microsoft.Graphics.Canvas.UI.Xaml.CanvasDrawEventArgs args)
        {

            args.DrawingSession.DrawEllipse(155, 115, 80, 30, Microsoft.UI.Colors.Black, 3);
            args.DrawingSession.DrawText("Hello, Win2D World!", 100, 100, Microsoft.UI.Colors.Yellow);
        }
    }
}
