/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla strings.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2000 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Scott Collins <scc@mozilla.org> (original author)
 *
 */

#ifndef nsBufferHandle_h___
#define nsBufferHandle_h___

#include <stddef.h>
  // for |ptrdiff_t|

#include "prtypes.h"
  // for |PRBool|

#include "nsDebug.h"
  // for |NS_ASSERTION|

#include "nscore.h"
  // for |PRUnichar|

  /**
   *
   */
template <class CharT>
class nsBufferHandle
  {
    public:
      nsBufferHandle( CharT* aDataStart, CharT* aDataEnd ) : mDataStart(aDataStart), mDataEnd(aDataEnd) { }

      void          DataStart( CharT* aNewDataStart )       { mDataStart = aNewDataStart; }
      CharT*        DataStart()                             { return mDataStart; }
      const CharT*  DataStart() const                       { return mDataStart; }

      void          DataEnd( CharT* aNewDataEnd )           { mDataEnd = aNewDataEnd; }
      CharT*        DataEnd()                               { return mDataEnd; }
      const CharT*  DataEnd() const                         { return mDataEnd; }

//    void          DataLength( ptrdiff_t aNewDataLength )  { mDataEnd = mDataStart+aNewDataLength; }
      ptrdiff_t     DataLength() const                      { return mDataEnd - mDataStart; }

    protected:
      CharT*  mDataStart;
      CharT*  mDataEnd;
  };


  /**
   * string allocator stuff needs to move to its own file
   * also see http://bugzilla.mozilla.org/show_bug.cgi?id=70087
   */

template <class CharT>
class nsStringAllocator
  {
    public:
      // more later
      virtual void Deallocate( CharT* ) const = 0;
  };

  /**
   * the following two routines must be provided by the client embedding strings
   */
nsStringAllocator<char>&      StringAllocator_char();
nsStringAllocator<PRUnichar>& StringAllocator_wchar_t();


  /**
   * this traits class lets templated clients pick the appropriate non-template global allocator
   */
template <class T>
struct nsStringAllocatorTraits
  {
  };

NS_SPECIALIZE_TEMPLATE
struct nsStringAllocatorTraits<char>
  {
    static nsStringAllocator<char>&      global_string_allocator() { return StringAllocator_char(); }
  };

NS_SPECIALIZE_TEMPLATE
struct nsStringAllocatorTraits<PRUnichar>
  {
    static nsStringAllocator<PRUnichar>& global_string_allocator() { return StringAllocator_wchar_t(); }
  };



  /**
   *
   */
template <class CharT>
class nsSharedBufferHandle
    : public nsBufferHandle<CharT>
  {
    protected:
      enum
        {
          kIsShared                       = 1<<31,
          kIsSingleAllocationWithBuffer   = 1<<30,  // handle and buffer are one piece, no separate deallocation is possible for the buffer
          kIsStorageDefinedSeparately     = 1<<29,  // i.e., we're using the ``flex'' structure defined below
          kIsUserAllocator                = 1<<28,  // can't |delete|, call a hook instead

          kFlagsMask                      = kIsShared | kIsSingleAllocationWithBuffer | kIsStorageDefinedSeparately | kIsUserAllocator,
          kRefCountMask                   = ~kFlagsMask
        };

    public:
      nsSharedBufferHandle( CharT* aDataStart, CharT* aDataEnd )
          : nsBufferHandle<CharT>(aDataStart, aDataEnd)
        {
          mFlags = kIsShared;
        }

      ~nsSharedBufferHandle();

      void
      AcquireReference() const
        {
          nsSharedBufferHandle<CharT>* mutable_this = NS_CONST_CAST(nsSharedBufferHandle<CharT>*, this);
          mutable_this->set_refcount( get_refcount()+1 );
        }

      void
      ReleaseReference() const
        {
          nsSharedBufferHandle<CharT>* mutable_this = NS_CONST_CAST(nsSharedBufferHandle<CharT>*, this);
          if ( !mutable_this->set_refcount( get_refcount()-1 ) )
            delete mutable_this;
        }

      PRBool
      IsReferenced() const
        {
          return get_refcount() != 0;
        }

    protected:
      PRUint32  mFlags;

      PRUint32
      get_refcount() const
        {
          return mFlags & kRefCountMask;
        }

      PRUint32
      set_refcount( PRUint32 aNewRefCount )
        {
          NS_ASSERTION(aNewRefCount <= kRefCountMask, "aNewRefCount <= kRefCountMask");

          mFlags = (mFlags & kFlagsMask) | aNewRefCount;
          return aNewRefCount;
        }

      nsStringAllocator<CharT>& get_allocator() const;
  };


  // need a name for this
