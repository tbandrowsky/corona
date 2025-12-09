using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CoronaCharts
{
    public static class Extensions
    {
        public static Windows.UI.Color ToColor(this string str)
        {
            if (str == null)
            {
                throw new ArgumentNullException();
            }

            str = str.ToLower();

            if (str.Length == 0)
            {
                return Windows.UI.Color.FromArgb(255, 64, 0, 0);
            }
            else if (str[0] == '#')
            {
                str = str.Substring(1);
            }

            byte[] byteArray = Convert.FromHexString(str);

            if (byteArray.Length == 3)
            {
                return Windows.UI.Color.FromArgb(255, byteArray[0], byteArray[1], byteArray[2]);
            }
            else if (byteArray.Length == 4)
            {
                return Windows.UI.Color.FromArgb(byteArray[3], byteArray[0], byteArray[1], byteArray[2]);
            }
            else
            {
                throw new FormatException("Invalid color format.");
            }
        }

        public static string FirstWord(this string src)
        {
            if (string.IsNullOrEmpty(src))
            {
                return src;
            }
            int index = src.IndexOf(' ');
            if (index == -1)
            {
                return src;
            }
            else
            {
                return src.Substring(0, index);
            }
        }
    }
}