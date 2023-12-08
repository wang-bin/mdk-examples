using Avalonia;
using Avalonia.Media;

namespace Example;

internal static class FontExtension
{
    internal static AppBuilder UseCHSFonts(this AppBuilder builder)
    {
        return builder.With(new FontManagerOptions
        {
            DefaultFamilyName = "微软雅黑",
            FontFallbacks = new[]
            {
                new FontFallback
                {
                    FontFamily = new FontFamily("Segoe UI")
                },
                new FontFallback
                {
                    FontFamily = new FontFamily("WenQuanYi Micro Hei")
                }
            }
        });
    }
}