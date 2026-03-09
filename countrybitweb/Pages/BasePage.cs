using Microsoft.AspNetCore.Mvc.RazorPages;

namespace countrybitweb.Pages;

public class BasePage : PageModel
{
    /// <summary>
    /// Sets SEO metadata for the current page
    /// </summary>
    protected void SetSeoData(
        string title, 
        string description, 
        string? keywords = null, 
        string? ogImage = null)
    {
        ViewData["Title"] = title;
        ViewData["Description"] = description;
        ViewData["Keywords"] = keywords ?? string.Empty;
        
        var fullOgImage = string.IsNullOrEmpty(ogImage) 
            ? $"{Request.Scheme}://{Request.Host}/images/og-default.jpg"
            : ogImage.StartsWith("http") 
                ? ogImage 
                : $"{Request.Scheme}://{Request.Host}{ogImage}";
        
        ViewData["OgImage"] = fullOgImage;
        ViewData["CanonicalUrl"] = $"{Request.Scheme}://{Request.Host}{Request.Path}";
    }
    
    /// <summary>
    /// Sets SEO data with structured data
    /// </summary>
    protected void SetSeoDataWithStructuredData(
        string title,
        string description,
        object structuredData,
        string? keywords = null,
        string? ogImage = null)
    {
        SetSeoData(title, description, keywords, ogImage);
        ViewData["StructuredData"] = System.Text.Json.JsonSerializer.Serialize(structuredData);
    }
}