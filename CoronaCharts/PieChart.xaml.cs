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
                Refresh();
            }
        }

        public float CanvasWidth = 1000;
        public float CanvasHeight = 1000;
        public float BarSpacing = 32;

        CanvasRenderTarget offscreen;

        public void Refresh()
        {
            if (offscreen == null || Series == null || Series.Series.Count == 0)
                return;
            using (var chumpy = offscreen.CreateDrawingSession())
            {
                chumpy.Clear(Microsoft.UI.Colors.White);
                double maxValue = Series.Series.Max(s => s.Value);
                double totalValue = Series.Series.Sum(s => s.Value);
                double angularScale = 360.0 / totalValue;

                if (maxValue > 0 && totalValue > 0)
                {
                    double start_angle = 0;
                    
                    foreach (var item in Series.Series)
                    {
                        double angle = angularScale * item.Value;
                        CanvasPathBuilder pathBuilder = new CanvasPathBuilder(chumpy);
                        float cx = (float)(CanvasWidth / 2);
                        float cy = (float)(CanvasHeight / 2);

                        pathBuilder.BeginFigure(cx, cy);

                        pathBuilder.AddArc(
                            new Vector2(CanvasWidth / 2, CanvasHeight / 2),
                            (float)(CanvasWidth / 2),
                            (float)(CanvasHeight / 2),
                            (float)(start_angle),
                            (float)(angle)
                        );

                        pathBuilder.AddLine(cx, cy);
                        pathBuilder.EndFigure(CanvasFigureLoop.Closed);

                        var path = CanvasGeometry.CreatePath(pathBuilder);

                        if (item.FillBrush != null)
                        chumpy.FillGeometry(path, item.FillBrush);                        
                        if (item.BorderBrush != null)
                        chumpy.DrawGeometry(path, item.BorderBrush);

                        start_angle += (float)(item.Value * angularScale);
                    }
                }
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        void OnPropertyChanged(string name)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

        private void Canvas_CreateResources(CanvasControl sender, Microsoft.Graphics.Canvas.UI.CanvasCreateResourcesEventArgs args)
        {
            offscreen = new CanvasRenderTarget(sender, CanvasWidth, CanvasHeight, sender.Dpi);
        }

        void Canvas_Draw(Microsoft.Graphics.Canvas.UI.Xaml.CanvasControl sender, Microsoft.Graphics.Canvas.UI.Xaml.CanvasDrawEventArgs args)
        {
            if (offscreen != null)
            {
                args.DrawingSession.DrawImage(offscreen, new Rect(0, 0, CanvasWidth, CanvasHeight));
            }
        }
    }
}
