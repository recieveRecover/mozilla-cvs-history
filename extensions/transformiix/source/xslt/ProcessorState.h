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
 *    -- original author.
 *
 * $Id: ProcessorState.h,v 1.15 2001/06/26 11:58:52 sicking%bigfoot.com Exp $
 */



#ifndef TRANSFRMX_PROCESSORSTATE_H
#define TRANSFRMX_PROCESSORSTATE_H

#include "DOMHelper.h"
#include "NodeSet.h"
#include "NodeStack.h"
#include "Stack.h"
#include "ErrorObserver.h"
#include "NamedMap.h"
#include "ExprParser.h"
#include "Expr.h"
#include "StringList.h"
#include "OutputFormat.h"

class txXSLKey;

/**
 * Class used for keeping the current state of the XSL Processor
 * @author <a href="mailto:kvisco@ziplink.net">Keith Visco</a>
 * @version $Revision: 1.15 $ $Date: 2001/06/26 11:58:52 $
**/
class ProcessorState : public ContextState {

public:

    static const String wrapperNSPrefix;
    static const String wrapperName;
    static const String wrapperNS;

    /**
     * Creates a new ProcessorState
    **/
    ProcessorState();

    /**
     * Creates a new ProcessorState for the given XSL document
     * And result Document
    **/
    ProcessorState(Document& xslDocument, Document& resultDocument);

    /**
     * Destroys this ProcessorState
    **/
    ~ProcessorState();

    /**
     *  Adds the given attribute set to the list of available named attribute sets
     * @param attributeSet the Element to add as a named attribute set
    **/
    void addAttributeSet(Element* attributeSet);

    /**
     * Registers the given ErrorObserver with this ProcessorState
    **/
    void addErrorObserver(ErrorObserver& errorObserver);


    /**
     * Adds the given XSL document to the list of includes
     * The href is used as a key for the include, to prevent
     * including the same document more than once
    **/
    void addInclude(const String& href, Document* xslDocument);

    /**
     *  Adds the given template to the list of templates to process
     * @param xslTemplate, the Element to add as a template
    **/
    void addTemplate(Element* xslTemplate);

    /**
     *  Adds the given Node to the Result Tree
     *
    **/
    MBool addToResultTree(Node* node);

    /**
     * Copies the node using the rules defined in the XSL specification
    **/
    Node* copyNode(Node* node);

    /**
     * Generates a unique ID for the given node and places the result in
     * dest
    **/
    void generateId(Node* node, String& dest);

    /**
     * Returns the AttributeSet associated with the given name
     * or null if no AttributeSet is found
    **/
    NodeSet* getAttributeSet(const String& name);

    /**
     * Returns the source node currently being processed
    **/
    Node* getCurrentNode();

    /**
     * Gets the default Namespace URI stack.
    **/ 
    Stack* getDefaultNSURIStack();

    /**
     * @return the included xsl document that was associated with the
     * given href, or null if no document is found 
    **/
    Document* getInclude(const String& href);

    /**
     * Returns the template associated with the given name, or
     * null if not template is found
    **/
    Element* getNamedTemplate(String& name);

    /**
     * Returns the NodeStack which keeps track of where we are in the
     * result tree
     * @return the NodeStack which keeps track of where we are in the
     * result tree
    **/
    NodeStack* getNodeStack();

    /**
     * Returns the OutputFormat which contains information on how
     * to serialize the output. I will be removing this soon, when
     * change to an event based printer, so that I can serialize
     * as I go
    **/
    OutputFormat* getOutputFormat();

    
    Stack*     getVariableSetStack();

    Expr*        getExpr(const String& pattern);
    PatternExpr* getPatternExpr(const String& pattern);

    /**
     * Returns a pointer to the result document
    **/
    Document* getResultDocument();

    /**
     * Returns the namespace URI for the given name, this method should
     * only be called to get a namespace declared within the result
     * document.
    **/
    void getResultNameSpaceURI(const String& name, String& nameSpaceURI);

    /**
     * Returns a pointer to a list of available templates
    **/
    NodeSet* getTemplates();

    String& getXSLNamespace();

    /**
     * Finds a template for the given Node. Only templates without
     * a mode attribute will be searched.
    **/
    Element* findTemplate(Node* node, Node* context);

    /**
     * Finds a template for the given Node. Only templates with
     * a mode attribute equal to the given mode will be searched.
    **/
    Element* findTemplate(Node* node, Node* context, String* mode);

    /**
     * Determines if the given XSL node allows Whitespace stripping
    **/
    MBool isXSLStripSpaceAllowed(Node* node);

    /**
     * Adds the set of names to the Whitespace preserving element set
    **/
    void preserveSpace(String& names);

    /**
     * Removes the current XSLT action from the top of the stack.
     * @returns the XSLT action after removing from the top of the stack
    **/
    Node* popAction();

    /**
     * Removes and returns the current node being processed from the stack
     * @return the current node
    **/
    Node* popCurrentNode();

    /**
     * Adds the given XSLT action to the top of the action stack
    **/
    void pushAction(Node* xsltAction);

    /**
     * Sets the given source node as the "current node" being processed
     * @param node the source node currently being processed
    **/
    void pushCurrentNode(Node* node);


