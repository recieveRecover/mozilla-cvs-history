/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim:expandtab:shiftwidth=4:tabstop=4:
 */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Christopher Blizzard
 * <blizzard@mozilla.org>.  Portions created by the Initial Developer
 * are Copyright (C) 2004 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsFont.h"
#include "nsIDeviceContext.h"
#include "nsICharsetConverterManager.h"
#include "nsIPref.h"
#include "nsIServiceManagerUtils.h"

#define PANGO_ENABLE_BACKEND
#define PANGO_ENABLE_ENGINE

#include "nsFontMetricsPango.h"
#include "nsRenderingContextGTK.h"
#include "nsDeviceContextGTK.h"

#include "nsUnicharUtils.h"

#include <pango/pangoxft.h>
#include <fontconfig/fontconfig.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <freetype/tttables.h>

#define FORCE_PR_LOG
#include "prlog.h"

// Globals

static PRLogModuleInfo            *gPangoFontLog;
static int                         gNumInstances;

// Defines

// This is the scaling factor that we keep fonts limited to against
// the display size.  If a pixel size is requested that is more than
// this factor larger than the height of the display, it's clamped to
// that value instead of the requested size.
#define FONT_MAX_FONT_SCALE 2

static NS_DEFINE_CID(kCharsetConverterManagerCID,
                     NS_ICHARSETCONVERTERMANAGER_CID);

struct MozPangoLangGroup {
    const char *mozLangGroup;
    const char *PangoLang;
};

static const MozPangoLangGroup MozPangoLangGroups[] = {
    { "x-western",      "en" },
    { "x-central-euro", "pl" },
    { "x-cyrillic",     "ru" },
    { "x-baltic",       "lv" },
    { "x-devanagari",   "hi" },
    { "x-tamil",        "ta" },
    { "x-unicode",      0    },
    { "x-user-def",     0    },
};

#define NUM_PANGO_LANG_GROUPS (sizeof (MozPangoLangGroups) / \
                               sizeof (MozPangoLangGroups[0]))

#ifdef DEBUG
#define DUMP_PRUNICHAR(ustr, ulen) for (PRUint32 llen=0;llen<ulen;llen++) \
                                      printf("0x%x ", ustr[llen]); \
                                   printf("\n");
#endif

// rounding and truncation functions for a Freetype floating point number 
// (FT26Dot6) stored in a 32bit integer with high 26 bits for the integer
// part and low 6 bits for the fractional part. 
#define MOZ_FT_ROUND(x) (((x) + 32) & ~63) // 63 = 2^6 - 1
#define MOZ_FT_TRUNC(x) ((x) >> 6)
#define CONVERT_DESIGN_UNITS_TO_PIXELS(v, s) \
        MOZ_FT_TRUNC(MOZ_FT_ROUND(FT_MulFix((v) , (s))))

// Static function decls

static PRBool IsASCIIFontName  (const nsString& aName);
static int    FFRECountHyphens (nsACString &aFFREName);

static PangoLanguage *GetPangoLanguage(nsIAtom *aLangGroup);
static const MozPangoLangGroup* FindPangoLangGroup (nsACString &aLangGroup);

static void   FreeGlobals    (void);

static PangoStyle  CalculateStyle  (PRUint8 aStyle);
static PangoWeight CalculateWeight (PRUint16 aWeight);

nsFontMetricsPango::nsFontMetricsPango()
{
    if (!gPangoFontLog)
        gPangoFontLog = PR_NewLogModule("PangoFont");

    gNumInstances++;

    mPangoFontDesc = nsnull;
    mPangoContext = nsnull;
    mLTRPangoContext = nsnull;
    mRTLPangoContext = nsnull;
    mPangoAttrList = nsnull;
    mIsRTL = PR_FALSE;
}

nsFontMetricsPango::~nsFontMetricsPango()
{
    delete mFont;

    if (mDeviceContext)
        mDeviceContext->FontMetricsDeleted(this);

    if (mPangoFontDesc)
        pango_font_description_free(mPangoFontDesc);

    if (mLTRPangoContext)
        g_object_unref(mLTRPangoContext);

    if (mRTLPangoContext)
        g_object_unref(mRTLPangoContext);

    if (mPangoAttrList)
        pango_attr_list_unref(mPangoAttrList);

    // XXX clean up all the pango objects

    if (--gNumInstances == 0)
        FreeGlobals();
}


NS_IMPL_ISUPPORTS1(nsFontMetricsPango, nsIFontMetrics)

// nsIFontMetrics impl

