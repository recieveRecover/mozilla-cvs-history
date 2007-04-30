/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Stuart Parmenter <pavlov@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include "nsGdkUtils.h"
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>

void
my_gdk_draw_text(GdkDrawable *drawable,
                 GdkFont     *font,
                 GdkGC       *gc,
                 gint         x,
                 gint         y,
                 const gchar *text,
                 gint         text_length)
{
  g_return_if_fail (drawable != NULL);
  g_return_if_fail (font != NULL);
  g_return_if_fail (gc != NULL);
  g_return_if_fail (text != NULL);

  if (GDK_IS_WINDOW(drawable) && GDK_WINDOW_OBJECT(drawable)->destroyed)
    return;

  if (font->type == GDK_FONT_FONT)
  {
    XFontStruct *xfont = (XFontStruct *) GDK_FONT_XFONT(font);

    // gdk does this... we don't need it..
    //    XSetFont(drawable_private->xdisplay, gc_private->xgc, xfont->fid);

    // We clamp the sizes down to 32768 which is the maximum width of
    // a window.  Even if a font was 1 pixel high and started at the
    // left, the maximum size of a draw request could only be 32k.

    if ((xfont->min_byte1 == 0) && (xfont->max_byte1 == 0))
    {
      XDrawString (GDK_WINDOW_XDISPLAY(drawable), GDK_DRAWABLE_XID(drawable),
                   GDK_GC_XGC(gc), x, y, text, MIN(text_length, 32768));
    }
    else
    {
      XDrawString16 (GDK_WINDOW_XDISPLAY(drawable), GDK_DRAWABLE_XID(drawable),
                     GDK_GC_XGC(gc), x, y, (XChar2b *) text, 
                     MIN((text_length / 2), 32768));
    }
  }
  else if (font->type == GDK_FONT_FONTSET)
  {
    XFontSet fontset = (XFontSet) GDK_FONT_XFONT(font);
    XmbDrawString (GDK_WINDOW_XDISPLAY(drawable), GDK_DRAWABLE_XID(drawable),
                   fontset, GDK_GC_XGC(gc), x, y, text, text_length);
  }
  else
    g_error("undefined font type\n");
}
