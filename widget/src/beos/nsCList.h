/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef CLIST_H
#define CLIST_H

#include <stddef.h>

// -----------------------------------------------------------------------
//
// Simple circular linked-list implementation...
//
// -----------------------------------------------------------------------

// Foreward declarations...
struct CList;

#define OBJECT_PTR_FROM_CLIST(className, listElement) \
            ((char*)listElement - offsetof(className, m_link))
                

struct CList {
    CList *next;
    CList *prev;

    CList() {
        next = prev = this;
    }

    ~CList() {
        Remove();
    }
        
    //
    // Append an element to the end of this list
    //
    void Append(CList &element) {
        element.next = this;
        element.prev = prev;
        prev->next = &element;
        prev = &element;
    }

    //
    // Add an element to the beginning of this list
    //
    void Add(CList &element) {
        element.next = next;
        element.prev = this;
        next->prev = &element;
        next = &element;
    }

    //
    // Append this element to the end of a list
    //
    void AppendToList(CList &list) {
        list.Append(*this);
    }

    //
    // Add this element to the beginning of a list
    //
    void AddToList(CList &list) {
        list.Add(*this);
    }

    //
    // Remove this element from the list and re-initialize
    //
    void Remove(void) {
        prev->next = next;
        next->prev = prev;
 
        next = prev = this;
    }

    //
    // Is this list empty ?
    //
    bool IsEmpty(void) {
        return (next == this);
    }
};    

#endif // CLIST_H

