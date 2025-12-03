using Microsoft.UI.Xaml.Media;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CoronaCharts
{
    public interface ISeriesBase
    {
        public string Name { get; set; } 
        public Brush FillBrush { get; set; }
        public Brush BorderBrush { get; set; }
    }
}
