/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#include "nsHTMLParts.h"
#include "nsHTMLContainer.h"
#include "nsFrame.h"
#include "nsHTMLIIDs.h"

#define nsHTMLCommentSuper nsHTMLTagContent

class nsHTMLComment : public nsHTMLCommentSuper {
public:
  nsHTMLComment(nsIAtom* aTag, const nsString& aComment);

  NS_IMETHOD CreateFrame(nsIPresContext*  aPresContext,
                         nsIFrame*        aParentFrame,
                         nsIStyleContext* aStyleContext,
                         nsIFrame*&       aResult);

  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;

protected:
  virtual ~nsHTMLComment();
  nsString mComment;
};

nsHTMLComment::nsHTMLComment(nsIAtom* aTag, const nsString& aComment)
  : nsHTMLCommentSuper(aTag),
    mComment(aComment)
{
}

nsHTMLComment::~nsHTMLComment()
{
}

NS_IMETHODIMP
nsHTMLComment::CreateFrame(nsIPresContext*  aPresContext,
                           nsIFrame*        aParentFrame,
                           nsIStyleContext* aStyleContext,
                           nsIFrame*&       aResult)
{
  nsIFrame* frame;
  nsFrame::NewFrame(&frame, this, aParentFrame);
  if (nsnull == frame) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  frame->SetStyleContext(aPresContext, aStyleContext);
  aResult = frame;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLComment::List(FILE* out, PRInt32 aIndent) const
{
  NS_PRECONDITION(nsnull != mDocument, "bad content");

  PRInt32 index;
  for (index = aIndent; --index >= 0; ) fputs("  ", out);

  nsIAtom* tag;
  GetTag(tag);
  if (tag != nsnull) {
    nsAutoString buf;
    tag->ToString(buf);
    fputs(buf, out);
    NS_RELEASE(tag);
  }

  ListAttributes(out);

  fprintf(out, " RefCount=%d<", mRefCnt);
  fputs(mComment, out);
  fputs(">\n", out);
  return NS_OK;
}

nsresult
NS_NewHTMLComment(nsIHTMLContent** aInstancePtrResult,
                  nsIAtom* aTag, const nsString& aComment)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* it = new nsHTMLComment(aTag, aComment);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void **) aInstancePtrResult);
}