template <class CharT>
class nsFlexBufferHandle
    : public nsSharedBufferHandle<CharT>
  {
    public:
      nsFlexBufferHandle( CharT* aDataStart, CharT* aDataEnd, CharT* aStorageStart, CharT* aStorageEnd )
          : nsSharedBufferHandle<CharT>(aDataStart, aDataEnd),
            mStorageStart(aStorageStart),
            mStorageEnd(aStorageEnd)
        {
          this->mFlags |= this->kIsStorageDefinedSeparately;
        }

      void          StorageStart( CharT* aNewStorageStart )       { mStorageStart = aNewStorageStart; }
      CharT*        StorageStart()                                { return mStorageStart; }
      const CharT*  StorageStart() const                          { return mStorageStart; }

      void          StorageEnd( CharT* aNewStorageEnd )           { mStorageEnd = aNewStorageEnd; }
      CharT*        StorageEnd()                                  { return mStorageEnd; }
      const CharT*  StorageEnd() const                            { return mStorageEnd; }

//    void          StorageLength( ptrdiff_t aNewStorageLength )  { mStorageEnd = mStorageStart+aNewStorageLength; }
      ptrdiff_t     StorageLength() const                         { return mStorageEnd - mStorageStart; }

    protected:
      CharT*  mStorageStart;
      CharT*  mStorageEnd;
  };

template <class CharT>
class nsSharedBufferHandleWithAllocator
    : public nsSharedBufferHandle<CharT>
  {
    public:
      nsSharedBufferHandleWithAllocator( CharT* aDataStart, CharT* aDataEnd, nsStringAllocator<CharT>& aAllocator )
          : nsSharedBufferHandle<CharT>(aDataStart, aDataEnd),
            mAllocator(aAllocator)
        {
          mFlags |= kIsUserAllocator;
        }

      nsStringAllocator<CharT>& get_allocator() const { return mAllocator; }

    protected:
      nsStringAllocator<CharT>& mAllocator;
  };

template <class CharT>
class nsFlexBufferHandleWithAllocator
    : public nsFlexBufferHandle<CharT>
  {
    public:
      nsFlexBufferHandleWithAllocator( CharT* aDataStart, CharT* aDataEnd,
                                       CharT* aStorageStart, CharT* aStorageEnd,
                                       nsStringAllocator<CharT>& aAllocator )
          : nsFlexBufferHandle<CharT>(aDataStart, aDataEnd, aStorageStart, aStorageEnd),
            mAllocator(aAllocator)
        {
          mFlags |= kIsUserAllocator;
        }

      nsStringAllocator<CharT>& get_allocator() const { return mAllocator; }

    protected:
      nsStringAllocator<CharT>& mAllocator;
  };


template <class CharT>
nsStringAllocator<CharT>&
nsSharedBufferHandle<CharT>::get_allocator() const
  {
    if ( mFlags & kIsUserAllocator )
      {
        if ( mFlags & kIsStorageDefinedSeparately )
          return NS_REINTERPRET_CAST(const nsFlexBufferHandleWithAllocator<CharT>*, this)->get_allocator();
        else
          return NS_REINTERPRET_CAST(const nsSharedBufferHandleWithAllocator<CharT>*, this)->get_allocator();
      }

    return nsStringAllocatorTraits<CharT>::global_string_allocator();
  }


template <class CharT>
nsSharedBufferHandle<CharT>::~nsSharedBufferHandle()
    // really don't want this to be |inline|
  {
    NS_ASSERTION(!IsReferenced(), "!IsReferenced()");

    if ( !(mFlags & kIsSingleAllocationWithBuffer) )
      {
        CharT* string_storage = this->mDataStart;
        if ( mFlags & kIsStorageDefinedSeparately )
          string_storage = NS_REINTERPRET_CAST(nsFlexBufferHandle<CharT>*, this)->StorageStart();

        get_allocator().Deallocate(string_storage);
        // nsMemory::Free(string_storage);
      }
  }


#endif // !defined(nsBufferHandle_h___)
