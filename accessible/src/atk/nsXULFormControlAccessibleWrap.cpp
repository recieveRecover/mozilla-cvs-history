/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Sun Microsystems, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Gilbert Fang (gilbert.fang@sun.com)
 *   Kyle Yuan (kyle.yuan@sun.com)
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

#include "nsIDOMElement.h"
#include "nsXULFormControlAccessibleWrap.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMXULTextboxElement.h"

NS_IMPL_ISUPPORTS_INHERITED2(nsXULTextFieldAccessibleWrap, nsXULTextFieldAccessible, nsIAccessibleText, nsIAccessibleEditableText)

nsXULTextFieldAccessibleWrap::nsXULTextFieldAccessibleWrap(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsXULTextFieldAccessible(aNode, aShell), nsAccessibleEditableText(aNode)
{
  nsCOMPtr<nsIDOMXULTextBoxElement> textBox(do_QueryInterface(aNode));
  NS_ASSERTION(textBox, "Not a XUL textbox!");
  if (!textBox)
    return;

  textBox->GetInputField(getter_AddRefs(mTextNode));
  if (!mTextNode)
    return;

  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(mWeakShell));
  if (!shell)
    return;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mTextNode));
  nsIFrame* frame = shell->GetPrimaryFrameFor(content);

  if (!frame)
    return;

  nsITextControlFrame *textFrame;
  frame->QueryInterface(NS_GET_IID(nsITextControlFrame), (void**)&textFrame);
  if (textFrame) {
    nsCOMPtr<nsIEditor> editor;
    textFrame->GetEditor(getter_AddRefs(editor));
    SetEditor(editor);
  }
}

NS_IMETHODIMP nsXULTextFieldAccessibleWrap::Shutdown()
{
  nsAccessibleEditableText::ShutdownEditor();
  return nsXULTextFieldAccessible::Shutdown();
}
