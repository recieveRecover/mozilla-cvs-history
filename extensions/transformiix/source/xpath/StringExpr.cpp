/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is TransforMiiX XSLT processor.
 * 
 * The Initial Developer of the Original Code is The MITRE Corporation.
 * Portions created by MITRE are Copyright (C) 1999 The MITRE Corporation.
 *
 * Portions created by Keith Visco as a Non MITRE employee,
 * (C) 1999 Keith Visco. All Rights Reserved.
 * 
 * Contributor(s): 
 * Keith Visco, kvisco@ziplink.net
 *   -- original author.
 *    
 * $Id: StringExpr.cpp,v 1.4 2001/07/02 20:11:04 sicking%bigfoot.com Exp $
 */

#include "Expr.h"

/**
 * StringExpr
 * @author <A HREF="mailto:kvisco@ziplink.net">Keith Visco</A>
 * @version $Revision: 1.4 $ $Date: 2001/07/02 20:11:04 $
**/

/**
 * Creates a new StringExpr
**/
StringExpr::StringExpr(const String& value) {
    //-- copy value
    this->value.append(value);
} //-- StringExpr

/**
 * Evaluates this Expr based on the given context node and processor state
 * @param context the context node for evaluation of this Expr
 * @param ps the ContextState containing the stack information needed
 * for evaluation
 * @return the result of the evaluation
**/
ExprResult* StringExpr::evaluate(Node* context, ContextState* cs) {
   return new StringResult(value);
} //-- evaluate

/**
 * Returns the String representation of this Expr.
 * @param dest the String to use when creating the String
 * representation. The String representation will be appended to
 * any data in the destination String, to allow cascading calls to
 * other #toString() methods for Expressions.
 * @return the String representation of this Expr.
**/
void StringExpr::toString(String& str) {
    UNICODE_CHAR ch = '\'';
    if (value.indexOf(ch) != NOT_FOUND)
        ch = '\"';
    str.append(ch);
    str.append(value);
    str.append(ch);
} //-- toString

