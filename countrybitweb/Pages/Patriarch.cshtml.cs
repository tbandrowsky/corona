using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.RazorPages;

namespace countrybitweb.Pages
{
    public class PatriarchModel : BasePage
    {

        public void OnGet()
        {
            SetSeoData(
                title: "Country Video Games - Patriarch",
                description: "Complete family management, appointment, project and home and property tracking application for Windows.",
                keywords: "patriarch, timothy, catholic, church, jesus, christian, family, countryvideogames, divine, art, software divinely inspired, native SDK, Corona, C++",
                ogImage: "/images/patriarch-og.jpg"
            );
        }
    }
}
