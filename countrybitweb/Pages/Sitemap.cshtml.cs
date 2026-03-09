using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.RazorPages;
using System.Text;
using System.Xml;

namespace countrybitweb.Pages;

public class SitemapModel : PageModel
{
    public IActionResult OnGet()
    {
        var baseUrl = $"{Request.Scheme}://{Request.Host}";
        
        var urls = new List<SitemapUrl>
        {
            new SitemapUrl { Loc = baseUrl, Priority = 1.0, ChangeFreq = "daily" },
            new SitemapUrl { Loc = $"{baseUrl}/About", Priority = 0.8, ChangeFreq = "monthly" },
            new SitemapUrl { Loc = $"{baseUrl}/Contact", Priority = 0.8, ChangeFreq = "monthly" },
            // Add more URLs as needed
        };

        var xml = GenerateSitemapXml(urls);
        return Content(xml, "application/xml", Encoding.UTF8);
    }

    private string GenerateSitemapXml(List<SitemapUrl> urls)
    {
        var settings = new XmlWriterSettings
        {
            Encoding = Encoding.UTF8,
            Indent = true
        };

        using var stream = new MemoryStream();
        using (var writer = XmlWriter.Create(stream, settings))
        {
            writer.WriteStartDocument();
            writer.WriteStartElement("urlset", "http://www.sitemaps.org/schemas/sitemap/0.9");

            foreach (var url in urls)
            {
                writer.WriteStartElement("url");
                writer.WriteElementString("loc", url.Loc);
                writer.WriteElementString("lastmod", DateTime.UtcNow.ToString("yyyy-MM-dd"));
                writer.WriteElementString("changefreq", url.ChangeFreq);
                writer.WriteElementString("priority", url.Priority.ToString("F1"));
                writer.WriteEndElement();
            }

            writer.WriteEndElement();
            writer.WriteEndDocument();
        }

        return Encoding.UTF8.GetString(stream.ToArray());
    }

    private class SitemapUrl
    {
        public string Loc { get; set; } = string.Empty;
        public double Priority { get; set; }
        public string ChangeFreq { get; set; } = string.Empty;
    }
}