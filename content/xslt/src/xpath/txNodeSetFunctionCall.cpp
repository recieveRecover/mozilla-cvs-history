/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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
 * Marina Mechtcheriakova, mmarina@mindspring.com
 *   -- changed some behavoir to be more compliant with spec
 * 
 */

/*
 * NodeSetFunctionCall
 * A representation of the XPath NodeSet funtions
 */

#include "FunctionLib.h"
#include "NodeSet.h"
#include "Tokenizer.h"
#include "txAtoms.h"
#include "txIXPathContext.h"
#include "XMLDOMUtils.h"

/*
 * Creates a NodeSetFunctionCall of the given type
 */
NodeSetFunctionCall::NodeSetFunctionCall(NodeSetFunctions aType)
    : mType(aType)
{
}

/*
 * Evaluates this Expr based on the given context node and processor state
 * @param context the context node for evaluation of this Expr
 * @param ps the ContextState containing the stack information needed
 * for evaluation
 * @return the result of the evaluation
 */
ExprResult* NodeSetFunctionCall::evaluate(txIEvalContext* aContext) {
    txListIterator iter(&params);
    switch (mType) {
        case COUNT:
        {
            if (!requireParams(1, 1, aContext))
                return new StringResult(NS_LITERAL_STRING("error"));

            NodeSet* nodes;
            nodes = evaluateToNodeSet((Expr*)iter.next(), aContext);
            if (!nodes)
                return new StringResult(NS_LITERAL_STRING("error"));

            double count = nodes->size();
            delete nodes;
            return new NumberResult(count);
        }
        case ID:
        {
            if (!requireParams(1, 1, aContext))
                return new StringResult(NS_LITERAL_STRING("error"));

            ExprResult* exprResult;
            exprResult = ((Expr*)iter.next())->evaluate(aContext);
            if (!exprResult)
                return new StringResult(NS_LITERAL_STRING("error"));

            NodeSet* resultSet = new NodeSet();
            if (!resultSet) {
                // XXX ErrorReport: out of memory
                return 0;
            }

            Document* contextDoc = 0;
            Node* contextNode = aContext->getContextNode();
            if (contextNode->getNodeType() == Node::DOCUMENT_NODE)
                contextDoc = (Document*)contextNode;
            else
                contextDoc = contextNode->getOwnerDocument();
            
            if (exprResult->getResultType() == ExprResult::NODESET) {
                NodeSet* nodes = (NodeSet*)exprResult;
                int i;
                for (i = 0; i < nodes->size(); i++) {
                    String idList, id;
                    XMLDOMUtils::getNodeValue(nodes->get(i), idList);
                    txTokenizer tokenizer(idList);
                    while (tokenizer.hasMoreTokens()) {
                        tokenizer.nextToken(id);
                        Node* idNode = contextDoc->getElementById(id);
                        if (idNode)
                            resultSet->add(idNode);
                    }
                }
            }
            else {
                String idList, id;
                exprResult->stringValue(idList);
                txTokenizer tokenizer(idList);
                while (tokenizer.hasMoreTokens()) {
                    tokenizer.nextToken(id);
                    Node* idNode = contextDoc->getElementById(id);
                    if (idNode)
                        resultSet->add(idNode);
                }
            }
            delete exprResult;

            return resultSet;
        }
        case LAST:
        {
            if (!requireParams(0, 0, aContext))
                return new StringResult(NS_LITERAL_STRING("error"));

            return new NumberResult(aContext->size());
        }
        case LOCAL_NAME:
        case NAME:
        case NAMESPACE_URI:
        {
            if (!requireParams(0, 1, aContext))
                return new StringResult(NS_LITERAL_STRING("error"));

            Node* node = 0;
            // Check for optional arg
            if (iter.hasNext()) {
                NodeSet* nodes;
                nodes = evaluateToNodeSet((Expr*)iter.next(), aContext);
                if (!nodes)
                    return new StringResult(NS_LITERAL_STRING("error"));

                if (nodes->isEmpty()) {
                    delete nodes;
                    return new StringResult();
                }
                node = nodes->get(0);
                delete nodes;
            }
            else {
                node = aContext->getContextNode();
            }

            switch (mType) {
                case LOCAL_NAME:
                {
                    String localName;
                    txAtom* localNameAtom;
                    node->getLocalName(&localNameAtom);
                    if (localNameAtom) {
                        // Node has a localName
                        TX_GET_ATOM_STRING(localNameAtom, localName);
                        TX_RELEASE_ATOM(localNameAtom);
                    }
                    
                    return new StringResult(localName);
                }
                case NAMESPACE_URI:
                {
                    return new StringResult(node->getNamespaceURI());
                }
                case NAME:
                {
                    switch (node->getNodeType()) {
                        case Node::ATTRIBUTE_NODE:
                        case Node::ELEMENT_NODE:
                        case Node::PROCESSING_INSTRUCTION_NODE:
                        // XXX Namespace: namespaces have a name
                            return new StringResult(node->getNodeName());
                        default:
                            break;
                    }
                    return new StringResult();
                }
                default:
                {
                    break;
                }
            }
        }
        case POSITION:
        {
            if (!requireParams(0, 0, aContext))
                return new StringResult(NS_LITERAL_STRING("error"));

            return new NumberResult(aContext->position());
        }
    }

    String err(NS_LITERAL_STRING("Internal error"));
    aContext->receiveError(err, NS_ERROR_UNEXPECTED);
    return new StringResult(NS_LITERAL_STRING("error"));
}

nsresult NodeSetFunctionCall::getNameAtom(txAtom** aAtom)
{
    switch (mType) {
        case COUNT:
        {
            *aAtom = txXPathAtoms::count;
            break;
        }
        case ID:
        {
            *aAtom = txXPathAtoms::id;
            break;
        }
        case LAST:
        {
            *aAtom = txXPathAtoms::last;
            break;
        }
        case LOCAL_NAME:
        {
            *aAtom = txXPathAtoms::localName;
            break;
        }
        case NAME:
        {
            *aAtom = txXPathAtoms::name;
            break;
        }
        case NAMESPACE_URI:
        {
            *aAtom = txXPathAtoms::namespaceUri;
            break;
        }
        case POSITION:
        {
            *aAtom = txXPathAtoms::position;
            break;
        }
        default:
        {
            *aAtom = 0;
            return NS_ERROR_FAILURE;
        }
    }
    TX_ADDREF_ATOM(*aAtom);
    return NS_OK;
}
