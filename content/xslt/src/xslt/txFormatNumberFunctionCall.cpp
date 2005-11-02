/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is the TransforMiiX XSLT processor.
 *
 * The Initial Developer of the Original Code is
 * Jonas Sicking.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Jonas Sicking <sicking@bigfoot.com>
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

#include "XSLTFunctions.h"
#include "ProcessorState.h"
#include "primitives.h"
#include "txAtoms.h"
#include "txIXPathContext.h"
#include <math.h>

#ifndef TX_EXE
#include "prdtoa.h"
#else
#include <stdio.h>
#endif

const PRUnichar txFormatNumberFunctionCall::FORMAT_QUOTE = '\'';

/*
 * FormatNumberFunctionCall
 * A representation of the XSLT additional function: format-number()
 */

/*
 * Creates a new format-number function call
 */
txFormatNumberFunctionCall::txFormatNumberFunctionCall(ProcessorState* aPs,
                                                       Node* aQNameResolveNode)
    : mPs(aPs),
      mQNameResolveNode(aQNameResolveNode)
{
}

/*
 * Evaluates this Expr based on the given context node and processor state
 * @param context the context node for evaluation of this Expr
 * @param cs the ContextState containing the stack information needed
 * for evaluation
 * @return the result of the evaluation
 */
ExprResult* txFormatNumberFunctionCall::evaluate(txIEvalContext* aContext)
{
    nsresult rv = NS_OK;
    if (!requireParams(2, 3, aContext))
        return new StringResult();

    // Get number and format
    txListIterator iter(&params);

    double value;
    nsAutoString formatStr;
    txExpandedName formatName;

    value = evaluateToNumber((Expr*)iter.next(), aContext);
    evaluateToString((Expr*)iter.next(), aContext, formatStr);
    if (iter.hasNext()) {
        nsAutoString formatQName;
        evaluateToString((Expr*)iter.next(), aContext, formatQName);
        rv = formatName.init(formatQName, mQNameResolveNode, MB_FALSE);
        if (NS_FAILED(rv))
            formatName.mNamespaceID = kNameSpaceID_Unknown;
    }

    txDecimalFormat* format = mPs->getDecimalFormat(formatName);
    if (!format) {
        nsAutoString err(NS_LITERAL_STRING("unknown decimal format for: "));
        toString(err);
        aContext->receiveError(err, NS_ERROR_XPATH_INVALID_ARG);
        return new StringResult(err);
    }

    // Special cases
    if (Double::isNaN(value))
        return new StringResult(format->mNaN);

    if (value == Double::POSITIVE_INFINITY)
        return new StringResult(format->mInfinity);

    if (value == Double::NEGATIVE_INFINITY) {
        nsAutoString res;
        res.Append(format->mMinusSign);
        res.Append(format->mInfinity);
        return new StringResult(res);
    }
    
    // Value is a normal finite number
    nsAutoString prefix;
    nsAutoString suffix;
    int minIntegerSize=0;
    int minFractionSize=0;
    int maxFractionSize=0;
    int multiplier=1;
    int groupSize=-1;

    PRUint32 pos = 0;
    PRUint32 formatLen = formatStr.Length();
    MBool inQuote;

    // Get right subexpression
    inQuote = MB_FALSE;
    if (Double::isNeg(value)) {
        while (pos < formatLen &&
               (inQuote ||
                formatStr.CharAt(pos) != format->mPatternSeparator)) {
            if (formatStr.CharAt(pos) == FORMAT_QUOTE)
                inQuote = !inQuote;
            pos++;
        }

        if (pos == formatLen) {
            pos = 0;
            prefix.Append(format->mMinusSign);
        }
        else
            pos++;
    }

    // Parse the format string
    FormatParseState pState = Prefix;
    inQuote = MB_FALSE;

    PRUnichar c = 0;
    while (pos < formatLen && pState != Finished) {
        c=formatStr.CharAt(pos++);

        switch (pState) {

        case Prefix:
        case Suffix:
            if (!inQuote) {
                if (c == format->mPercent) {
                    if (multiplier == 1)
                        multiplier = 100;
                    else {
                        nsAutoString err(INVALID_PARAM_VALUE);
                        toString(err);
                        aContext->receiveError(err,
                                               NS_ERROR_XPATH_EVAL_FAILED);
                        return new StringResult(err);
                    }
                }
                else if (c == format->mPerMille) {
                    if (multiplier == 1)
                        multiplier = 1000;
                    else {
                        nsAutoString err(INVALID_PARAM_VALUE);
                        toString(err);
                        aContext->receiveError(err,
                                               NS_ERROR_XPATH_EVAL_FAILED);
                        return new StringResult(err);
                    }
                }
                else if (c == format->mDecimalSeparator ||
                         c == format->mGroupingSeparator ||
                         c == format->mZeroDigit ||
                         c == format->mDigit ||
                         c == format->mPatternSeparator) {
                    pState = pState == Prefix ? IntDigit : Finished;
                    pos--;
                    break;
                }
            }

            if (c == FORMAT_QUOTE)
                inQuote = !inQuote;
            else if (pState == Prefix)
                prefix.Append(c);
            else
                suffix.Append(c);
            break;

        case IntDigit:
            if (c == format->mGroupingSeparator)
                groupSize=0;
            else if (c == format->mDigit) {
                if (groupSize >= 0)
                    groupSize++;
            }
            else {
                pState = IntZero;
                pos--;
            }
            break;

        case IntZero:
            if (c == format->mGroupingSeparator)
                groupSize = 0;
            else if (c == format->mZeroDigit) {
                if (groupSize >= 0)
                    groupSize++;
                minIntegerSize++;
            }
            else if (c == format->mDecimalSeparator) {
                pState = FracZero;
            }
            else {
                pState = Suffix;
                pos--;
            }
            break;

        case FracZero:
            if (c == format->mZeroDigit) {
                maxFractionSize++;
                minFractionSize++;
            }
            else {
                pState = FracDigit;
                pos--;
            }
            break;

        case FracDigit:
            if (c == format->mDigit)
                maxFractionSize++;
            else {
                pState = Suffix;
                pos--;
            }
            break;

        case Finished:
            break;
        }
    }

    // Did we manage to parse the entire formatstring and was it valid
    if ((c != format->mPatternSeparator && pos < formatLen) ||
        inQuote ||
        groupSize == 0) {
        nsAutoString err(INVALID_PARAM_VALUE);
        toString(err);
        aContext->receiveError(err,
                               NS_ERROR_XPATH_EVAL_FAILED);
        return new StringResult(err);
    }


    /*
     * FINALLY we're done with the parsing
     * now build the result string
     */

    value = fabs(value) * multiplier;

    // Prefix
    nsAutoString res(prefix);

#ifdef TX_EXE

    int bufsize;
    if (value > 1)
        bufsize = (int)log10(value) + 1;
    else
        bufsize = 1;

    if (bufsize < minIntegerSize)
        bufsize = minIntegerSize;

    bufsize += maxFractionSize + 3; // decimal separator + ending null +
                                    // rounding safety

    char* buf = new char[bufsize];
    if (!buf) {
        //XXX ErrorReport: out of memory
        return new StringResult;
    }

    int totalSize = minIntegerSize + maxFractionSize;
    if (maxFractionSize > 0)
        totalSize++; //to account for decimal point

    char formatBuf[30];
    sprintf(formatBuf, "%%0%d.%df", totalSize, maxFractionSize);

    sprintf(buf, formatBuf, value);
    
    // Find decimalseparator for grouping
    int intDigits;
    for (intDigits = 0; buf[intDigits] && buf[intDigits] != '.'; intDigits++);
    
    if (groupSize < 0)
        groupSize = intDigits * 2; //to simplify grouping

    // Integer digits
    int i;
    for (i = 0; i < intDigits; ++i) {
        if ((intDigits-i)%groupSize == 0 && i != 0)
            res.Append(format->mGroupingSeparator);

        res.Append((PRUnichar)(buf[i] - '0' + format->mZeroDigit));
    }

    // Fractions
    MBool printDeci = MB_TRUE;
    int extraZeros = 0;

    if (buf[i])
        i++;  // skip decimal separator
    
    for (; buf[i]; i++) {
        if (i-intDigits-1 >= minFractionSize && buf[i] == '0') {
            extraZeros++;
        }
        else {
            if (printDeci) {
                res.Append(format->mDecimalSeparator);
                printDeci = MB_FALSE;
            }
            while (extraZeros) {
                res.Append(format->mZeroDigit);
                extraZeros--;
            }

            res.Append((PRUnichar)(buf[i] - '0' + format->mZeroDigit));
        }
    }

    if (!intDigits && printDeci) {
        // If we havn't added any characters we add a '0'
        // This can only happen for formats like '##.##'
        res.Append(format->mZeroDigit);
    }
    
    delete [] buf;


#else // TX_EXE

    int bufsize;
    if (value > 1)
        bufsize = (int)log10(value) + 30;
    else
        bufsize = 1 + 30;

    char* buf = new char[bufsize];
    if (!buf) {
        //XXX ErrorReport: out of memory
        return new StringResult;
    }

    PRIntn bufIntDigits, sign;
    char* endp;
    PR_dtoa(value, 0, 0, &bufIntDigits, &sign, &endp, buf, bufsize-1);

    int buflen = endp - buf;
    int intDigits;
    intDigits = bufIntDigits > minIntegerSize ? bufIntDigits : minIntegerSize;

    if (groupSize < 0)
        groupSize = intDigits + 10; //to simplify grouping

    // XXX We shouldn't use SetLength.
    res.SetLength(res.Length() +
                  intDigits +               // integer digits
                  1 +                       // decimal separator
                  maxFractionSize +         // fractions
                  (intDigits-1)/groupSize); // group separators

    PRInt32 i = bufIntDigits + maxFractionSize - 1;
    MBool carry = (i+1 < buflen) && (buf[i+1] >= '5');
    MBool hasFraction = MB_FALSE;

    PRUint32 resPos = res.Length()-1;

    // Fractions
    for (; i >= bufIntDigits; --i) {
        int digit;
        if (i >= buflen || i < 0) {
            digit = 0;
        }
        else {
            digit = buf[i] - '0';
        }
        
        if (carry) {
            digit = (digit + 1) % 10;
            carry = digit == 0;
        }

        if (hasFraction || digit != 0 || i < bufIntDigits+minFractionSize) {
            hasFraction = MB_TRUE;
            res.SetCharAt((PRUnichar)(digit + format->mZeroDigit),
                          resPos--);
        }
        else {
            res.Truncate(resPos--);
        }
    }

    // Decimal separator
    if (hasFraction) {
        res.SetCharAt(format->mDecimalSeparator, resPos--);
    }
    else {
        res.Truncate(resPos--);
    }

    // Integer digits
    for (i = 0; i < intDigits; ++i) {
        int digit;
        if (bufIntDigits-i-1 >= buflen || bufIntDigits-i-1 < 0) {
            digit = 0;
        }
        else {
            digit = buf[bufIntDigits-i-1] - '0';
        }
        
        if (carry) {
            digit = (digit + 1) % 10;
            carry = digit == 0;
        }

        if (i != 0 && i%groupSize == 0) {
            res.SetCharAt(format->mGroupingSeparator, resPos--);
        }

        res.SetCharAt((PRUnichar)(digit + format->mZeroDigit), resPos--);
    }

    if (carry) {
        if (i%groupSize == 0) {
            res.Insert(format->mGroupingSeparator, resPos + 1);
        }
        res.Insert((PRUnichar)(1 + format->mZeroDigit), resPos + 1);
    }
    
    if (!hasFraction && !intDigits && !carry) {
        // If we havn't added any characters we add a '0'
        // This can only happen for formats like '##.##'
        res.Append(format->mZeroDigit);
    }

    delete [] buf;

#endif // TX_EXE

    // Build suffix
    res.Append(suffix);

    return new StringResult(res);
} //-- evaluate

