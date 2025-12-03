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
            set {
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

        public float CanvasWidth = 1000;
        public float CanvasHeight = 1000;

        private void Canvas_CreateResources(CanvasControl sender, Microsoft.Graphics.Canvas.UI.CanvasCreateResourcesEventArgs args)
        {
            offscreen = new CanvasRenderTarget(sender, CanvasWidth, CanvasHeight, sender.Dpi);
        }

        void Canvas_Draw(Microsoft.Graphics.Canvas.UI.Xaml.CanvasControl sender, Microsoft.Graphics.Canvas.UI.Xaml.CanvasDrawEventArgs args)
        {
            using (var chumpy = offscreen.CreateDrawingSession())
            {
                if (Series != null)
                {
                    chumpy.Clear(Microsoft.UI.Colors.White);
                    double maxValue = Series.Series.Max(s => s.Value);
                    double barWidth = Series.Series.Count() 

                    foreach (var item in Series.Series)
                    {
                        float barHeight = (float)(item.Value / totalValue * maxBarHeight);
                        chumpy.FillRectangle(x, y - barHeight, barWidth, barHeight, (item.FillBrush as SolidColorBrush).Color);
                        chumpy.DrawRectangle(x, y - barHeight, barWidth, barHeight, (item.BorderBrush as SolidColorBrush).Color, 2);
                        x += barWidth + 20;
                    }
                }
            }

            args.DrawingSession.DrawEllipse(155, 115, 80, 30, Microsoft.UI.Colors.Black, 3);
            args.DrawingSession.DrawText("Hello, Win2D World!", 100, 100, Microsoft.UI.Colors.Yellow);
        }
    }
}
