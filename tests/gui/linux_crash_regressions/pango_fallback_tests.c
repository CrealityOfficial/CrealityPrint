#include <pango/pangocairo.h>
#include <pango/pangofc-font.h>
#include <fontconfig/fontconfig.h>
#include <stdio.h>

int main(void)
{
    PangoFontMap *map = pango_cairo_font_map_get_default();
    PangoContext *context = pango_font_map_create_context(map);
    PangoFontDescription *description = pango_font_description_from_string("sans 12");
    PangoFont *font = pango_font_map_load_font(map, context, description);
    if (!font || !PANGO_IS_FC_FONT(font))
        return 1;

    // Emulate a substituted font whose family isn't in the map. The public
    // get_face call must return NULL without a GLib critical or a crash.
    FcPattern *pattern = pango_fc_font_get_pattern(PANGO_FC_FONT(font));
    FcPattern *saved = FcPatternDuplicate(pattern);
    FcPatternDel(pattern, FC_FAMILY);
    FcPatternAddString(pattern, FC_FAMILY, (const FcChar8 *)"__creality_missing_font_family__");
    if (pango_font_get_face(font) != NULL)
        return 2;
    FcPatternDel(pattern, FC_FAMILY);
    FcChar8 *family;
    for (int i = 0; FcPatternGetString(saved, FC_FAMILY, i, &family) == FcResultMatch; ++i)
        FcPatternAddString(pattern, FC_FAMILY, family);
    FcPatternDestroy(saved);

    PangoLayout *layout = pango_layout_new(context);
    pango_layout_set_font_description(layout, description);
    // U+1FAEA was being resolved in the original Ubuntu 26 crash.
    pango_layout_set_text(layout, "CrealityPrint \xF0\x9F\xAB\xAA", -1);
    PangoRectangle extent;
    pango_layout_get_extents(layout, &extent, NULL);
    printf("PASS: Pango %s: missing family and U+1FAEA fallback\n", pango_version_string());
    g_object_unref(layout);
    g_object_unref(font);
    pango_font_description_free(description);
    g_object_unref(context);
    return 0;
}
