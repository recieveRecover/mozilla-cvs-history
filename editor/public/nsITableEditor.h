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


#ifndef nsITableEditor_h__
#define nsITableEditor_h__

  // Wrapping includes can speed up compiles (see "Large Scale C++ Software Design")
#ifndef nsCOMPtr_h___
#include "nsCOMPtr.h"
#endif

#ifndef nsIDOMElement_h__
#include "nsIDOMElement.h"
	// for |nsIDOMElement| because an |nsCOMPtr<nsIDOMElement>&| is used, below
#endif


#define NS_ITABLEEDITOR_IID                   \
{ /* 4805e684-49b9-11d3-9ce4-ed60bd6cb5bc} */ \
0x4805e684, 0x49b9, 0x11d3,                   \
{ 0x9c, 0xe4, 0xed, 0x60, 0xbd, 0x6c, 0xb5, 0xbc } }

class nsITableEditor : public nsISupports
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_ITABLEEDITOR_IID; return iid; }

  /* ------------ Table editing Methods -------------- */

  /** Insert table methods
    * Insert relative to the selected cell or the 
    *  cell enclosing the selection anchor
    * The selection is collapsed and is left in the new cell
    *  at the same row,col location as the original anchor cell
    *
    * @param aNumber    Number of items to insert
    * @param aAfter     If TRUE, insert after the current cell,
    *                     else insert before current cell
    */
  NS_IMETHOD InsertTableCell(PRInt32 aNumber, PRBool aAfter)=0;
  NS_IMETHOD InsertTableColumn(PRInt32 aNumber, PRBool aAfter)=0;
  NS_IMETHOD InsertTableRow(PRInt32 aNumber, PRBool aAfter)=0;

  /** Delete table methods
    * Delete starting at the selected cell or the 
    *  cell (or table) enclosing the selection anchor
    * The selection is collapsed and is left in the 
    *  cell at the same row,col location as
    *  the previous selection anchor, if possible,
    *  else in the closest neigboring cell
    *
    * @param aNumber    Number of items to insert/delete
    */
  NS_IMETHOD DeleteTable()=0;
  NS_IMETHOD DeleteTableCell(PRInt32 aNumber)=0;
  NS_IMETHOD DeleteTableCellContents()=0;
  NS_IMETHOD DeleteTableColumn(PRInt32 aNumber)=0;
  NS_IMETHOD DeleteTableRow(PRInt32 aNumber)=0;

  /** Join the contents of the selected cells into one cell,
    *   expanding that cells ROWSPAN and COLSPAN to take up
    *   the same number of cellmap locations as before.
    *   Cells whose contents were moved are deleted.
    *   If there's one cell selected or caret is in one cell,
    *     it is joined with the cell to the right, if it exists
    */
  NS_IMETHOD JoinTableCells()=0;


  /** Scan through all rows and add cells as needed so 
    *   all locations in the cellmap are occupied.
    *   Used after inserting single cells or pasting
    *   a collection of cells that extend past the
    *   previous size of the table
    * If aTable is null, it uses table enclosing the selection anchor
    * This doesn't doesn't change the selection,
    *   thus it can be used to fixup all tables
    *   in a page independant of the selection
    */
  NS_IMETHOD NormalizeTable(nsIDOMElement *aTable)=0;

  /** Get the row an column index from the layout's cellmap
    * If aTable is null, it will try to find enclosing table of selection ancho
    * 
    */
  NS_IMETHOD GetCellIndexes(nsIDOMElement *aCell, PRInt32& aRowIndex, PRInt32& aColIndex)=0;

  /** Get the number of rows and columns in a table from the layout's cellmap
    * If aTable is null, it will try to find enclosing table of selection ancho
    * Note that all rows in table will not have this many because of 
    * ROWSPAN effects or if table is not "rectangular" (has short rows)
    */
  NS_IMETHOD GetTableSize(nsIDOMElement *aTable, PRInt32& aRowCount, PRInt32& aColCount)=0;

  /** Get a cell element at cellmap grid coordinates
    * A cell that spans across multiple cellmap locations will
    *   be returned multiple times, once for each location it occupies
    *
    * @param aTable                   A table in the document
    * @param aRowIndex, aColIndex     The 0-based cellmap indexes
    *
    * Note that this returns NS_TABLELAYOUT_CELL_NOT_FOUND 
    *   when a cell is not found at the given indexes,
    *   but this passes the NS_SUCCEEDED() test,
    *   so you can scan for all cells in a row or column
    *   by iterating through the appropriate indexes
    *   until the returned aCell is null
    */
  NS_IMETHOD GetCellAt(nsIDOMElement* aTable, PRInt32 aRowIndex, PRInt32 aColIndex, nsIDOMElement* &aCell)=0;

  /** Get a cell at cellmap grid coordinates and associated data
    * A cell that spans across multiple cellmap locations will
    *   be returned multiple times, once for each location it occupies
    * Examine the returned aStartRowIndex and aStartColIndex to see 
    *   if it is in the same layout column or layout row:
    *   A "layout row" is all cells sharing the same top edge
    *   A "layout column" is all cells sharing the same left edge
    *   This is important to determine what to do when inserting or deleting a column or row
    * 
    *  @param aTable                   A table in the document
    *  @param aRowIndex, aColIndex     The 0-based cellmap indexes
    * returns values:
    *  @param aCell                    The cell at this cellmap location
    *  @param aStartRowIndex           The row index where cell starts
    *  @param aStartColIndex           The col index where cell starts
    *  @param aRowSpan                 May be 0 (to span down entire table) or number of cells spanned
    *  @param aColSpan                 May be 0 (to span across entire table) or number of cells spanned
    *  @param aActualRowSpan           The actual number of cellmap locations (rows) spanned by the cell
    *  @param aActualColSpan           The actual number of cellmap locations (columns) spanned by the cell
    *  @param aIsSelected
    *  @param 
    *
    * Note that this returns NS_TABLELAYOUT_CELL_NOT_FOUND
    *   when a cell is not found at the given indexes  (see note for GetCellAt())
    */
  NS_IMETHOD GetCellDataAt(nsIDOMElement* aTable, PRInt32 aRowIndex, PRInt32 aColIndex, nsIDOMElement* &aCell,
                           PRInt32& aStartRowIndex, PRInt32& aStartColIndex,
                           PRInt32& aRowSpan, PRInt32& aColSpan, 
                           PRInt32& aActualRowSpan, PRInt32& aActualColSpan, 
                           PRBool& aIsSelected)=0;

  /** Get the first row element in a table
    *
    * @param aTableElement Any TABLE or child-of-table element in the document
    * @param aRowIndex     The 0-based index of the row
    *
    * Returns:
    * @param aRow        The row at the requested index
    *                    Returns null if there are no rows in table
    */
  NS_IMETHOD GetFirstRow(nsIDOMElement* aTableElement, nsIDOMElement* &aRow)=0;

  /** Get the next row element starting the search from aTableElement
    *
    * @param aTableElement Any TR or child-of-TR element in the document
    *
    * Returns:
    * @param aRow        The row to start search from
    *                    and the row returned from the search
    *                    Returns null if there isn't another row
    */
  NS_IMETHOD GetNextRow(nsIDOMElement* aTableElement, nsIDOMElement* &aRow)=0;
  
  /** Preferred direction to search for neighboring cell
    * when trying to locate a cell to place caret in after
    * a table editing action. 
    * Used for aDirection param in SetCaretAfterTableEdit
    */
  enum { 
    eNoSearch, 
    ePreviousColumn, 
    ePreviousRow 
  };
  /** Reset a collapsed selection (the caret) after table editing
    *
    * @param aTable      A table in the document
    * @param aRow        The row ...
    * @param aCol        ... and column defining the cell
    *                    where we will try to place the caret
    * @param aDirection  If cell at (aCol, aRow) is not found,
    *                    search for previous cell in the same
    *                    column (aPreviousColumn) or row (ePreviousRow)
    *                    or don't search for another cell (aNoSearch)
    *                    If no cell is found, caret is place just before table;
    *                    and if that fails, at beginning of document.
    *                    Thus we generally don't worry about the return value
    *                     and can use the nsSetCaretAfterTableEdit stack-based 
    *                     object to insure we reset the caret in a table-editing method.
    */
  NS_IMETHOD SetCaretAfterTableEdit(nsIDOMElement* aTable, PRInt32 aRow, PRInt32 aCol, PRInt32 aDirection)=0;

  /** Examine the current selection and find
    *   a selected TABLE, TD or TH, or TR element.
    *   or return the parent TD or TH if selection is inside a table cell
    *   Returns null if no table element is found.
    *
    * @param aTableElement      The table element (table, row, or cell) returned
    * @param aTagName           The tagname of returned element
    *                           Note that "td" will be returned if name is actually "th"
    * @param aIsSelected        Tells if element returned is a selected element 
    *                           (false if element is a parent cell of selection)
    */
  NS_IMETHOD GetSelectedOrParentTableElement(nsIDOMElement* &aTableElement, nsString& aTagName, PRBool &aIsSelected)=0;

};


#endif // nsITableEditor_h__
