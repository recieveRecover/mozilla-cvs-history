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

#include "nsWindow.h"



nsWindow::nsWindow() : nsWidget()
{
  NS_INIT_REFCNT();
  name = "nsWindow";
  mBackground = NS_RGB(255, 255, 255);
  bg_pixel = xlib_rgb_xpixel_from_rgb(mBackground);
  mBackground = NS_RGB(255, 255, 255);
  border_pixel = xlib_rgb_xpixel_from_rgb(border_rgb);
}

nsWindow::~nsWindow()
{
}

void
nsWindow::DestroyNative(void)
{
  if (mGC)
    XFreeGC(gDisplay, mGC);
  if (mBaseWindow) {
    XDestroyWindow(gDisplay, mBaseWindow);
    DeleteWindowCallback(mBaseWindow);
  }
}

void nsWindow::CreateNative(Window aParent, nsRect aRect)
{
  XSetWindowAttributes attr;
  unsigned long attr_mask;

  // on a window resize, we don't want to window contents to
  // be discarded...
  attr.bit_gravity = NorthWestGravity;
  // make sure that we listen for events
  attr.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
  // set the default background color and border to that awful gray
  attr.background_pixel = bg_pixel;
  attr.border_pixel = border_pixel;
  // set the colormap
  attr.colormap = xlib_rgb_get_cmap();
  // here's what's in the struct
  attr_mask = CWBitGravity | CWEventMask | CWBackPixel | CWBorderPixel;
  // check to see if there was actually a colormap.
  if (attr.colormap)
    attr_mask |= CWColormap;

  CreateNativeWindow(aParent, mBounds, attr, attr_mask);
  // set up the GC for this window.
  if (!mBaseWindow)
    printf("*** warning: this is about to fail...\n");
  mGC = XCreateGC(gDisplay, mBaseWindow, 0, NULL);
}

NS_IMETHODIMP nsWindow::Invalidate(PRBool aIsSynchronous)
{
  printf("nsWindow::Invalidate(sync)\n");
  nsPaintEvent pevent;
  pevent.message = NS_PAINT;
  pevent.widget = this;
  pevent.eventStructType = NS_PAINT_EVENT;
  pevent.rect = new nsRect (mBounds.x, mBounds.y,
                            mBounds.height, mBounds.width);
  // XXX fix this
  pevent.time = 0;
  AddRef();
  OnPaint(pevent);
  Release();
  delete pevent.rect;
  return NS_OK;
}

NS_IMETHODIMP nsWindow::Invalidate(const nsRect & aRect, PRBool aIsSynchronous)
{
  printf("nsWindow::Invalidate(rect, sync)\n");
  nsPaintEvent pevent;
  pevent.message = NS_PAINT;
  pevent.widget = this;
  pevent.eventStructType = NS_PAINT_EVENT;
  pevent.rect = new nsRect(aRect);
  // XXX fix this
  pevent.time = 0;
  AddRef();
  OnPaint(pevent);
  Release();
  // XXX will this leak?
  //delete pevent.rect;
  return NS_OK;
}

NS_IMETHODIMP nsWindow::Update()
{
  printf("nsWindow::Update()\n");
  nsPaintEvent pevent;
  pevent.message = NS_PAINT;
  pevent.widget = this;
  pevent.eventStructType = NS_PAINT_EVENT;
  pevent.rect = new nsRect (mBounds.x, mBounds.y,
                            mBounds.height, mBounds.width);
  // XXX fix this
  pevent.time = 0;
  AddRef();
  OnPaint(pevent);
  Release();
  delete pevent.rect;
  return NS_OK;
}

ChildWindow::ChildWindow(): nsWindow()
{
  name = "nsChildWindow";
}
