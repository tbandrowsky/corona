using Microsoft.Graphics.Canvas.Brushes;
using Microsoft.UI.Xaml.Media;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CoronaCharts
{
    public class TimePoint
    {
        public DateTime Time { get; set; }
        public double Value { get; set; }
    }

    public class TimeChartSeries : ISeriesBase
    {
        public string Name { get; set; }
        public List<TimePoint> Points { get; set; } = new List<TimePoint>();
        public ICanvasBrush FillBrush { get; set; }
        public ICanvasBrush BorderBrush { get; set; }
    }

    public class TimeChartSeriesCollection
    {
        public List<TimeChartSeries> Series { get; set; } = new List<TimeChartSeries>();
    }

}
