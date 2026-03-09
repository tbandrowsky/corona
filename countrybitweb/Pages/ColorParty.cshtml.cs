using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.RazorPages;

namespace countrybitweb.Pages
{
    public class ColorPartyModel : BasePage
    {
        public void OnGet()
        {
            SetSeoData(
                title: "Country Video Games - Color Party",
                description: "Color Party is a fun and engaging game that allows players to explore their creativity through color.",
                keywords: "color party, creativity, fun, engaging, game, family, children, parties, gatherings, community, fellowship"
            );
        }
    }
}