NS_IMETHODIMP
nsFontMetricsPango::Init(const nsFont& aFont, nsIAtom* aLangGroup,
                         nsIDeviceContext *aContext)
{
    mFont = new nsFont(aFont);
    mLangGroup = aLangGroup;

    // Hang on to the device context
    mDeviceContext = aContext;
    
    mPointSize = NSTwipsToFloatPoints(mFont->size);

    // Make sure to clamp the pixel size to something reasonable so we
    // don't make the X server blow up.
    nscoord screenPixels = gdk_screen_height();
    mPointSize = PR_MIN(screenPixels * FONT_MAX_FONT_SCALE, mPointSize);

    // enumerate over the font names passed in
    mFont->EnumerateFamilies(nsFontMetricsPango::EnumFontCallback, this);

    nsCOMPtr<nsIPref> prefService;
    prefService = do_GetService(NS_PREF_CONTRACTID);
    if (!prefService)
        return NS_ERROR_FAILURE;
        
    nsXPIDLCString value;

    // Set up the default font name if it's not set
    if (!mGenericFont) {
        prefService->CopyCharPref("font.default", getter_Copies(value));

        if (value.get())
            mDefaultFont = value.get();
        else
            mDefaultFont = "serif";
        
        mGenericFont = &mDefaultFont;
    }

    // set up the minimum sizes for fonts
    if (mLangGroup) {
        nsCAutoString name("font.min-size.");

        if (mGenericFont->Equals("monospace"))
            name.Append("fixed");
        else
            name.Append("variable");

        name.Append(char('.'));

        const char* langGroup;
        mLangGroup->GetUTF8String(&langGroup);

        name.Append(langGroup);

        PRInt32 minimumInt = 0;
        float minimum;
        nsresult res;
        res = prefService->GetIntPref(name.get(), &minimumInt);
        if (NS_FAILED(res))
            prefService->GetDefaultIntPref(name.get(), &minimumInt);

        if (minimumInt < 0)
            minimumInt = 0;

        minimum = minimumInt;

        // The minimum size is specified in pixels, not in points.
        // Convert the size from pixels to points.
        minimum = NSTwipsToFloatPoints(NSFloatPixelsToTwips(minimum, mDeviceContext->DevUnitsToAppUnits()));
        if (mPointSize < minimum)
            mPointSize = minimum;
    }

    // Make sure that the pixel size is at least greater than zero
    if (mPointSize < 1) {
#ifdef DEBUG
        printf("*** Warning: nsFontMetricsPango created with point size %f\n",
               mPointSize);
#endif
        mPointSize = 1;
    }

    nsresult rv = RealizeFont();
    if (NS_FAILED(rv))
        return rv;

    // Cache font metrics for the 'x' character
    return CacheFontMetrics();
}

