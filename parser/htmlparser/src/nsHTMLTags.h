/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

/* Do not edit - generated by gentags.pl */
#ifndef nsHTMLTags_h___
#define nsHTMLTags_h___
#include "nshtmlpars.h"
enum nsHTMLTag {
  /* this enum must be first and must be zero */
  eHTMLTag_unknown=0,

  /* begin tag enums */
  eHTMLTag_a=1, eHTMLTag_abbr=2, eHTMLTag_acronym=3, eHTMLTag_address=4, 
  eHTMLTag_applet=5, eHTMLTag_area=6, eHTMLTag_b=7, eHTMLTag_base=8, 
  eHTMLTag_basefont=9, eHTMLTag_bdo=10, eHTMLTag_bgsound=11, eHTMLTag_big=12, 
  eHTMLTag_blink=13, eHTMLTag_blockquote=14, eHTMLTag_body=15, 
  eHTMLTag_br=16, eHTMLTag_button=17, eHTMLTag_caption=18, 
  eHTMLTag_center=19, eHTMLTag_cite=20, eHTMLTag_code=21, eHTMLTag_col=22, 
  eHTMLTag_colgroup=23, eHTMLTag_dd=24, eHTMLTag_del=25, eHTMLTag_dfn=26, 
  eHTMLTag_dir=27, eHTMLTag_div=28, eHTMLTag_dl=29, eHTMLTag_dt=30, 
  eHTMLTag_em=31, eHTMLTag_embed=32, eHTMLTag_endnote=33, 
  eHTMLTag_fieldset=34, eHTMLTag_font=35, eHTMLTag_form=36, 
  eHTMLTag_frame=37, eHTMLTag_frameset=38, eHTMLTag_h1=39, eHTMLTag_h2=40, 
  eHTMLTag_h3=41, eHTMLTag_h4=42, eHTMLTag_h5=43, eHTMLTag_h6=44, 
  eHTMLTag_head=45, eHTMLTag_hr=46, eHTMLTag_html=47, eHTMLTag_i=48, 
  eHTMLTag_iframe=49, eHTMLTag_ilayer=50, eHTMLTag_image=51, eHTMLTag_img=52, 
  eHTMLTag_input=53, eHTMLTag_ins=54, eHTMLTag_isindex=55, eHTMLTag_kbd=56, 
  eHTMLTag_keygen=57, eHTMLTag_label=58, eHTMLTag_layer=59, 
  eHTMLTag_legend=60, eHTMLTag_li=61, eHTMLTag_link=62, eHTMLTag_listing=63, 
  eHTMLTag_map=64, eHTMLTag_menu=65, eHTMLTag_meta=66, eHTMLTag_multicol=67, 
  eHTMLTag_nobr=68, eHTMLTag_noembed=69, eHTMLTag_noframes=70, 
  eHTMLTag_nolayer=71, eHTMLTag_noscript=72, eHTMLTag_object=73, 
  eHTMLTag_ol=74, eHTMLTag_optgroup=75, eHTMLTag_option=76, eHTMLTag_p=77, 
  eHTMLTag_param=78, eHTMLTag_parsererror=79, eHTMLTag_plaintext=80, 
  eHTMLTag_pre=81, eHTMLTag_q=82, eHTMLTag_s=83, eHTMLTag_samp=84, 
  eHTMLTag_script=85, eHTMLTag_select=86, eHTMLTag_server=87, 
  eHTMLTag_small=88, eHTMLTag_sound=89, eHTMLTag_sourcetext=90, 
  eHTMLTag_spacer=91, eHTMLTag_span=92, eHTMLTag_strike=93, 
  eHTMLTag_strong=94, eHTMLTag_style=95, eHTMLTag_sub=96, eHTMLTag_sup=97, 
  eHTMLTag_table=98, eHTMLTag_tbody=99, eHTMLTag_td=100, 
  eHTMLTag_textarea=101, eHTMLTag_tfoot=102, eHTMLTag_th=103, 
  eHTMLTag_thead=104, eHTMLTag_title=105, eHTMLTag_tr=106, eHTMLTag_tt=107, 
  eHTMLTag_u=108, eHTMLTag_ul=109, eHTMLTag_var=110, eHTMLTag_wbr=111, 
  eHTMLTag_xmp=112, 

  /* The remaining enums are not for tags */
  eHTMLTag_text=113, eHTMLTag_whitespace=114, eHTMLTag_newline=115, 
  eHTMLTag_comment=116, eHTMLTag_entity=117, eHTMLTag_userdefined=118, 
  eHTMLTag_secret_h1style=119, eHTMLTag_secret_h2style=120, 
  eHTMLTag_secret_h3style=121, eHTMLTag_secret_h4style=122, 
  eHTMLTag_secret_h5style=123, eHTMLTag_secret_h6style=124
};
#define NS_HTML_TAG_MAX 112

extern NS_HTMLPARS nsHTMLTag NS_TagToEnum(const char* aTag);
extern NS_HTMLPARS const char* NS_EnumToTag(nsHTMLTag aEnum);

#endif /* nsHTMLTags_h___ */
