using Microsoft.Graphics.Canvas;
using Microsoft.Graphics.Canvas.Brushes;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CoronaCharts
{

    public class SummaryChartSeries : ISeriesBase
    {
        public string Name { get; set; }
        public double Value { get; set; }
        public string FillColor { get; set; }
        public string BorderColor { get; set; }
    }

    public class SummaryChartSeriesCollection
    {
        public List<string> Palette { get; set; } = new List<string> { "#33691E", "#1e6954", "#35478c", "#012840", "#d23600", "#36175E", "#5E1736", "#281740" }; // Palette

        public List<SummaryChartSeries> Series { get; set; } = new List<SummaryChartSeries>();
    }
}