nsresult txFormatNumberFunctionCall::getNameAtom(txAtom** aAtom)
{
    *aAtom = txXSLTAtoms::formatNumber;
    TX_ADDREF_ATOM(*aAtom);
    return NS_OK;
}

/*
 * txDecimalFormat
 * A representation of the XSLT element <xsl:decimal-format>
 */

txDecimalFormat::txDecimalFormat() : mInfinity(NS_LITERAL_STRING("Infinity")),
                                     mNaN(NS_LITERAL_STRING("NaN"))
{
    mDecimalSeparator = '.';
    mGroupingSeparator = ',';
    mMinusSign = '-';
    mPercent = '%';
    mPerMille = 0x2030;
    mZeroDigit = '0';
    mDigit = '#';
    mPatternSeparator = ';';
}

MBool txDecimalFormat::isEqual(txDecimalFormat* other)
{
    return mDecimalSeparator == other->mDecimalSeparator &&
           mGroupingSeparator == other->mGroupingSeparator &&
           mInfinity.Equals(other->mInfinity) &&
           mMinusSign == other->mMinusSign &&
           mNaN.Equals(other->mNaN) &&
           mPercent == other->mPercent &&
           mPerMille == other->mPerMille &&
           mZeroDigit == other->mZeroDigit &&
           mDigit == other->mDigit &&
           mPatternSeparator  == other->mPatternSeparator;
}
