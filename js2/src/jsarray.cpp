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

#ifdef _WIN32
 // Turn off warnings about identifiers too long in browser information
#pragma warning(disable: 4786)
#pragma warning(disable: 4711)
#pragma warning(disable: 4710)
#endif

#include <algorithm>

#include "parser.h"
#include "numerics.h"
#include "js2runtime.h"

// this is the IdentifierList passed to the name lookup routines
#define CURRENT_ATTR    (NULL)

#include "jsarray.h"

namespace JavaScript {    
namespace JS2Runtime {

JSValue Array_Constructor(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue thatValue = thisValue;
    if (thatValue.isNull())
        thatValue = Array_Type->newInstance(cx);
    ASSERT(thatValue.isObject());
    JSObject *thisObj = thatValue.object;
    ASSERT(dynamic_cast<JSArrayInstance *>(thisObj));
    JSArrayInstance *arrInst = (JSArrayInstance *)thisObj;
    if (argc > 0) {
        if (argc == 1) {
            arrInst->mLength = (uint32)(argv[0].toNumber(cx).f64);
        }
        else {
            arrInst->mLength = argc;
            for (uint32 i = 0; i < argc; i++) {
                String *id = numberToString(i);
                arrInst->defineVariable(cx, *id, (NamespaceList *)(NULL), Object_Type, argv[i]);
                delete id;
            }
        }
    }
    return thatValue;
}


static JSValue Array_toString(Context *cx, const JSValue& thisValue, JSValue * /*argv*/, uint32 /*argc*/)
{
    ASSERT(thisValue.isObject());
    JSObject *thisObj = thisValue.object;
    ASSERT(dynamic_cast<JSArrayInstance *>(thisObj));
    JSArrayInstance *arrInst = (JSArrayInstance *)thisObj;

    ContextStackReplacement csr(cx);

    if (arrInst->mLength == 0)
        return JSValue(new String(widenCString("")));
    else {
        String *s = new String();
        for (uint32 i = 0; i < arrInst->mLength; i++) {
            String *id = numberToString(i);
            arrInst->getProperty(cx, *id, NULL);
            JSValue result = cx->popValue();
            s->append(*result.toString(cx).string);
            if (i < (arrInst->mLength - 1))
                s->append(widenCString(","));
        }
        return JSValue(s);
    }
    
}

static JSValue Array_toSource(Context *cx, const JSValue& thisValue, JSValue * /*argv*/, uint32 /*argc*/)
{
    ASSERT(thisValue.isObject());
    JSObject *thisObj = thisValue.object;
    ASSERT(dynamic_cast<JSArrayInstance *>(thisObj));
    JSArrayInstance *arrInst = (JSArrayInstance *)thisObj;

    ContextStackReplacement csr(cx);

    if (arrInst->mLength == 0)
        return JSValue(new String(widenCString("[]")));
    else {
        String *s = new String(widenCString("["));
        for (uint32 i = 0; i < arrInst->mLength; i++) {
            String *id = numberToString(i);
            arrInst->getProperty(cx, *id, NULL);
            JSValue result = cx->popValue();
            if (!result.isUndefined())
                s->append(*result.toString(cx).string);
            if (i < (arrInst->mLength - 1))
                s->append(widenCString(", "));
        }
        s->append(widenCString("]"));
        return JSValue(s);
    }
    
}

static JSValue Array_push(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    ASSERT(thisValue.isObject());
    JSObject *thisObj = thisValue.object;
    ASSERT(dynamic_cast<JSArrayInstance *>(thisObj));
    JSArrayInstance *arrInst = (JSArrayInstance *)thisObj;

    for (uint32 i = 0; i < argc; i++) {
        String *id = numberToString(i + arrInst->mLength);
        arrInst->defineVariable(cx, *id, (NamespaceList *)(NULL), Object_Type, argv[i]);
        delete id;
    }
    arrInst->mLength += argc;
    return JSValue((float64)arrInst->mLength);
}
              
static JSValue Array_pop(Context *cx, const JSValue& thisValue, JSValue * /*argv*/, uint32 /*argc*/)
{
    ASSERT(thisValue.isObject());
    JSObject *thisObj = thisValue.object;
    ASSERT(dynamic_cast<JSArrayInstance *>(thisObj));
    JSArrayInstance *arrInst = (JSArrayInstance *)thisObj;

    ContextStackReplacement csr(cx);

    if (arrInst->mLength > 0) {
        String *id = numberToString(arrInst->mLength - 1);
        arrInst->getProperty(cx, *id, NULL);
        JSValue result = cx->popValue();
        arrInst->deleteProperty(*id, NULL);
        --arrInst->mLength;
        delete id;
        return result;
    }
    else
        return kUndefinedValue;
}

JSValue Array_concat(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    JSValue E = thisValue;

    JSArrayInstance *A = (JSArrayInstance *)(Array_Type->newInstance(cx));
    uint32 n = 0;
    uint32 i = 0;

    ContextStackReplacement csr(cx);

    do {
        if (E.getType() != Array_Type) {
            String *id = numberToString(n++);
            A->setProperty(cx, *id, CURRENT_ATTR, E);            
        }
        else {
            ASSERT(E.isObject() && dynamic_cast<JSArrayInstance *>(E.object));
            JSArrayInstance *arrInst = (JSArrayInstance *)(E.object);
            for (uint32 k = 0; k < arrInst->mLength; k++) {
                String *id = numberToString(k);
                arrInst->getProperty(cx, *id, NULL);
                JSValue result = cx->popValue();
                id = numberToString(n++);
                A->setProperty(cx, *id, CURRENT_ATTR, result);            
            }
        }
        E = argv[i++];
    } while (i <= argc);
    
    return JSValue(A);
}

static JSValue Array_join(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    ContextStackReplacement csr(cx);

    ASSERT(thisValue.isObject());
    JSObject *thisObj = thisValue.object;

    thisObj->getProperty(cx, widenCString("length"), CURRENT_ATTR);
    JSValue result = cx->popValue();
    uint32 length = (uint32)(result.toUInt32(cx).f64);    

    const String *separator;
    if (argc == 0)
        separator = new String(',', 1);
    else
        separator = argv[0].toString(cx).string;

    thisObj->getProperty(cx, *numberToString(0), CURRENT_ATTR);
    result = cx->popValue();

    String *S = new String();

    for (uint32 k = 0; k < length; k++) {
        thisObj->getProperty(cx, *numberToString(0), CURRENT_ATTR);
        result = cx->popValue();
        if (!result.isUndefined() && !result.isNull())
            *S += *result.toString(cx).string;

        if (k < (length - 1))
            *S += *separator;
    }
    
    return JSValue(S);
}

static JSValue Array_reverse(Context *cx, const JSValue& thisValue, JSValue * /*argv*/, uint32 /*argc*/)
{
    ContextStackReplacement csr(cx);

    ASSERT(thisValue.isObject());
    JSObject *thisObj = thisValue.object;

    thisObj->getProperty(cx, widenCString("length"), CURRENT_ATTR);
    JSValue result = cx->popValue();
    uint32 length = (uint32)(result.toUInt32(cx).f64);    

    uint32 halfway = length / 2;

    for (uint32 k = 0; k < halfway; k++) {    
        String *id1 = numberToString(k);
        String *id2 = numberToString(length - k - 1);

        PropertyIterator it;
        if (thisObj->hasOwnProperty(*id1, CURRENT_ATTR, Read, &it)) {
            if (thisObj->hasOwnProperty(*id2, CURRENT_ATTR, Read, &it)) {
                thisObj->getProperty(cx, *id1, CURRENT_ATTR);
                thisObj->getProperty(cx, *id2, CURRENT_ATTR);
                thisObj->setProperty(cx, *id1, CURRENT_ATTR, cx->popValue());
                thisObj->setProperty(cx, *id2, CURRENT_ATTR, cx->popValue());
            }
            else {
                thisObj->getProperty(cx, *id1, CURRENT_ATTR);
                thisObj->setProperty(cx, *id2, CURRENT_ATTR, cx->popValue());
                thisObj->deleteProperty(*id1, CURRENT_ATTR);
            }
        }
        else {
            if (thisObj->hasOwnProperty(*id2, CURRENT_ATTR, Read, &it)) {
                thisObj->getProperty(cx, *id2, CURRENT_ATTR);
                thisObj->setProperty(cx, *id1, CURRENT_ATTR, cx->popValue());
                thisObj->deleteProperty(*id2, CURRENT_ATTR);
            }
            else {
                thisObj->deleteProperty(*id1, CURRENT_ATTR);
                thisObj->deleteProperty(*id2, CURRENT_ATTR);
            }
        }
    }

    return thisValue;
}

static JSValue Array_shift(Context *cx, const JSValue& thisValue, JSValue * /*argv*/, uint32 /*argc*/)
{
    ContextStackReplacement csr(cx);

    ASSERT(thisValue.isObject());
    JSObject *thisObj = thisValue.object;

    thisObj->getProperty(cx, widenCString("length"), CURRENT_ATTR);
    JSValue result = cx->popValue();
    uint32 length = (uint32)(result.toUInt32(cx).f64);    

    if (length == 0) {
        thisObj->setProperty(cx, widenCString("length"), CURRENT_ATTR, result);
        return kUndefinedValue;
    }

    thisObj->getProperty(cx, *numberToString(0), CURRENT_ATTR);
    result = cx->popValue();

    for (uint32 k = 1; k < length; k++) {
        String *id1 = numberToString(k);
        String *id2 = numberToString(k - 1);

        PropertyIterator it;
        if (thisObj->hasOwnProperty(*id1, CURRENT_ATTR, Read, &it)) {
            thisObj->getProperty(cx, *id1, CURRENT_ATTR);
            thisObj->setProperty(cx, *id2, CURRENT_ATTR, cx->popValue());
        }
        else
            thisObj->deleteProperty(*id2, CURRENT_ATTR);        
    }

    thisObj->deleteProperty(*numberToString(length - 1), CURRENT_ATTR);
    thisObj->setProperty(cx, widenCString("length"), CURRENT_ATTR, JSValue((float64)(length - 1)) );

    return result;
}

static JSValue Array_slice(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    ContextStackReplacement csr(cx);

    ASSERT(thisValue.isObject());
    JSObject *thisObj = thisValue.object;

    JSArrayInstance *A = (JSArrayInstance *)Array_Type->newInstance(cx);

    thisObj->getProperty(cx, widenCString("length"), CURRENT_ATTR);
    JSValue result = cx->popValue();
    uint32 length = (uint32)(result.toUInt32(cx).f64);    

    uint32 start, end;
    if (argc < 1) 
        start = 0;
    else {
        int32 arg0 = (int32)(argv[0].toInt32(cx).f64);
        if (arg0 < 0) {
            arg0 += length; 
            if (arg0 < 0)
                start = 0;
            else
                start = toUInt32(arg0);
        }
        else {
            if (toUInt32(arg0) >= length)    // cast ok since > 0
                start = length;
            else
                start = toUInt32(arg0);
        }
    }

    if (argc < 2) 
        end = length;
    else {
        int32 arg1 = (int32)(argv[1].toInt32(cx).f64);
        if (arg1 < 0) {
            arg1 += length;
            if (arg1 < 0)
                end = 0;
            else
                end = toUInt32(arg1);
        }
        else {
            if (toUInt32(arg1) >= length)
                end = length;
            else
                end = toUInt32(arg1);
        }
    }
    
    uint32 n = 0;
    while (start < end) {
        String *id1 = numberToString(start);
        PropertyIterator it;
        if (thisObj->hasOwnProperty(*id1, CURRENT_ATTR, Read, &it)) {
            String *id2 = numberToString(n);
            thisObj->getProperty(cx, *id1, CURRENT_ATTR);
            A->setProperty(cx, *id2, CURRENT_ATTR, cx->popValue());
        }
        n++;
        start++;
    }
    A->setProperty(cx, widenCString("length"), CURRENT_ATTR, JSValue((float64)n) );
    return JSValue(A);
}

static JSValue Array_sort(Context * /*cx*/, const JSValue& /*thisValue*/, JSValue * /*argv*/, uint32 /*argc*/)
{
    return kUndefinedValue;
}

static JSValue Array_splice(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    if (argc > 1) {
        uint32 k;
        ContextStackReplacement csr(cx);

        ASSERT(thisValue.isObject());
        JSObject *thisObj = thisValue.object;
        thisObj->getProperty(cx, widenCString("length"), CURRENT_ATTR);
        JSValue result = cx->popValue();
        uint32 length = (uint32)(result.toUInt32(cx).f64);    

        JSArrayInstance *A = (JSArrayInstance *)Array_Type->newInstance(cx);

        int32 arg0 = (int32)(argv[0].toInt32(cx).f64);
        uint32 start;
        if (arg0 < 0) {
            arg0 += length;
            if (arg0 < 0)
                start = 0;
            else
                start = toUInt32(arg0);
        }
        else {
            if (toUInt32(arg0) >= length)
                start = length;
            else
                start = toUInt32(arg0);
        }

        uint32 deleteCount;
        int32 arg1 = (int32)(argv[1].toInt32(cx).f64);
        if (arg1 < 0)
            deleteCount = 0;
        else
            if (toUInt32(arg1) >= (length - start))
                deleteCount = length - start;
            else
                deleteCount = toUInt32(arg1);
        
        for (k = 0; k < deleteCount; k++) {
            String *id1 = numberToString(start + k);
            PropertyIterator it;
            if (thisObj->hasOwnProperty(*id1, CURRENT_ATTR, Read, &it)) {
                String *id2 = numberToString(k);
                thisObj->getProperty(cx, *id1, CURRENT_ATTR);
                A->setProperty(cx, *id2, CURRENT_ATTR, cx->popValue());
            }
        }
        A->setProperty(cx, widenCString("length"), CURRENT_ATTR, JSValue((float64)deleteCount) );

        uint32 newItemCount = argc - 2;
        if (newItemCount < deleteCount) {
            for (k = start; k < (length - deleteCount); k++) {
                String *id1 = numberToString(k + deleteCount);
                String *id2 = numberToString(k + newItemCount);
                PropertyIterator it;
                if (thisObj->hasOwnProperty(*id1, CURRENT_ATTR, Read, &it)) {
                    thisObj->getProperty(cx, *id1, CURRENT_ATTR);
                    thisObj->setProperty(cx, *id2, CURRENT_ATTR, cx->popValue());
                }
                else
                    thisObj->deleteProperty(*id2, CURRENT_ATTR);
            }
            for (k = length; k > (length - deleteCount + newItemCount); k--) {
                String *id1 = numberToString(k - 1);
                thisObj->deleteProperty(*id1, CURRENT_ATTR);
            }
        }
        else {
            if (newItemCount > deleteCount) {
                for (k = length - deleteCount; k > start; k--) {
                    String *id1 = numberToString(k + deleteCount - 1);
                    String *id2 = numberToString(k + newItemCount - 1);
                    PropertyIterator it;
                    if (thisObj->hasOwnProperty(*id1, CURRENT_ATTR, Read, &it)) {
                        thisObj->getProperty(cx, *id1, CURRENT_ATTR);
                        thisObj->setProperty(cx, *id2, CURRENT_ATTR, cx->popValue());
                    }
                    else
                        thisObj->deleteProperty(*id2, CURRENT_ATTR);
                }
            }
        }
        k = start;
        for (uint32 i = 2; i < argc; i++) {
            String *id1 = numberToString(k++);
            thisObj->setProperty(cx, *id1, CURRENT_ATTR, argv[i]);
        }
        thisObj->setProperty(cx, widenCString("length"), CURRENT_ATTR, JSValue((float64)(length - deleteCount + newItemCount)) );

        return JSValue(A);
    }
    return kUndefinedValue;
}

static JSValue Array_unshift(Context *cx, const JSValue& thisValue, JSValue *argv, uint32 argc)
{
    ContextStackReplacement csr(cx);

    ASSERT(thisValue.isObject());
    JSObject *thisObj = thisValue.object;
    thisObj->getProperty(cx, widenCString("length"), CURRENT_ATTR);
    JSValue result = cx->popValue();
    uint32 length = (uint32)(result.toUInt32(cx).f64);
    uint32 k;

    for (k = length; k > 0; k--) {
        String *id1 = numberToString(k - 1);
        String *id2 = numberToString(k + argc - 1);
        PropertyIterator it;
        if (thisObj->hasOwnProperty(*id1, CURRENT_ATTR, Read, &it)) {
            thisObj->getProperty(cx, *id1, CURRENT_ATTR);
            thisObj->setProperty(cx, *id2, CURRENT_ATTR, cx->popValue());
        }
        else
            thisObj->deleteProperty(*id2, CURRENT_ATTR);
    }

    for (k = 0; k < argc; k++) {
        String *id1 = numberToString(k);
        thisObj->setProperty(cx, *id1, CURRENT_ATTR, argv[k]);
    }
    thisObj->setProperty(cx, widenCString("length"), CURRENT_ATTR, JSValue((float64)(length + argc)) );

    return thisValue;
}


Context::PrototypeFunctions *getArrayProtos()
{
    Context::ProtoFunDef arrayProtos[] = 
    {
        { "toString",           String_Type, 0, Array_toString },
        { "toLocaleString",     String_Type, 0, Array_toString },       // XXX 
        { "toSource",           String_Type, 0, Array_toSource },
        { "push",               Number_Type, 1, Array_push },
        { "pop",                Object_Type, 0, Array_pop },
        { "concat",             Array_Type,  1, Array_concat },
        { "join",               String_Type, 1, Array_join },
        { "reverse",            Array_Type,  0, Array_reverse },
        { "shift",              Object_Type, 0, Array_shift },
        { "slice",              Array_Type,  2, Array_slice },
        { "sort",               Array_Type,  1, Array_sort },
        { "splice",             Array_Type,  2, Array_splice },
        { "unshift",            Number_Type, 1, Array_unshift },
        { NULL }
    };
    return new Context::PrototypeFunctions(&arrayProtos[0]);
}

}
}