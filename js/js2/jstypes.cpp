/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
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

#include "jstypes.h"
#include "numerics.h"
#include "icodegenerator.h"

namespace JavaScript {
namespace JSTypes {

//    using JavaScript::StringAtom;

// the canonical undefined value, etc.
const JSValue kUndefinedValue;
const JSValue kNaN = JSValue(nan);
const JSValue kTrue = JSValue(true);
const JSValue kFalse = JSValue(false);

const JSType Any_Type = JSType(NULL);
const JSType Integer_Type = JSType(&Any_Type);
const JSType Number_Type = JSType(&Integer_Type);
const JSType Character_Type = JSType(&Any_Type);
const JSType String_Type = JSType(&Character_Type);
const JSType Function_Type = JSType(&Any_Type);
const JSType Array_Type = JSType(&Any_Type);
const JSType Type_Type = JSType(&Any_Type);
const JSType Boolean_Type = JSType(&Any_Type);
const JSType Null_Type = JSType(&Any_Type);
const JSType Void_Type = JSType(&Any_Type);
const JSType None_Type = JSType(&Any_Type);


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

#define JSDOUBLE_IS_NEGZERO(d)  (JSDOUBLE_HI32(d) == JSDOUBLE_HI32_SIGNBIT && \
				 JSDOUBLE_LO32(d) == 0)


const JSType *JSValue::getType() const
{
    switch (tag) {
    case JSValue::i32_tag:
        return &Integer_Type;
    case JSValue::u32_tag:
        return &Integer_Type;
    case JSValue::f64_tag:
        return &Number_Type;
    case JSValue::object_tag:
        return &Any_Type;        // XXX get type from Object
    case JSValue::array_tag:
        return &Array_Type;
    case JSValue::function_tag:
        return &Function_Type;
    case JSValue::string_tag:
        return &String_Type;
    case JSValue::boolean_tag:
        return &Boolean_Type;
    case JSValue::undefined_tag:
        return &Void_Type;
    case JSValue::type_tag:
        return &Type_Type;
    default:
        NOT_REACHED("Bad tag");
        return &None_Type;
    }
}

bool JSValue::isNaN() const
{
    ASSERT(isNumber());
    switch (tag) {
    case i32_tag:
    case u32_tag:
        return false;
    case f64_tag:
        return JSDOUBLE_IS_NaN(f64);
    default:
        NOT_REACHED("Broken compiler?");
        return true;
    }
}
              
int JSValue::operator==(const JSValue& value) const
{
    if (this->tag == value.tag) {
        #define CASE(T) case T##_tag: return (this->T == value.T)
        switch (tag) {
        CASE(i8); CASE(u8);
        CASE(i16); CASE(u16);
        CASE(i32); CASE(u32); CASE(f32);
        CASE(i64); CASE(u64); CASE(f64);
        CASE(object); CASE(array); CASE(function); CASE(string);
        CASE(boolean);
        #undef CASE
        // question:  are all undefined values equal to one another?
        case undefined_tag: return 1;
        }
    }
    return 0;
}

Formatter& operator<<(Formatter& f, const JSObject& obj)
{
    obj.printProperties(f);
    return f;
}

void JSObject::printProperties(Formatter& f) const
{
    for (JSProperties::const_iterator i = mProperties.begin(); i != mProperties.end(); i++) {
        f << (*i).first << " : " << (*i).second;
        f << "\n";
    }
}

Formatter& operator<<(Formatter& f, const JSValue& value)
{
    switch (value.tag) {
    case JSValue::i32_tag:
        f << float64(value.i32);
        break;
    case JSValue::u32_tag:
        f << float64(value.u32);
        break;
    case JSValue::f64_tag:
        f << value.f64;
        break;
    case JSValue::object_tag:
        {
            printFormat(f, "Object @ 0x%08X\n", value.object);
            f << *value.object;
        }
        break;
    case JSValue::array_tag:
        printFormat(f, "Array @ 0x%08X", value.object);
        break;
    case JSValue::function_tag:
        printFormat(f, "Function @ 0x%08X", value.object);
        break;
    case JSValue::string_tag:
        f << "\"" << *value.string << "\"";
        break;
    case JSValue::boolean_tag:
        f << ((value.boolean) ? "true" : "false");
        break;
    case JSValue::undefined_tag:
        f << "undefined";
        break;
    default:
        NOT_REACHED("Bad tag");
    }
    return f;
}

JSValue JSValue::toPrimitive(ECMA_type /*hint*/) const
{
    JSObject *obj;
    switch (tag) {
    case i32_tag:
    case u32_tag:
    case f64_tag:
    case string_tag:
    case boolean_tag:
    case undefined_tag:
        return *this;

    case object_tag:
        obj = object;
        break;
    case array_tag:
        obj = array;
        break;
    case function_tag:
        obj = function;
        break;

    default:
        NOT_REACHED("Bad tag");
        return kUndefinedValue;
    }

    const JSValue &toString = obj->getProperty(widenCString("toString"));
    if (toString.isObject()) {
        if (toString.isFunction()) {
        }
        else    // right? The spec doesn't say
            throw new JSException("Runtime error from toPrimitive");    // XXX
    }

    const JSValue &valueOf = obj->getProperty(widenCString("valueOf"));
    if (!valueOf.isObject())
        throw new JSException("Runtime error from toPrimitive");    // XXX
    
    return kUndefinedValue;
    
}


JSValue JSValue::valueToString(const JSValue& value) // can assume value is not a string
{
    const char* chrp;
    char buf[dtosStandardBufferSize];
    switch (value.tag) {
    case i32_tag:
        chrp = doubleToStr(buf, dtosStandardBufferSize, value.i32, dtosStandard, 0);
        break;
    case u32_tag:
        chrp = doubleToStr(buf, dtosStandardBufferSize, value.u32, dtosStandard, 0);
        break;
    case f64_tag:
        chrp = doubleToStr(buf, dtosStandardBufferSize, value.f64, dtosStandard, 0);
        break;
    case object_tag:
        chrp = "object";
        break;
    case array_tag:
        chrp = "array";
        break;
    case function_tag:
        chrp = "function";
        break;
    case string_tag:
        return value;
    case boolean_tag:
        chrp = (value.boolean) ? "true" : "false";
        break;
    case undefined_tag:
        chrp = "undefined";
        break;
    default:
        NOT_REACHED("Bad tag");
    }
    return JSValue(new JSString(chrp));
}

JSValue JSValue::valueToNumber(const JSValue& value) // can assume value is not a number
{
    switch (value.tag) {
    case i32_tag:
        return JSValue((float64)value.i32);
    case u32_tag:
        return JSValue((float64)value.u32);
    case f64_tag:
        return value;
    case string_tag: 
        {
            JSString* str = value.string;
            const char16 *numEnd;
	        double d = stringToDouble(str->begin(), str->end(), numEnd);
            return JSValue(d);
        }
    case object_tag:
    case array_tag:
    case function_tag:
        // XXX more needed :
        // toNumber(toPrimitive(hint Number))
        return kUndefinedValue;
    case boolean_tag:
        return JSValue((value.boolean) ? 1.0 : 0.0);
    case undefined_tag:
        return kNaN;
    default:
        NOT_REACHED("Bad tag");
        return kUndefinedValue;
    }
}

JSValue JSValue::valueToBoolean(const JSValue& value)
{
    switch (value.tag) {
    case i32_tag:
        return JSValue(value.i32 != 0);
    case u32_tag:
        return JSValue(value.u32 != 0);
    case f64_tag:
        return JSValue(!(value.f64 == 0.0) || JSDOUBLE_IS_NaN(value.f64));
    case string_tag: 
        return JSValue(value.string->length() != 0);
    case boolean_tag:
        return value;
    case object_tag:
    case array_tag:
    case function_tag:
        return kTrue;
    case undefined_tag:
        return kFalse;
    default:
        NOT_REACHED("Bad tag");
        return kUndefinedValue;
    }
}


static const double two32 = 4294967296.0;
static const double two31 = 2147483648.0;

JSValue JSValue::valueToInt32(const JSValue& value)
{
    double d;
    switch (value.tag) {
    case i32_tag:
        return value;
    case u32_tag:
        d = value.u32;
        break;
    case f64_tag:
        d = value.f64;
        break;
    case string_tag: 
        {
            JSString* str = value.string;
            const char16 *numEnd;
	        d = stringToDouble(str->begin(), str->end(), numEnd);
        }
        break;
    case boolean_tag:
        return JSValue((int32)((value.boolean) ? 1 : 0));
    case object_tag:
    case array_tag:
    case undefined_tag:
        // toNumber(toPrimitive(hint Number))
        return kUndefinedValue;
    default:
        NOT_REACHED("Bad tag");
        return kUndefinedValue;
    }
    if ((d == 0.0) || !JSDOUBLE_IS_FINITE(d) )
        return JSValue((int32)0);
    d = fmod(d, two32);
    d = (d >= 0) ? d : d + two32;
    if (d >= two31)
	    return JSValue((int32)(d - two32));
    else
	    return JSValue((int32)d);
    
}

JSValue JSValue::valueToUInt32(const JSValue& value)
{
    double d;
    switch (value.tag) {
    case i32_tag:
        return JSValue((uint32)value.i32);
    case u32_tag:
        return value;
    case f64_tag:
        d = value.f64;
        break;
    case string_tag: 
        {
            JSString* str = value.string;
            const char16 *numEnd;
	        d = stringToDouble(str->begin(), str->end(), numEnd);
        }
        break;
    case boolean_tag:
        return JSValue((uint32)((value.boolean) ? 1 : 0));
    case object_tag:
    case array_tag:
    case undefined_tag:
        // toNumber(toPrimitive(hint Number))
        return kUndefinedValue;
    default:
        NOT_REACHED("Bad tag");
        return kUndefinedValue;
    }
    if ((d == 0.0) || !JSDOUBLE_IS_FINITE(d))
        return JSValue((uint32)0);
    bool neg = (d < 0);
    d = floor(neg ? -d : d);
    d = neg ? -d : d;
    d = fmod(d, two32);
    d = (d >= 0) ? d : d + two32;
    return JSValue((uint32)d);
}

JSFunction::~JSFunction()
{
    delete mICode;
}

JSString::JSString(const String& str)
{
    size_t n = str.size();
    resize(n);
    traits_type::copy(begin(), str.begin(), n);
}

JSString::JSString(const String* str)
{
    size_t n = str->size();
    resize(n);
    traits_type::copy(begin(), str->begin(), n);
}

JSString::JSString(const char* str)
{
    size_t n = ::strlen(str);
    resize(n);
    std::transform(str, str + n, begin(), JavaScript::widen);
}

// # of sub-type relationship between this type and the other type 
// (== MAX_INT if other is not a base type)

int32 JSType::distance(const JSType *other) const
{
    if (other == this) 
        return 0; 
    if (baseType == NULL)
        return NoRelation;
    int32 baseDistance = baseType->distance(other);
    if (baseDistance != NoRelation)
        ++baseDistance;
    return baseDistance;
}


} /* namespace JSTypes */    
} /* namespace JavaScript */
