/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is the JavaScript 2 Prototype.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */


#define JS2_BIT(n)       ((uint32)1 << (n))
#define JS2_BITMASK(n)   (JS2_BIT(n) - 1)
/*
 * Type tags stored in the low bits of a js2val.
 */
#define JS2VAL_OBJECT            0x0     /* untagged reference to object */
#define JS2VAL_INT               0x1     /* tagged 31-bit integer value */
#define JS2VAL_DOUBLE            0x2     /* tagged reference to double */
#define JS2VAL_STRING            0x4     /* tagged reference to string */
#define JS2VAL_BOOLEAN           0x6     /* tagged boolean value */

/* Type tag bitfield length and derived macros. */
#define JS2VAL_TAGBITS           3
#define JS2VAL_TAGMASK           JS2_BITMASK(JS2VAL_TAGBITS)
#define JS2VAL_TAG(v)            ((v) & JS2VAL_TAGMASK)
#define JS2VAL_SETTAG(v,t)       ((v) | (t))
#define JS2VAL_CLRTAG(v)         ((v) & ~(js2val)JS2VAL_TAGMASK)
#define JS2VAL_ALIGN             JS2_BIT(JS2VAL_TAGBITS)

#define JS2VAL_INT_POW2(n)       ((js2val)1 << (n))
#define JS2VAL_INT_MAX           (JS2VAL_INT_POW2(30) - 1)
#define INT_TO_JS2VAL(i)         (((js2val)(i) << 1) | JS2VAL_INT)
#define JS2VAL_TO_INT(v)         ((int32)(v) >> 1)
#define INT_FITS_IN_JS2VAL(i)    ((uint32)((i)+JS2VAL_INT_MAX) <= 2*JS2VAL_INT_MAX)

#define JS2VAL_VOID              INT_TO_JS2VAL(0 - JS2VAL_INT_POW2(30))
#define JS2VAL_NULL              OBJECT_TO_JS2VAL(0)
#define JS2VAL_FALSE             BOOLEAN_TO_JS2VAL(false)
#define JS2VAL_TRUE              BOOLEAN_TO_JS2VAL(true)

/* Predicates for type testing. */
#define JS2VAL_IS_OBJECT(v)      (JS2VAL_TAG(v) == JS2VAL_OBJECT)
#define JS2VAL_IS_NUMBER(v)      (JS2VAL_IS_INT(v) || JS2VAL_IS_DOUBLE(v))
#define JS2VAL_IS_INT(v)         (((v) & JS2VAL_INT) && (v) != JS2VAL_VOID)
#define JS2VAL_IS_DOUBLE(v)      (JS2VAL_TAG(v) == JS2VAL_DOUBLE)
#define JS2VAL_IS_STRING(v)      (JS2VAL_TAG(v) == JS2VAL_STRING)
#define JS2VAL_IS_BOOLEAN(v)     (JS2VAL_TAG(v) == JS2VAL_BOOLEAN)
#define JS2VAL_IS_NULL(v)        ((v) == JS2VAL_NULL)
#define JS2VAL_IS_VOID(v)        ((v) == JS2VAL_VOID)
#define JS2VAL_IS_PRIMITIVE(v)   (!JS2VAL_IS_OBJECT(v) || JS2VAL_IS_NULL(v))

/* Objects, strings, and doubles are GC'ed. */
#define JS2VAL_IS_GCTHING(v)     (!((v) & JS2VAL_INT) && !JS2VAL_IS_BOOLEAN(v))
#define JS2VAL_TO_GCTHING(v)     ((void *)JS2VAL_CLRTAG(v))
#define JS2VAL_TO_OBJECT(v)      ((JS2Object *)JS2VAL_TO_GCTHING(v))
#define JS2VAL_TO_DOUBLE(v)      ((float64 *)JS2VAL_TO_GCTHING(v))
#define JS2VAL_TO_STRING(v)      ((String *)JS2VAL_TO_GCTHING(v))
#define OBJECT_TO_JS2VAL(obj)    ((js2val)(obj))
#define DOUBLE_TO_JS2VAL(dp)     JS2VAL_SETTAG((js2val)(dp), JS2VAL_DOUBLE)
#define STRING_TO_JS2VAL(str)    JS2VAL_SETTAG((js2val)(str), JS2VAL_STRING)

/* Convert between boolean and js2val. */
#define JS2VAL_TO_BOOLEAN(v)     (((v) >> JS2VAL_TAGBITS) != 0)
#define BOOLEAN_TO_JS2VAL(b)     JS2VAL_SETTAG((js2val)(b) << JS2VAL_TAGBITS,     \
                                             JS2VAL_BOOLEAN)

#ifdef IS_LITTLE_ENDIAN
#define JSDOUBLE_HI32(x)        (((uint32 *)&(x))[1])
#define JSDOUBLE_LO32(x)        (((uint32 *)&(x))[0])
#else
#define JSDOUBLE_HI32(x)        (((uint32 *)&(x))[0])
#define JSDOUBLE_LO32(x)        (((uint32 *)&(x))[1])
#endif

#define JSDOUBLE_HI32_SIGNBIT   0x80000000
#define JSDOUBLE_HI32_EXPMASK   0x7ff00000
#define JSDOUBLE_HI32_MANTMASK  0x000fffff

#define JSDOUBLE_IS_NaN(x)                                                    \
    ((JSDOUBLE_HI32(x) & JSDOUBLE_HI32_EXPMASK) == JSDOUBLE_HI32_EXPMASK &&   \
     (JSDOUBLE_LO32(x) || (JSDOUBLE_HI32(x) & JSDOUBLE_HI32_MANTMASK)))

#define JSDOUBLE_IS_INFINITE(x)                                               \
    ((JSDOUBLE_HI32(x) & ~JSDOUBLE_HI32_SIGNBIT) == JSDOUBLE_HI32_EXPMASK &&   \
     !JSDOUBLE_LO32(x))

#define JSDOUBLE_IS_FINITE(x)                                                 \
    ((JSDOUBLE_HI32(x) & JSDOUBLE_HI32_EXPMASK) != JSDOUBLE_HI32_EXPMASK)

#define JSDOUBLE_IS_POSZERO(d)  (JSDOUBLE_HI32(d) == 0 && JSDOUBLE_LO32(d) == 0)

#define JSDOUBLE_IS_NEGZERO(d)  (JSDOUBLE_HI32(d) == JSDOUBLE_HI32_SIGNBIT && \
                                 JSDOUBLE_LO32(d) == 0)



/*
 * JSDOUBLE_IS_INT first checks that d is neither NaN nor infinite, to avoid
 * raising SIGFPE on platforms such as Alpha Linux, then (only if the cast is
 * safe) leaves i as (jsint)d.  This also avoid anomalous NaN floating point
 * comparisons under MSVC.
 */
#define JSDOUBLE_IS_INT(d, i) (JSDOUBLE_IS_FINITE(d)                          \
                               && !JSDOUBLE_IS_NEGZERO(d)                     \
			       && ((d) == (i = (int32)(d))))


#define JS2VAL_IS_UNDEFINED(v)  JS2VAL_IS_VOID(v)
#define JS2VAL_UNDEFINED JS2VAL_VOID

typedef uint32 js2val;