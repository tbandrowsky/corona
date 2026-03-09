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
                title: "Country Video Games - Home",
                description: "Country Video Games builds Windows Applications like no one else.",
                keywords: "countrybit, main keyword, feature keyword, Windows SDK based apps, experimental, art",
                ogImage: "/images/home-og.jpg"
            );
        }
    }
}
