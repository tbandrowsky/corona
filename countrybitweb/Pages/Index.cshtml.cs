using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.RazorPages;
using Microsoft.Extensions.Logging;

namespace countrybitweb.Pages
{
    public class IndexModel : BasePage
    {
        private readonly ILogger<IndexModel> _logger;

        public IndexModel(ILogger<IndexModel> logger)
        {
            _logger = logger;
        }

        public void OnGet()
        {
            SetSeoData(
                title: "Country Video Games/CountryBit web site",
                description: "Country Video Games, also known as CountryBit, builds Conceptual Art Pieces of Windows Applications.",
                keywords: "countrybit, main keyword, feature keyword, Windows SDK based apps, experimental, art",
                ogImage: "/images/home-og.jpg"
            );
        }
    }
}
