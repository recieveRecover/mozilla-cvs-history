/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#include "jri.h"

#ifndef _netscape_plugin_composer_Composer_H_
#define _netscape_plugin_composer_Composer_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct java_lang_Class;
struct netscape_plugin_composer_Composer;

/*******************************************************************************
 * Class netscape/plugin/composer/Composer
 ******************************************************************************/
typedef struct netscape_plugin_composer_Composer netscape_plugin_composer_Composer;
struct netscape_plugin_composer_Composer {
  void *ptr;
};


/*******************************************************************************
 * Public Methods
 ******************************************************************************/

JRI_PUBLIC_API(struct netscape_plugin_composer_Composer*)
netscape_plugin_composer_Composer_new(JRIEnv* env, struct java_lang_Class* clazz, jint a, jint b, jint c);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* Class netscape/plugin/composer/Composer */