nsresult
nsFontMetricsPango::CacheFontMetrics(void)
{
    // Get our scale factor
    float f;
    float val;
    f = mDeviceContext->DevUnitsToAppUnits();

    mPangoAttrList = pango_attr_list_new();

    GList *items = pango_itemize(mPangoContext,
                                 "a", 0, 1, mPangoAttrList, NULL);

    if (!items)
        return NS_ERROR_FAILURE;

    guint nitems = g_list_length(items);
    if (nitems != 1)
        return NS_ERROR_FAILURE;

    PangoItem *item = (PangoItem *)items->data;
    PangoFcFont  *fcfont = PANGO_FC_FONT(item->analysis.font);
    if (!fcfont)
        return NS_ERROR_FAILURE;

    // Get our font face
    FT_Face face;
    TT_OS2 *os2;
    XftFont *xftFont = pango_xft_font_get_font(PANGO_FONT(fcfont));
    if (!xftFont)
        return NS_ERROR_NOT_AVAILABLE;

    face = XftLockFace(xftFont);
    os2 = (TT_OS2 *) FT_Get_Sfnt_Table(face, ft_sfnt_os2);

    // mEmHeight (size in pixels of EM height)
    int size;
    if (FcPatternGetInteger(fcfont->font_pattern, FC_PIXEL_SIZE, 0, &size) !=
        FcResultMatch) {
        size = 12;
    }
    mEmHeight = PR_MAX(1, nscoord(size * f));

    // mMaxAscent
    mMaxAscent = nscoord(xftFont->ascent * f);

    // mMaxDescent
    mMaxDescent = nscoord(xftFont->descent * f);

    nscoord lineHeight = mMaxAscent + mMaxDescent;

    // mLeading (needs ascent and descent and EM height) 
    if (lineHeight > mEmHeight)
        mLeading = lineHeight - mEmHeight;
    else
        mLeading = 0;

    // mMaxHeight (needs ascent and descent)
    mMaxHeight = lineHeight;

    // mEmAscent (needs maxascent, EM height, ascent and descent)
    mEmAscent = nscoord(mMaxAscent * mEmHeight / lineHeight);

    // mEmDescent (needs EM height and EM ascent
    mEmDescent = mEmHeight - mEmAscent;

    // mMaxAdvance
    mMaxAdvance = nscoord(xftFont->max_advance_width * f);

    // mSpaceWidth (width of a space)
    nscoord tmpWidth;
    GetWidth(" ", 1, tmpWidth, NULL);
    mSpaceWidth = tmpWidth;

    // mAveCharWidth (width of an 'average' char)
    //    XftTextExtents16(GDK_DISPLAY(), xftFont, &xUnichar, 1, &extents);
    //rawWidth = extents.width;
    //mAveCharWidth = NSToCoordRound(rawWidth * f);
    GetWidth("x", 1, tmpWidth, NULL);
    mAveCharWidth = tmpWidth;

    // mXHeight (height of an 'x' character)
    PRUnichar xUnichar('x');
    XGlyphInfo extents;
    if (FcCharSetHasChar(xftFont->charset, xUnichar)) {
        XftTextExtents16(GDK_DISPLAY(), xftFont, &xUnichar, 1, &extents);
        mXHeight = extents.height;
    }
    else {
        // 56% of ascent, best guess for non-true type or asian fonts
        mXHeight = nscoord(((float)mMaxAscent) * 0.56);
    }
    mXHeight = nscoord(mXHeight * f);

    // mUnderlineOffset (offset for underlines)
    val = CONVERT_DESIGN_UNITS_TO_PIXELS(face->underline_position,
                                         face->size->metrics.y_scale);
    if (val) {
        mUnderlineOffset = NSToIntRound(val * f);
    }
    else {
        mUnderlineOffset =
            -NSToIntRound(PR_MAX(1, floor(0.1 * xftFont->height + 0.5)) * f);
    }

    // mUnderlineSize (thickness of an underline)
    val = CONVERT_DESIGN_UNITS_TO_PIXELS(face->underline_thickness,
                                         face->size->metrics.y_scale);
    if (val) {
        mUnderlineSize = nscoord(PR_MAX(f, NSToIntRound(val * f)));
    }
    else {
        mUnderlineSize =
            NSToIntRound(PR_MAX(1, floor(0.05 * xftFont->height + 0.5)) * f);
    }

    // mSuperscriptOffset
    if (os2 && os2->ySuperscriptYOffset) {
        val = CONVERT_DESIGN_UNITS_TO_PIXELS(os2->ySuperscriptYOffset,
                                             face->size->metrics.y_scale);
        mSuperscriptOffset = nscoord(PR_MAX(f, NSToIntRound(val * f)));
    }
    else {
        mSuperscriptOffset = mXHeight;
    }

    // mSubscriptOffset
    if (os2 && os2->ySubscriptYOffset) {
        val = CONVERT_DESIGN_UNITS_TO_PIXELS(os2->ySubscriptYOffset,
                                             face->size->metrics.y_scale);
        // some fonts have the incorrect sign. 
        val = (val < 0) ? -val : val;
        mSubscriptOffset = nscoord(PR_MAX(f, NSToIntRound(val * f)));
    }
    else {
        mSubscriptOffset = mXHeight;
    }

    // mStrikeoutOffset
    mStrikeoutOffset = NSToCoordRound(mXHeight / 2.0);

    // mStrikeoutSize
    mStrikeoutSize = mUnderlineSize;

    XftUnlockFace(xftFont);

    /*
    printf("%i\n", mXHeight);
    printf("%i\n", mSuperscriptOffset);
    printf("%i\n", mSubscriptOffset);
    printf("%i\n", mStrikeoutOffset);
    printf("%i\n", mStrikeoutSize);
    printf("%i\n", mUnderlineOffset);
    printf("%i\n", mUnderlineSize);
    printf("%i\n", mMaxHeight);
    printf("%i\n", mLeading);
    printf("%i\n", mEmHeight);
    printf("%i\n", mEmAscent);
    printf("%i\n", mEmDescent);
    printf("%i\n", mMaxAscent);
    printf("%i\n", mMaxDescent);
    printf("%i\n", mMaxAdvance);
    printf("%i\n", mSpaceWidth);
    printf("%i\n", mAveCharWidth);
    */

    return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPango::Destroy()
{
    mDeviceContext = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPango::GetFont(const nsFont *&aFont)
{
    aFont = mFont;
    return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPango::GetLangGroup(nsIAtom** aLangGroup)
{
    *aLangGroup = mLangGroup;
    NS_IF_ADDREF(*aLangGroup);

    return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPango::GetFontHandle(nsFontHandle &aHandle)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

// nsIFontMetricsPango impl

nsresult
nsFontMetricsPango::GetWidth(const char* aString, PRUint32 aLength,
                             nscoord& aWidth,
                             nsRenderingContextGTK *aContext)
{
    PangoLayout *layout = pango_layout_new(mPangoContext);

    pango_layout_set_text(layout, aString, aLength);

    int width, height;

    pango_layout_get_size(layout, &width, &height);

    width /= PANGO_SCALE;

    g_object_unref(layout);

    float f;
    f = mDeviceContext->DevUnitsToAppUnits();
    aWidth = NSToCoordRound(width * f);

    //    printf("GetWidth (char *) %d\n", aWidth);

    return NS_OK;
}

nsresult
nsFontMetricsPango::GetWidth(const PRUnichar* aString, PRUint32 aLength,
                             nscoord& aWidth, PRInt32 *aFontID,
                             nsRenderingContextGTK *aContext)
{
    nsresult rv = NS_OK;
    PangoLayout *layout = pango_layout_new(mPangoContext);

    gchar *text = g_utf16_to_utf8(aString, aLength,
                                  NULL, NULL, NULL);

    if (!text) {
        aWidth = 0;
#ifdef DEBUG
        NS_WARNING("nsFontMetricsPango::GetWidth invalid unicode to follow");
        DUMP_PRUNICHAR(aString, aLength)
#endif
        rv = NS_ERROR_FAILURE;
        goto loser;
    }

    gint width, height;

    pango_layout_set_text(layout, text, strlen(text));
    pango_layout_get_size(layout, &width, &height);

    width /= PANGO_SCALE;

    float f;
    f = mDeviceContext->DevUnitsToAppUnits();
    aWidth = NSToCoordRound(width * f);

    //    printf("GetWidth %d\n", aWidth);

 loser:
    g_free(text);
    g_object_unref(layout);

    return rv;
}


nsresult
nsFontMetricsPango::GetTextDimensions(const PRUnichar* aString,
                                      PRUint32 aLength,
                                      nsTextDimensions& aDimensions, 
                                      PRInt32* aFontID,
                                      nsRenderingContextGTK *aContext)
{
    nsresult rv = NS_OK;

    PangoLayout *layout = pango_layout_new(mPangoContext);

    gchar *text = g_utf16_to_utf8(aString, aLength,
                                  NULL, NULL, NULL);

    if (!text) {
#ifdef DEBUG
        NS_WARNING("nsFontMetricsPango::GetTextDimensions invalid unicode to follow");
        DUMP_PRUNICHAR(aString, aLength)
#endif
        aDimensions.width = 0;
        aDimensions.ascent = 0;
        aDimensions.descent = 0;

        rv = NS_ERROR_FAILURE;
        goto loser;
    }
        

    pango_layout_set_text(layout, text, strlen(text));

    // Get the logical extents
    PangoLayoutLine *line;
    if (pango_layout_get_line_count(layout) != 1) {
        printf("Warning: more than one line!\n");
    }
    line = pango_layout_get_line(layout, 0);

    PangoRectangle rect;
    pango_layout_line_get_extents(line, NULL, &rect);

    float P2T;
    P2T = mDeviceContext->DevUnitsToAppUnits();

    aDimensions.width = NSToCoordRound(rect.width / PANGO_SCALE * P2T);
    aDimensions.ascent = NSToCoordRound(PANGO_ASCENT(rect) / PANGO_SCALE * P2T);
    aDimensions.descent = NSToCoordRound(PANGO_DESCENT(rect) / PANGO_SCALE * P2T);

    //    printf("GetTextDimensions %d %d %d\n", aDimensions.width,
    //aDimensions.ascent, aDimensions.descent);

 loser:
    g_free(text);
    g_object_unref(layout);

    return rv;
}

nsresult
nsFontMetricsPango::GetTextDimensions(const char*         aString,
                                      PRInt32             aLength,
                                      PRInt32             aAvailWidth,
                                      PRInt32*            aBreaks,
                                      PRInt32             aNumBreaks,
                                      nsTextDimensions&   aDimensions,
                                      PRInt32&            aNumCharsFit,
                                      nsTextDimensions&   aLastWordDimensions,
                                      PRInt32*            aFontID,
                                      nsRenderingContextGTK *aContext)
{

    return GetTextDimensionsInternal(aString, aLength, aAvailWidth, aBreaks,
                                     aNumBreaks, aDimensions, aNumCharsFit,
                                     aLastWordDimensions, aContext);

}

nsresult
nsFontMetricsPango::GetTextDimensions(const PRUnichar*    aString,
                                      PRInt32             aLength,
                                      PRInt32             aAvailWidth,
                                      PRInt32*            aBreaks,
                                      PRInt32             aNumBreaks,
                                      nsTextDimensions&   aDimensions,
                                      PRInt32&            aNumCharsFit,
                                      nsTextDimensions&   aLastWordDimensions,
                                      PRInt32*            aFontID,
                                      nsRenderingContextGTK *aContext)
{
    nsresult rv = NS_OK;
    PRInt32 curBreak = 0;
    gchar *curChar;

    PRInt32 *utf8Breaks = new PRInt32[aNumBreaks];

    gchar *text = g_utf16_to_utf8(aString, (PRInt32)aLength,
                                  NULL, NULL, NULL);

    curChar = text;

    if (!text) {
#ifdef DEBUG
        NS_WARNING("nsFontMetricsPango::GetWidth invalid unicode to follow");
        DUMP_PRUNICHAR(aString, (PRUint32)aLength)
#endif
        rv = NS_ERROR_FAILURE;
        goto loser;
    }

    // Covert the utf16 break offsets to utf8 break offsets
    for (PRInt32 curOffset=0; curOffset < aLength;
         curOffset++, curChar = g_utf8_find_next_char(curChar, NULL)) {
        if (aBreaks[curBreak] == curOffset) {
            utf8Breaks[curBreak] = curChar - text;
            curBreak++;
        }

        if (IS_HIGH_SURROGATE(aString[curOffset]))
            curOffset++;
    }

    // Always catch the last break
    utf8Breaks[curBreak] = curChar - text;

#if 0
    if (strlen(text) != aLength) {
        printf("Different lengths for utf16 %d and utf8 %d\n", aLength, strlen(text));
        DUMP_PRUNICHAR(aString, aLength)
        DUMP_PRUNICHAR(text, strlen(text))
        for (PRInt32 i = 0; i < aNumBreaks; ++i) {
            printf("  break %d utf16 %d utf8 %d\n", i, aBreaks[i], utf8Breaks[i]);
        }
    }
#endif

    // We'll use curBreak to indicate which of the breaks end up being
    // used for the break point for this line.
    curBreak = 0;
    rv = GetTextDimensionsInternal(text, strlen(text), aAvailWidth, utf8Breaks,
                                   aNumBreaks, aDimensions, aNumCharsFit,
                                   aLastWordDimensions, aContext);

    // Figure out which of the breaks we ended up using to convert
    // back to utf16 - start from the end.
    for (PRInt32 i = aNumBreaks - 1; i >= 0; --i) {
        if (utf8Breaks[i] == aNumCharsFit) {
            //      if (aNumCharsFit != aBreaks[i])
            //                printf("Fixing utf8 -> utf16 %d -> %d\n", aNumCharsFit, aBreaks[i]);
            aNumCharsFit = aBreaks[i];
            break;
        }
    }

 loser:
    if (text)
        g_free(text);

    delete[] utf8Breaks;

    return rv;
}

nsresult
nsFontMetricsPango::DrawString(const char *aString, PRUint32 aLength,
                               nscoord aX, nscoord aY,
                               const nscoord* aSpacing,
                               nsRenderingContextGTK *aContext,
                               nsDrawingSurfaceGTK *aSurface)
{
    PangoLayout *layout = pango_layout_new(mPangoContext);

    pango_layout_set_text(layout, aString, aLength);

    int x = aX;
    int y = aY;

    aContext->GetTranMatrix()->TransformCoord(&x, &y);

    PangoLayoutLine *line;
    if (pango_layout_get_line_count(layout) != 1) {
        printf("Warning: more than one line!\n");
    }
    line = pango_layout_get_line(layout, 0);

    aContext->UpdateGC();
    GdkGC *gc = aContext->GetGC();

    if (aSpacing && *aSpacing) {
        DrawStringSlowly(aString, NULL, aLength, aSurface->GetDrawable(),
                         gc, x, y, line, aSpacing);
    }
    else {
        gdk_draw_layout_line(aSurface->GetDrawable(), gc,
                             x, y,
                             line);
    }

    g_object_unref(gc);
    g_object_unref(layout);

    //    printf("DrawString (char *)\n");

    return NS_OK;
}

nsresult
nsFontMetricsPango::DrawString(const PRUnichar* aString, PRUint32 aLength,
                               nscoord aX, nscoord aY,
                               PRInt32 aFontID,
                               const nscoord* aSpacing,
                               nsRenderingContextGTK *aContext,
                               nsDrawingSurfaceGTK *aSurface)
{
    nsresult rv = NS_OK;
    int x = aX;
    int y = aY;

    aContext->UpdateGC();
    GdkGC *gc = aContext->GetGC();

    PangoLayout *layout = pango_layout_new(mPangoContext);

    gchar *text = g_utf16_to_utf8(aString, aLength,
                                  NULL, NULL, NULL);

    if (!text) {
#ifdef DEBUG
        NS_WARNING("nsFontMetricsPango::DrawString invalid unicode to follow");
        DUMP_PRUNICHAR(aString, aLength)
#endif
        rv = NS_ERROR_FAILURE;
        goto loser;
    }

    pango_layout_set_text(layout, text, strlen(text));

    aContext->GetTranMatrix()->TransformCoord(&x, &y);

    PangoLayoutLine *line;
    if (pango_layout_get_line_count(layout) != 1) {
        printf("Warning: more than one line!\n");
    }
    line = pango_layout_get_line(layout, 0);

    if (aSpacing && *aSpacing) {
        DrawStringSlowly(text, aString, aLength, aSurface->GetDrawable(),
                         gc, x, y, line, aSpacing);
    }
    else {
        gdk_draw_layout_line(aSurface->GetDrawable(), gc,
                             x, y,
                             line);
    }

 loser:

    g_free(text);
    g_object_unref(gc);
    g_object_unref(layout);

    //    printf("DrawString\n");

    return rv;
}

#ifdef MOZ_MATHML
nsresult
nsFontMetricsPango::GetBoundingMetrics(const char *aString, PRUint32 aLength,
                                       nsBoundingMetrics &aBoundingMetrics,
                                       nsRenderingContextGTK *aContext)
{
    printf("GetBoundingMetrics (char *)\n");
    return NS_ERROR_FAILURE;
}

nsresult
nsFontMetricsPango::GetBoundingMetrics(const PRUnichar *aString,
                                       PRUint32 aLength,
                                       nsBoundingMetrics &aBoundingMetrics,
                                       PRInt32 *aFontID,
                                       nsRenderingContextGTK *aContext)
{
    nsresult rv = NS_OK;
    PangoLayout *layout = pango_layout_new(mPangoContext);

    gchar *text = g_utf16_to_utf8(aString, aLength,
                                  NULL, NULL, NULL);

    if (!text) {
#ifdef DEBUG
        NS_WARNING("nsFontMetricsPango::GetBoundingMetrics invalid unicode to follow");
        DUMP_PRUNICHAR(aString, aLength)
#endif
        aBoundingMetrics.leftBearing = 0;
        aBoundingMetrics.rightBearing = 0;
        aBoundingMetrics.width = 0;
        aBoundingMetrics.ascent = 0;
        aBoundingMetrics.descent = 0;

        rv = NS_ERROR_FAILURE;
        goto loser;
    }

    pango_layout_set_text(layout, text, strlen(text));

    // Get the logical extents
    PangoLayoutLine *line;
    if (pango_layout_get_line_count(layout) != 1) {
        printf("Warning: more than one line!\n");
    }
    line = pango_layout_get_line(layout, 0);

    // Get the ink extents
    PangoRectangle rect;
    pango_layout_line_get_extents(line, NULL, &rect);

    float P2T;
    P2T = mDeviceContext->DevUnitsToAppUnits();

    aBoundingMetrics.leftBearing =
        NSToCoordRound(rect.x / PANGO_SCALE * P2T);
    aBoundingMetrics.rightBearing =
        NSToCoordRound(rect.width / PANGO_SCALE * P2T);
    aBoundingMetrics.width = NSToCoordRound((rect.x + rect.width) / PANGO_SCALE * P2T);
    aBoundingMetrics.ascent = NSToCoordRound(rect.y / PANGO_SCALE * P2T);
    aBoundingMetrics.descent = NSToCoordRound(rect.height / PANGO_SCALE * P2T);

 loser:
    g_free(text);
    g_object_unref(layout);

    return rv;
}

#endif /* MOZ_MATHML */

GdkFont*
nsFontMetricsPango::GetCurrentGDKFont(void)
{
    return nsnull;
}

nsresult
nsFontMetricsPango::SetRightToLeftText(PRBool aIsRTL)
{
    if (aIsRTL) {
        if (!mRTLPangoContext) {
            mRTLPangoContext = pango_xft_get_context(GDK_DISPLAY(), 0);
            pango_context_set_base_dir(mRTLPangoContext, PANGO_DIRECTION_RTL);

            gdk_pango_context_set_colormap(mRTLPangoContext, gdk_rgb_get_cmap());
            pango_context_set_language(mRTLPangoContext, GetPangoLanguage(mLangGroup));
            pango_context_set_font_description(mRTLPangoContext, mPangoFontDesc);
        }
        mPangoContext = mRTLPangoContext;
    }
    else {
        mPangoContext = mLTRPangoContext;
    }

    mIsRTL = aIsRTL;
    return NS_OK;
}

/* static */
PRUint32
nsFontMetricsPango::GetHints(void)
{
    return (NS_RENDERING_HINT_BIDI_REORDERING |
            NS_RENDERING_HINT_ARABIC_SHAPING | 
            NS_RENDERING_HINT_FAST_MEASURE);
}

/* static */
nsresult
nsFontMetricsPango::FamilyExists(nsIDeviceContext *aDevice,
                                 const nsString &aName)
{
    if (!IsASCIIFontName(aName))
        return NS_ERROR_FAILURE;

    NS_ConvertUCS2toUTF8 name(aName);

    nsresult rv = NS_ERROR_FAILURE;
    PangoContext *context = pango_xft_get_context(GDK_DISPLAY(), 0);
    PangoFontFamily **familyList;
    int n;

    pango_context_list_families(context, &familyList, &n);

    for (int i=0; i < n; i++) {
        const char *tmpname = pango_font_family_get_name(familyList[n]);
        if (!Compare(nsDependentCString(tmpname), name,
                     nsCaseInsensitiveCStringComparator())) {
            rv = NS_OK;
            break;
        }
    }

    g_free(familyList);
    g_object_unref(context);

    return rv;
}

// Private Methods

nsresult
nsFontMetricsPango::RealizeFont(void)
{
    nsCString familyList;
    // Create and fill out the font description.
    mPangoFontDesc = pango_font_description_new();

    // Add CSS names - walk the list of fonts, adding the generic as
    // the last font
    for (int i=0; i < mFontList.Count(); ++i) {
        // if this was a generic name, break out of the loop since we
        // don't want to add it to the pattern yet
        if (mFontIsGeneric[i])
            break;;

        nsCString *familyName = mFontList.CStringAt(i);
        familyList.Append(familyName->get());
        familyList.Append(',');
    }

    // If there's a generic add a pref for the generic if there's one
    // set.
    if (mGenericFont && !mFont->systemFont) {
        nsCString name;
        name += "font.name.";
        name += mGenericFont->get();
        name += ".";

        nsString langGroup;
        mLangGroup->ToString(langGroup);

        name.AppendWithConversion(langGroup);

        nsCOMPtr<nsIPref> pref;
        pref = do_GetService(NS_PREF_CONTRACTID);
        if (pref) {
            nsresult rv;
            nsXPIDLCString value;
            rv = pref->GetCharPref(name.get(), getter_Copies(value));

            // we ignore prefs that have three hypens since they are X
            // style prefs.
            if (FFRECountHyphens(value) < 3) {
                nsCString tmpstr;
                tmpstr.Append(value);

                familyList.Append(tmpstr);
                familyList.Append(',');
            }
        }
    }

    // Add the generic if there is one.
    if (mGenericFont && !mFont->systemFont) {
        familyList.Append(mGenericFont->get());
        familyList.Append(',');
    }

    // Set the family
    pango_font_description_set_family(mPangoFontDesc,
                                      familyList.get());

    // Set the point size
    pango_font_description_set_size(mPangoFontDesc,
                                    (gint)(mPointSize * PANGO_SCALE));

    // Set the style
    pango_font_description_set_style(mPangoFontDesc,
                                     CalculateStyle(mFont->style));

    // Set the weight
    pango_font_description_set_weight(mPangoFontDesc,
                                      CalculateWeight(mFont->weight));

    // Now that we have the font description set up, create the
    // context.
    mLTRPangoContext = pango_xft_get_context(GDK_DISPLAY(), 0);
    mPangoContext = mLTRPangoContext;

    // Set the color map so we can draw later.
    gdk_pango_context_set_colormap(mPangoContext, gdk_rgb_get_cmap());

    // Set the pango language now that we have a context
    pango_context_set_language(mPangoContext, GetPangoLanguage(mLangGroup));

    // And attach the font description to this context
    pango_context_set_font_description(mPangoContext, mPangoFontDesc);

    return NS_OK;
}

/* static */
PRBool
nsFontMetricsPango::EnumFontCallback(const nsString &aFamily,
                                     PRBool aIsGeneric, void *aData)
{
    // make sure it's an ascii name, if not then return and continue
    // enumerating
    if (!IsASCIIFontName(aFamily))
        return PR_TRUE;

    nsCAutoString name;
    name.AssignWithConversion(aFamily.get());
    ToLowerCase(name);
    nsFontMetricsPango *metrics = (nsFontMetricsPango *)aData;
    metrics->mFontList.AppendCString(name);
    metrics->mFontIsGeneric.AppendElement((void *)aIsGeneric);
    if (aIsGeneric) {
        metrics->mGenericFont = 
            metrics->mFontList.CStringAt(metrics->mFontList.Count() - 1);
        return PR_FALSE; // stop processing
    }

    return PR_TRUE; // keep processing
}

/*
 * This is only used when there's per-character spacing happening.
 * Well, really it can be either line or character spacing but it's
 * just turtles all the way down!
 */

void
nsFontMetricsPango::DrawStringSlowly(const gchar *aText,
                                     const PRUnichar *aOrigString,
                                     PRUint32 aLength,
                                     GdkDrawable *aDrawable,
                                     GdkGC *aGC, gint aX, gint aY,
                                     PangoLayoutLine *aLine,
                                     const nscoord *aSpacing)
{
    float app2dev;
    app2dev = mDeviceContext->AppUnitsToDevUnits();
    gint offset = 0;

    /*
     * We walk the list of glyphs returned in each layout run,
     * matching up the glyphs with the characters in the source text.
     * We use the aSpacing argument to figure out where to place those
     * glyphs.  It's important to note that since the string we're
     * working with is in UTF-8 while the spacing argument assumes
     * that offset will be part of the UTF-16 string.  Logical
     * attributes in pango are in byte offsets in the UTF-8 string, so
     * we need to store the offsets based on the UTF-8 string.
     */
    nscoord *utf8spacing = new nscoord[strlen(aText)];

    if (aOrigString) {
        const gchar *curChar = aText;
        bzero(utf8spacing, sizeof(nscoord) * strlen(aText));

        // Covert the utf16 spacing offsets to utf8 spacing offsets
        for (PRUint32 curOffset=0; curOffset < aLength;
             curOffset++, curChar = g_utf8_find_next_char(curChar, NULL)) {
            utf8spacing[curChar - aText] = aSpacing[curOffset];

            if (IS_HIGH_SURROGATE(aOrigString[curOffset]))
                curOffset++;
        }
    }
    else {
        memcpy(utf8spacing, aSpacing, (sizeof(nscoord *) * aLength));
    }

    gint curRun = 0;

    for (GSList *tmpList = aLine->runs; tmpList && tmpList->data;
         tmpList = tmpList->next, curRun++) {
        PangoLayoutRun *layoutRun = (PangoLayoutRun *)tmpList->data;
        gint tmpOffset = 0;

        /*        printf("    Rendering run %d: \"%s\"\n", curRun,
                  &aText[layoutRun->item->offset]); */

        for (gint i=0; i < layoutRun->glyphs->num_glyphs; i++) {
            /* printf("glyph %d offset %d orig width %d new width %d\n", i,
             *        layoutRun->glyphs->log_clusters[i] + layoutRun->item->offset,
             *        layoutRun->glyphs->glyphs[i].geometry.width,
             *       (gint)(utf8spacing[layoutRun->glyphs->log_clusters[i] + layoutRun->item->offset] * app2dev * PANGO_SCALE));
             */
            gint thisOffset = (gint)(utf8spacing[layoutRun->glyphs->log_clusters[i] + layoutRun->item->offset]
                                     * app2dev * PANGO_SCALE);
            layoutRun->glyphs->glyphs[i].geometry.width = thisOffset;
            tmpOffset += thisOffset;
        }

        /*        printf("    rendering at X coord %d\n", aX + offset); */

        gdk_draw_glyphs(aDrawable, aGC, layoutRun->item->analysis.font,
                        aX + (gint)(offset / PANGO_SCALE), aY, layoutRun->glyphs);

        offset += tmpOffset;
    }

    delete[] utf8spacing;
}

nsresult
nsFontMetricsPango::GetTextDimensionsInternal(const gchar*        aString,
                                              PRInt32             aLength,
                                              PRInt32             aAvailWidth,
                                              PRInt32*            aBreaks,
                                              PRInt32             aNumBreaks,
                                              nsTextDimensions&   aDimensions,
                                              PRInt32&            aNumCharsFit,
                                              nsTextDimensions&   aLastWordDimensions,
                                              nsRenderingContextGTK *aContext)
{
    NS_PRECONDITION(aBreaks[aNumBreaks - 1] == aLength, "invalid break array");

    // If we need to back up this state represents the last place
    // we could break. We can use this to avoid remeasuring text
    PRInt32 prevBreakState_BreakIndex = -1; // not known
                                            // (hasn't been computed)
    nscoord prevBreakState_Width = 0; // accumulated width to this point

    // Initialize OUT parameters
    GetMaxAscent(aLastWordDimensions.ascent);
    GetMaxDescent(aLastWordDimensions.descent);
    aLastWordDimensions.width = -1;
    aNumCharsFit = 0;

    // Iterate each character in the string and determine which font to use
    nscoord width = 0;
    PRInt32 start = 0;
    nscoord aveCharWidth;
    GetAveCharWidth(aveCharWidth);

    while (start < aLength) {
        // Estimate how many characters will fit. Do that by
        // diving the available space by the average character
        // width. Make sure the estimated number of characters is
        // at least 1
        PRInt32 estimatedNumChars = 0;

        if (aveCharWidth > 0)
            estimatedNumChars = (aAvailWidth - width) / aveCharWidth;

        if (estimatedNumChars < 1)
            estimatedNumChars = 1;

        // Find the nearest break offset
        PRInt32 estimatedBreakOffset = start + estimatedNumChars;
        PRInt32 breakIndex;
        nscoord numChars;

        // Find the nearest place to break that is less than or equal to
        // the estimated break offset
        if (aLength <= estimatedBreakOffset) {
            // All the characters should fit
            numChars = aLength - start;
            breakIndex = aNumBreaks - 1;
        } 
        else {
            breakIndex = prevBreakState_BreakIndex;
            while (((breakIndex + 1) < aNumBreaks) &&
                   (aBreaks[breakIndex + 1] <= estimatedBreakOffset)) {
                ++breakIndex;
            }

            if (breakIndex == prevBreakState_BreakIndex) {
                ++breakIndex; // make sure we advanced past the
                // previous break index
            }

            numChars = aBreaks[breakIndex] - start;
        }

        // Measure the text
        nscoord twWidth = 0;
        if ((1 == numChars) && (aString[start] == ' '))
            GetSpaceWidth(twWidth);
        else if (numChars > 0)
            GetWidth(&aString[start], numChars, twWidth, aContext);

        // See if the text fits
        PRBool  textFits = (twWidth + width) <= aAvailWidth;

        // If the text fits then update the width and the number of
        // characters that fit
        if (textFits) {
            aNumCharsFit += numChars;
            width += twWidth;
            start += numChars;

            // This is a good spot to back up to if we need to so remember
            // this state
            prevBreakState_BreakIndex = breakIndex;
            prevBreakState_Width = width;
        }
        else {
            // See if we can just back up to the previous saved
            // state and not have to measure any text
            if (prevBreakState_BreakIndex > 0) {
                // If the previous break index is just before the
                // current break index then we can use it
                if (prevBreakState_BreakIndex == (breakIndex - 1)) {
                    aNumCharsFit = aBreaks[prevBreakState_BreakIndex];
                    width = prevBreakState_Width;
                    break;
                }
            }

            // We can't just revert to the previous break state
            if (0 == breakIndex) {
                // There's no place to back up to, so even though
                // the text doesn't fit return it anyway
                aNumCharsFit += numChars;
                width += twWidth;
                break;
            }

            // Repeatedly back up until we get to where the text
            // fits or we're all the way back to the first word
            width += twWidth;
            while ((breakIndex >= 1) && (width > aAvailWidth)) {
                twWidth = 0;
                start = aBreaks[breakIndex - 1];
                numChars = aBreaks[breakIndex] - start;

                if ((1 == numChars) && (aString[start] == ' '))
                    GetSpaceWidth(twWidth);
                else if (numChars > 0)
                    GetWidth(&aString[start], numChars, twWidth,
                             aContext);
                width -= twWidth;
                aNumCharsFit = start;
                breakIndex--;
            }
            break;
        }
    }

    aDimensions.width = width;
    GetMaxAscent(aDimensions.ascent);
    GetMaxDescent(aDimensions.descent);

    /*    printf("aDimensions %d %d %d aLastWordDimensions %d %d %d aNumCharsFit %d\n",
           aDimensions.width, aDimensions.ascent, aDimensions.descent,
           aLastWordDimensions.width, aLastWordDimensions.ascent, aLastWordDimensions.descent,
           aNumCharsFit); */

    return NS_OK;
}

/* static */
PRBool
IsASCIIFontName(const nsString& aName)
{
    PRUint32 len = aName.Length();
    const PRUnichar* str = aName.get();
    for (PRUint32 i = 0; i < len; i++) {
        /*
         * X font names are printable ASCII, ignore others (for now)
         */
        if ((str[i] < 0x20) || (str[i] > 0x7E)) {
            return PR_FALSE;
        }
    }
  
    return PR_TRUE;
}

/* static */
int
FFRECountHyphens (nsACString &aFFREName)
{
    int h = 0;
    PRInt32 hyphen = 0;
    while ((hyphen = aFFREName.FindChar('-', hyphen)) >= 0) {
        ++h;
        ++hyphen;
    }
    return h;
}

/* static */
PangoLanguage *
GetPangoLanguage(nsIAtom *aLangGroup)
{
    // Find the FC lang group for this lang group
    nsCAutoString cname;
    aLangGroup->ToUTF8String(cname);

    // see if the lang group needs to be translated from mozilla's
    // internal mapping into fontconfig's
    const struct MozPangoLangGroup *langGroup;
    langGroup = FindPangoLangGroup(cname);

    // if there's no lang group, just use the lang group as it was
    // passed to us
    //
    // we're casting away the const here for the strings - should be
    // safe.
    if (!langGroup)
        return pango_language_from_string(cname.get());
    else if (langGroup->PangoLang) 
        return pango_language_from_string(langGroup->PangoLang);

    return pango_language_from_string("en");
}

/* static */
const MozPangoLangGroup*
FindPangoLangGroup (nsACString &aLangGroup)
{
    for (unsigned int i=0; i < NUM_PANGO_LANG_GROUPS; ++i) {
        if (aLangGroup.Equals(MozPangoLangGroups[i].mozLangGroup,
                              nsCaseInsensitiveCStringComparator())) {
            return &MozPangoLangGroups[i];
        }
    }

    return nsnull;
}

/* static */
void
FreeGlobals(void)
{
}

/* static */
PangoStyle
CalculateStyle(PRUint8 aStyle)
{
    switch(aStyle) {
    case NS_FONT_STYLE_ITALIC:
        return PANGO_STYLE_OBLIQUE;
        break;
    case NS_FONT_STYLE_OBLIQUE:
        return PANGO_STYLE_OBLIQUE;
        break;
    }

    return PANGO_STYLE_NORMAL;
}

/* static */
PangoWeight
CalculateWeight (PRUint16 aWeight)
{
    /*
     * weights come in two parts crammed into one
     * integer -- the "base" weight is weight / 100,
     * the rest of the value is the "offset" from that
     * weight -- the number of steps to move to adjust
     * the weight in the list of supported font weights,
     * this value can be negative or positive.
     */
    PRInt32 baseWeight = (aWeight + 50) / 100;
    PRInt32 offset = aWeight - baseWeight * 100;

    /* clip weights to range 0 to 9 */
    if (baseWeight < 0)
        baseWeight = 0;
    if (baseWeight > 9)
        baseWeight = 9;

    /* Map from weight value to fcWeights index */
    static int fcWeightLookup[10] = {
        0, 0, 0, 0, 1, 1, 2, 3, 3, 4,
    };

    PRInt32 fcWeight = fcWeightLookup[baseWeight];

    /*
     * adjust by the offset value, make sure we stay inside the 
     * fcWeights table
     */
    fcWeight += offset;

    if (fcWeight < 0)
        fcWeight = 0;
    if (fcWeight > 4)
        fcWeight = 4;

    /* Map to final PANGO_WEIGHT value */
    static int fcWeights[5] = {
        349,
        499,
        649,
        749,
        999
    };

    return (PangoWeight)fcWeights[fcWeight];
}