    /**
     * Sets a new default Namespace URI. This is used for the Result Tree
    **/ 
    void setDefaultNameSpaceURIForResult(const String& nsURI);

    /**
     * Sets the output method. Valid output method options are,
     * "xml", "html", or "text".
    **/ 
    void setOutputMethod(const String& method);

    /**
     * Adds the set of names to the Whitespace stripping element set
    **/
    void stripSpace(String& names);

    /**
     * Adds a document to set of loaded documents
    **/
    void addLoadedDocument(Document* doc, String& location);
    
    /**
     * Returns a loaded document given it's url. NULL if no such doc exists
    **/
    Document* getLoadedDocument(String& url);

    /**
     * Adds the supplied xsl:key to the set of keys
    **/
    MBool addKey(Element* xslKey);

    /**
     * Returns the key with the supplied name
     * returns NULL if no such key exists
    **/
    txXSLKey* getKey(String& keyName);

    //-------------------------------------/
    //- Virtual Methods from ContextState -/
    //-------------------------------------/

    /**
     * Returns the parent of the given Node. This method is needed 
     * beacuse with the DOM some nodes such as Attr do not have parents
     * @param node the Node to find the parent of
     * @return the parent of the given Node, or null if not found
    **/
    virtual Node* getParentNode(Node* node);

    /**
     * Returns the value of a given variable binding within the current scope
     * @param the name to which the desired variable value has been bound
     * @return the ExprResult which has been bound to the variable with
     *  the given name
    **/
    virtual ExprResult* getVariable(String& name);

    /**
     * Returns the Stack of context NodeSets
     * @return the Stack of context NodeSets
    **/
    virtual Stack* getNodeSetStack();

    /**
     * Determines if the given XML node allows Whitespace stripping
    **/
    virtual MBool isStripSpaceAllowed(Node* node);

    /**
     *  Notifies this Error observer of a new error, with default
     *  level of NORMAL
    **/
    virtual void recieveError(String& errorMessage);

    /**
     *  Notifies this Error observer of a new error using the given error level
    **/
    virtual void recieveError(String& errorMessage, ErrorLevel level);

    /**
     * Returns a call to the function that has the given name.
     * This method is used for XPath Extension Functions.
     * @return the FunctionCall for the function with the given name.
    **/
    virtual FunctionCall* resolveFunctionCall(const String& name);

    /**
     * Sorts the given NodeSet by DocumentOrder. 
     * @param nodes the NodeSet to sort
     * <BR />
     * <B>Note:</B> I will be moving this functionality elsewhere soon
    **/
    virtual void sortByDocumentOrder(NodeSet* nodes);

    //------------------------------------------/
    //- Virtual Methods from NamespaceResolver -/
    //------------------------------------------/

    /**
     * Returns the namespace URI for the given name, this method should
     * only be called to get a namespace declared within the context (ie.
     * the stylesheet).
    **/ 
    void getNameSpaceURI(const String& name, String& nameSpaceURI);

    /**
     * Returns the namespace URI for the given namespace prefix. This method
     * should only be called to get a namespace declared within the
     * context (ie. the stylesheet).
    **/
    void getNameSpaceURIFromPrefix(const String& prefix, String& nameSpaceURI);

private:

    enum XMLSpaceMode {STRIP = 0, DEFAULT, PRESERVE};

    struct XSLTAction {
        Node* node;
        XSLTAction* prev;
    };

    NodeStack currentNodeStack;

    /**
     * Allows us to overcome some DOM deficiencies
    **/
    DOMHelper domHelper;

    /**
     * The list of ErrorObservers registered with this ProcessorState
    **/
    List  errorObservers;

    /**
     * A map for included stylesheets 
     * (used for deletion when processing is done)
    **/
    NamedMap includes;

    /**
     * A map for named attribute sets
    **/
     NamedMap      namedAttributeSets;

    /**
     * A map for named templates
    **/
    NamedMap       namedTemplates;

    /**
     * Current stack of nodes, where we are in the result document tree
    **/
    NodeStack*     resultNodeStack;


    /**
     * The output format used when serializing the result
    **/
    OutputFormat format;

    /**
     * The set of whitespace preserving elements
    **/
    StringList       wsPreserve;

    /**
     * The set of whitespace stripping elements
    **/
    StringList       wsStrip;

    /**
     * The set of whitespace stripping elements
    **/
    XMLSpaceMode       defaultSpace;

    /**
     * A set of all availabe templates
    **/
    NodeSet        templates;
    
    /**
     * the set of loaded documents
    **/
    NamedMap       loadedDocuments;
    
    /**
     * The set of all available keys
    **/
    NamedMap       xslKeys;


    XSLTAction*    currentAction;
    Stack          nodeSetStack;
    Document*      xslDocument;
    Document*      resultDocument;
    NamedMap       exprHash;
    NamedMap       patternExprHash;
    Stack          variableSets;
    ExprParser     exprParser;
    String         xsltNameSpace;
    NamedMap       nameSpaceMap;
    StringList     nameSpaceURIList;
    Stack          defaultNameSpaceURIStack;
    Stack          xsltNameSpaces;

    /**
     * Returns the closest xml:space value for the given node
    **/
    XMLSpaceMode getXMLSpaceMode(Node* node);

    /**
     * Initializes the ProcessorState
    **/
    void initialize();

}; //-- ProcessorState


#endif

