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
        public Brush FillBrush { get; set; }
        public Brush BorderBrush { get; set; }
    }

    public class SummaryChartSeriesCollection
    {
        public List<SummaryChartSeries> Series { get; set; } = new List<SummaryChartSeries>();
    }
}
