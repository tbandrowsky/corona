using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.RazorPages;

namespace countrybitweb.Pages
{
    public class PoliticsDxModel : BasePage
    {
        public void OnGet()
        {
            SetSeoData(
                title: "Country Video Games - Politics Dx",
                description: "Political search, donor search, donations, bribes, truth, opposition search, political tool to bring truth and freedom to the Galaxy and restore the Republic.",
                keywords: "politicsdx, political search, donor search, donations, bribes, truth, opposition search, truth, freedom, galaxy, republic",
                ogImage: "/images/politicsdx-og.jpg"
            );
        }
    }
}
