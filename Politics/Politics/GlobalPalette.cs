using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Media.MediaProperties;

namespace Politics
{
    internal class GlobalPalette
    {

        public static GlobalPalette Current { get; } = new GlobalPalette();

        internal class ColorMap
        {
            public string Api { get; set; }
            public int ColorIndex { get; set; }
            public int CurrentIndex { get; set; } = 0;

            public Dictionary<string, string> TopicColors = new Dictionary<string, string>();
        }

        public Dictionary<string, ColorMap> ApiColorMapping = new Dictionary<string, ColorMap>
        {
            { "UserCommand", new ColorMap { Api = "UserCommand", ColorIndex = 0 } },
            { "Command", new ColorMap { Api = "Command", ColorIndex = 0 } },
            { "Job", new ColorMap { Api = "Job", ColorIndex = 30 } },
            { "JobSection", new ColorMap { Api = "JobSection", ColorIndex = 30 } },
            { "Function", new ColorMap { Api = "Function", ColorIndex = 40 } },
            { "Information", new ColorMap { Api = "Information", ColorIndex = 100 } },
            { "Exception", new ColorMap { Api = "Exception", ColorIndex = 20 } },
            { "Warning", new ColorMap { Api = "Warning", ColorIndex = 10 } },
        };

        public string GetApiColor( string api, string topic, string message )
        {
            ColorMap map = null;
            if (ApiColorMapping.TryGetValue(api, out map))
            {
                string key = topic + "." + message;
                string color = null;
                if (map.TopicColors.TryGetValue(key, out color))
                {
                    return color;
                }
                else
                {
                    if (map.CurrentIndex == 0)
                    {
                        map.CurrentIndex = map.ColorIndex;
                    }
                    int colorIndex = map.CurrentIndex;
                    color = Palette[colorIndex];
                    map.TopicColors[key] = color;
                    map.CurrentIndex++;
                    if ((map.CurrentIndex - map.ColorIndex) >= 10)
                    {
                        map.CurrentIndex = map.ColorIndex;
                    }
                    return color;
                }
            }
            else
            {
                return GetApiColor("Information", topic, message);
            }
        }

        public List<string> Palette = new List<string>
        {
            // Blue shades
            "#70A5D5", "#5896CE", "#4E79A7", "#3A5C7F", "#253F57",
            "#E8F0F8", "#D0E1F1", "#B8D2EA", "#A0C3E3", "#88B4DC",

            // Orange shades
            "#FFA355", "#F28E2B", "#C26F21", "#8F5118", "#5C3410",
            "#FFF3E6", "#FFE3C9", "#FFD3AC", "#FFC38F", "#FFB372",

            // Red shades
            "#E86D6E", "#E15759", "#B1443F", "#7F3130", "#4D1F20",
            "#FCEAEA", "#F8D1D2", "#F4B8B9", "#F09FA0", "#EC8687",

            // Teal shades
            "#99CFD3", "#88C7CC", "#76B7B2", "#4F7E7B", "#2A4A48",
            "#EEF7F6", "#DDEFEF", "#CCE7E8", "#BBDFE1", "#AAD7DA",

            // Green shades
            "#77C476", "#60BA5F", "#59A14F", "#3E6F38", "#244A22",
            "#EAF6E9", "#D3ECD2", "#BCE2BB", "#A5D8A4", "#8ECE8D",

            // Yellow shades
            "#FFDB66", "#FFD54D", "#EDC948", "#A88A1A", "#6B570F",
            "#FFF9E6", "#FFF3CC", "#FFEDB3", "#FFE799", "#FFE180",

            // Purple shades
            "#CA9BBF", "#C18AB4", "#B07AA1", "#7A546F", "#4A3343",
            "#F7F0F5", "#EEDFEA", "#E5CEDF", "#DCBDD4", "#D3ACCA",

            // Pink shades
            "#FFCCCD", "#FFBFC1", "#FF9DA7", "#A85C63", "#6B383D",
            "#FFF5F6", "#FFEDEE", "#FFE5E6", "#FFDCDD", "#FFD4D5",

            // Brown shades
            "#C3A486", "#B99572", "#9C755F", "#6B4F3F", "#3E2D24",
            "#F5EFEA", "#EBE0D6", "#E1D1C2", "#D7C2AE", "#CDB39A",

            // Gray shades
            "#CAC9C8", "#C1C0BF", "#BAB0AC", "#7A726F", "#4A4543",
            "#F7F6F6", "#EEEDEC", "#E5E4E3", "#DCDBDA", "#D3D2D1",

            // Gray shades
            "#F7F6F6", "#F7F6F6", "#F7F6F6", "#F7F6F6", "#F7F6F6",
            "#F7F6F6", "#F7F6F6", "#F7F6F6", "#F7F6F6", "#F7F6F6"
        };

    }

}
