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
 * Copyright (C) 1999 Alexander Larsson.  All Rights Reserved.
 */
#ifndef GTKMOZILLACONTAINER_H
#define GTKMOZILLACONTAINER_H

#include <gtk/gtk.h>
#include "nsIWebShell.h"
#include "gtkmozilla.h"


class nsIWebShell;

class GtkMozillaContainer : public nsIWebShellContainer
{
public:
// nsISupports interface declaration
  NS_DECL_ISUPPORTS

// nsIWebShellContainer
  NS_IMETHOD WillLoadURL(nsIWebShell* aShell, const PRUnichar* aURL, nsLoadType aReason);
  NS_IMETHOD BeginLoadURL(nsIWebShell* aShell, const PRUnichar* aURL);
  NS_IMETHOD ProgressLoadURL(nsIWebShell* aShell, const PRUnichar* aURL, PRInt32 aProgress, PRInt32 aProgressMax);
  NS_IMETHOD EndLoadURL(nsIWebShell* aShell, const PRUnichar* aURL, PRInt32 aStatus);
  
  NS_IMETHOD NewWebShell(PRUint32 aChromeMask, PRBool aVisible,
                         nsIWebShell *&aNewWebShell);
  NS_IMETHOD FocusAvailable(nsIWebShell* aFocusedWebShell,
                            PRBool& aFocusTaken);

  NS_IMETHOD CanCreateNewWebShell(PRBool& aResult);
  NS_IMETHOD SetNewWebShellInfo(const nsString& aName, const nsString& anURL, 
                                nsIWebShell* aOpenerShell, PRUint32 aChromeMask,
                                nsIWebShell** aNewShell, nsIWebShell** anInnerShell);
  
  NS_IMETHOD FindWebShellWithName(const PRUnichar* aName,
                                  nsIWebShell*& aResult);

  NS_IMETHOD ContentShellAdded(nsIWebShell* aChildShell,
                               nsIContent* frameNode);
  
  NS_IMETHOD CreatePopup(nsIDOMElement* aElement, nsIDOMElement* aPopupContent, 
                         PRInt32 aXPos, PRInt32 aYPos, 
                         const nsString& aPopupType, const nsString& aPopupAlignment,
                         nsIDOMWindow* aWindow);

  NS_IMETHOD ChildShellAdded(nsIWebShell** aChildShell, nsIContent* frameNode);


// Construction
  GtkMozillaContainer(GtkMozilla *moz);        // standard constructor
  virtual ~GtkMozillaContainer();

  void Show();
  void Resize(gint w, gint h);
  
  void LoadURL(const char *url);
  void Stop();
  void Reload(GtkMozillaReloadType type);

  gint Back();
  gint CanBack();
  gint Forward();
  gint CanForward();
  gint GoTo(gint history_index);
  gint GetHistoryLength();
  gint GetHistoryIndex();

private:
  nsIWebShell *mWebShell;
  GtkMozilla *mozilla;
  int width, height;
};

#endif /* GTKMOZILLACONTAINER_H */
