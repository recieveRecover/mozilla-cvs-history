/*
 * (C) Copyright The MITRE Corporation 1999  All rights reserved.
 *
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is mozilla.org Code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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
// Tom Kneeland (3/29/99)
//
//  Implementation of the Document Object Model Level 1 Core
//    Implementation of the NodeListDefinition class
//
// Modification History:
// Who  When      What
// TK   03/29/99  Created
//

#include "dom.h"

//
//Create an empty node list.
//
NodeListDefinition::NodeListDefinition()
{
  firstItem = nsnull;
  lastItem = nsnull;
  length = 0;
}

//
//Free up the memory used by the List of Items.  Don't delete the actual nodes
//though.
//
NodeListDefinition::~NodeListDefinition()
{
  ListItem* pDeleteItem;
  ListItem* pListTraversal = firstItem;

  while (pListTraversal)
    {
    pDeleteItem = pListTraversal;
    pListTraversal = pListTraversal->next;
    delete pDeleteItem;
    }
}

//
//Create a new ListItem, point it to the newNode, and append it to the current
//list of nodes.
//
void NodeListDefinition::append(Node* newNode)
{
  append(*newNode);
}

void NodeListDefinition::append(Node& newNode)
{
  ListItem* newListItem = new ListItem;
  if (!newListItem)
      return;

  // Setup the new list item
  newListItem->node = &newNode;
  newListItem->prev = lastItem;
  newListItem->next = nsnull;

  //Append the list item
  if (lastItem)
    lastItem->next = newListItem;

  lastItem = newListItem;

  //Adjust firstItem if this new item is being added to an empty list
  if (!firstItem)
    firstItem = lastItem;

  //Need to increment the length of the list.  Inherited from NodeList
  length++;
}

//
// Return the Node contained in the item specified
//
Node* NodeListDefinition::item(PRUint32 index)
{
  PRUint32 selectLoop;
  ListItem* pListItem = firstItem;

  if (index < length)
    {
      for (selectLoop=0;selectLoop<index;selectLoop++)
        pListItem = pListItem->next;

      return pListItem->node;
    }

  return nsnull;
}

//
// Return the number of items in the list
//
PRUint32 NodeListDefinition::getLength()
{
  return length;
}
