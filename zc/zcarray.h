/*
 * - Visual Studio "autoexp.dat" info. -Gleeok
 *
;------------------------------------------------------------------------------
;  Array
;------------------------------------------------------------------------------
Array<*>{
   children
   (
      #array
      (
         expr :      ($e._ptr)[$i],
         size :      $e._size
      )
   )
   preview
   (
      #(
         "[", $e._size, "](",
         #array
         (
            expr :   ($e._ptr)[$i],
            size :   $e._size
         ),
         ")"
      )
   )
}
ArrayIterator<*>{
   preview     ( #(*$e._ptr))
   children ( #(ptr: *$e._ptr))
}
*/

#include "zdefs.h"

#ifndef __ZCARRAY_H_
#define __ZCARRAY_H_

template <typename T> class ZCArrayIterator;
template <typename T> class ZCArray;

template <class T>
class ZCArrayIterator
{
public:
   friend class ZCArray<T>;
   typedef unsigned int size_type;
   typedef const T &const_reference;
   typedef const T *const_pointer;
   typedef T &reference;
   typedef T *pointer;

   ZCArrayIterator() : _ptr(0) {}
   ZCArrayIterator(T *_Tptr) : _ptr(_Tptr) {}
   ZCArrayIterator(const ZCArrayIterator &_It) : _ptr(_It._ptr) {}
   ~ZCArrayIterator() {}

   bool operator == (const ZCArrayIterator &v) const
   {
      return (_ptr == v._ptr);
   }
   bool operator != (const ZCArrayIterator &v) const
   {
      return (_ptr != v._ptr);
   }
   bool operator < (const ZCArrayIterator &v) const
   {
      return (_ptr <  v._ptr);
   }
   bool operator > (const ZCArrayIterator &v) const
   {
      return (_ptr >  v._ptr);
   }
   bool operator <= (const ZCArrayIterator &v) const
   {
      return (_ptr <= v._ptr);
   }
   bool operator >= (const ZCArrayIterator &v) const
   {
      return (_ptr >= v._ptr);
   }

   pointer         operator ->()
   {
      return &(*_ptr);
   }
   const_pointer    operator ->() const
   {
      return &(*_ptr);
   }
   reference      operator  *()
   {
      return (*_ptr);
   }
   const_reference  operator  *() const
   {
      return (*_ptr);
   }
   reference      operator [](size_type _Index)
   {
      return (*(_ptr + _Index));
   }
   const_reference  operator [](size_type _Index) const
   {
      return (*(_ptr + _Index));
   }

   ZCArrayIterator &operator ++ ()
   {
      ++_ptr;
      return *this;
   }
   ZCArrayIterator &operator -- ()
   {
      --_ptr;
      return *this;
   }
   ZCArrayIterator  operator ++ (int)
   {
      pointer _Tmp = _ptr;
      _ptr++;
      return ZCArrayIterator(_Tmp);
   }
   ZCArrayIterator  operator -- (int)
   {
      pointer _Tmp = _ptr;
      _ptr--;
      return ZCArrayIterator(_Tmp);
   }

   ZCArrayIterator &operator += (int _Offset)
   {
      _ptr += _Offset;
      return *this;
   }
   ZCArrayIterator &operator -= (int _Offset)
   {
      _ptr -= _Offset;
      return *this;
   }
   ZCArrayIterator  operator + (int _Offset) const
   {
      return ZCArrayIterator(_ptr + _Offset);
   }
   ZCArrayIterator  operator - (int _Offset) const
   {
      return ZCArrayIterator(_ptr - _Offset);
   }

protected:
   pointer _ptr;
};


template <typename T>
class ZCArray
{
public:
   friend class ZCArrayIterator<T>;
   typedef const ZCArrayIterator<T> const_iterator;
   typedef ZCArrayIterator<T> iterator;
   typedef unsigned int size_type;
   typedef const T &const_reference;
   typedef const T *const_pointer;
   typedef const T const_type;
   typedef T &reference;
   typedef T *pointer;
   typedef T type;

   ZCArray() : _ptr(NULL), _size(0)
   {
      for (int i = 0; i < 4; i++)
         _dim[i] = 0;
   }

   ZCArray(size_type _Size) : _ptr(NULL)
   {
      _SetDimensions(0, 0, _Size);
      _Alloc(_size);
   }

   ZCArray(size_type _Y, size_type _X) : _ptr(NULL)
   {
      _SetDimensions(0, _Y, _X);
      _Alloc(_size);
   }

   ZCArray(size_type _Z, size_type _Y, size_type _X) : _ptr(NULL)
   {
      _SetDimensions(_Z, _Y, _X);
      _Alloc(_size);
   }

   ZCArray(const ZCArray &_Array) : _ptr(NULL), _size(0)
   {
      for (int i = 0; i < 4; i++) _dim[i] = 0;

      Copy(_Array);
   }

   ~ZCArray()
   {
      _Delete();
   }

   const ZCArray &operator = (const ZCArray &_Array)
   {
      if (this != &_Array)
         Copy(_Array);

      return *this;
   }

   bool operator == (const ZCArray &_Array) const
   {
      if (_size != _Array._size)
         return false;

      for (size_type i(0); i < _size; i++)
         if (*(_ptr + i) != *(_Array._ptr + i))
            return false;

      return true;
   }

   bool operator != (const ZCArray &right) const
   {
      return !(*this == right);
   }

   reference operator()(size_type _X)
   {
      return _ptr[ _X ];
   }
   reference operator()(size_type _Y, size_type _X)
   {
      return _ptr[ _X + _Y * _dim[0] ];
   }
   reference operator()(size_type _Z, size_type _Y, size_type _X)
   {
      return _ptr[ _X + _Y * _dim[0] + _Z * _dim[3] ];
   }

   const_reference operator()(size_type _X) const
   {
      return _ptr[ _X ];
   }
   const_reference operator()(size_type _Y, size_type _X) const
   {
      return _ptr[ _X + _Y * _dim[0] ];
   }
   const_reference operator()(size_type _Z, size_type _Y, size_type _X) const
   {
      return _ptr[ _X + _Y * _dim[0] + _Z * _dim[3] ];
   }


   pointer     operator ->()
   {
      return &(*_ptr);
   }
   const_pointer  operator ->() const
   {
      return &(*_ptr);
   }
   type        operator * ()
   {
      return *_ptr;
   }
   const_type     operator * () const
   {
      return *_ptr;
   }
   reference      operator [](const size_type &i)
   {
      return _ptr[i];
   }
   const_reference operator [](const size_type &i) const
   {
      return _ptr[i];
   }

   reference At(size_type _X)
   {
      if (!_Bounds(_X))
         return _ptr[ 0 ]; //Must be a better way of returning an invalied element in a template than this...

      return _ptr[ _X ];
   }
   reference At(const size_type &_Y, const size_type &_X)
   {
      if (!_Bounds(_X, _Y))
         return _ptr[ 0 ];

      return _ptr[ Offset(_Y, _X) ];
   }
   reference At(const size_type &_Z, const size_type &_Y, const size_type &_X)
   {
      if (!_BoundsExp(_X, _Y, _Z))
         return _ptr[ 0 ];

      return _ptr[ Offset(_Z, _Y, _X) ];
   }

   reference StrictAt(size_type _X)
   {
      _BoundsExp(_X);
      return _ptr[ _X ];
   }
   reference StrictAt(size_type _Y, size_type _X)
   {
      _BoundsExp(_X, _Y);
      return _ptr[ Offset(_Y, _X) ];
   }
   reference StrictAt(size_type _Z, size_type _Y, size_type _X)
   {
      _BoundsExp(_X, _Y, _Z);
      return _ptr[ Offset(_Z, _Y, _X) ];
   }

   reference Front()
   {
      return *_ptr;
   }
   reference Back()
   {
      return _ptr[ _size - 1 ];
   }
   const_reference Front() const
   {
      return *_ptr;
   }
   const_reference Back() const
   {
      return _ptr[ _size - 1 ];
   }

   size_type Offset(const size_type &_Z, const size_type &_Y, const size_type &_X) const
   {
      return (_X + _Y * _dim[0] + _Z * _dim[3]);
   }
   size_type Offset(const size_type &_Y, const size_type &_X) const
   {
      return (_X + _Y * _dim[0]);
   }

   iterator Begin()
   {
      return iterator(_ptr);
   }
   iterator End()
   {
      return iterator(_ptr + _size);
   }
   const_iterator Begin() const
   {
      return const_iterator(_ptr);
   }
   const_iterator End()   const
   {
      return const_iterator(_ptr + _size);
   }

   size_type Size() const
   {
      return _size;
   }
   bool Empty() const
   {
      return (_size == 0);
   }

   size_type Offset(const size_type _Z, const size_type _Y, const size_type _X) const
   {
      return (_X + _Y * _dim[0] + _Z * _dim[3]);
   }
   size_type Offset(const size_type _Y, const size_type _X) const
   {
      return (_X + _Y * _dim[0]);
   }

   void Assign(const size_type _Begin, const size_type _End, const type &_Val = type())
   {
      for (size_type i(_Begin); i < _End; i++)
         _ptr[ i ] = _Val;
   }

   void Resize(const size_type _Size)
   {
      Resize(0, 0, _Size);
   }
   void Resize(const size_type _Y, const size_type _X)
   {
      Resize(0, _Y, _X);
   }
   void Resize(const size_type _Z, const size_type _Y, const size_type _X)
   {
      const size_type _OldSize = _size;
      const size_type _NewSize = _GetSize(_Z, _Y, _X);

      _SetDimensions(_Z, _Y, _X);

      if (_NewSize == 0)
      {
         _Delete();
         _size = _NewSize;
      }
      else if (_OldSize != _NewSize)
         _ReAssign(_OldSize, _NewSize);
   }

   void Copy(const ZCArray &_Array)
   {
      if (_Array.Empty())
      {
         Clear();
         return;
      }

      if (_size != _Array.Size())
         _Alloc(_Array.Size());

      for (int i = 0; i < 4; i++)
         _dim[ i ] = _Array._dim[ i ];

      for (size_type i(0); i < _size; i++)
         _ptr[ i ] = _Array._ptr[ i ];
   }

   void GetDimensions(size_type _4dim[]) const
   {
      _4dim[0] = _dim[0];
      _4dim[1] = _dim[1];
      _4dim[2] = _dim[2];
      _4dim[3] = _dim[3];
   }

   void Clear()
   {
      Resize(0);
   }


protected:

   void _Alloc(size_type size)
   {
      if (_ptr)
         _Delete();

      if (size == 0)
      {
         zc_message("Tried to allocate zero sized array");
         size = 1;
      }

      _ptr = new type[ size ];
      _size = size;
   }

   void _ReAssign(const size_type _OldSize, const size_type _NewSize)
   {
      pointer _oldPtr = _ptr;
      _ptr = new type[ _NewSize ];

      const size_type _copyRange = (_OldSize < _NewSize ? _OldSize : _NewSize);

      for (size_type i(0); i < _copyRange; i++)
         _ptr[ i ] = _oldPtr[ i ];

      _Delete(_oldPtr);
      _size = _NewSize;
   }

   void _Delete()
   {
      if (_ptr)
         delete [] _ptr;

      _ptr = NULL;

      _size = 0;
   }

   void _Delete(pointer _Ptr)
   {
      if (_Ptr)
         delete [] _Ptr;

      _Ptr = NULL;
   }

   void _SetDimensions(size_type _Z, size_type _Y, size_type _X)
   {
      _dim[0] = _X;
      _dim[1] = _Y;
      _dim[2] = _Z;
      _dim[3] = (_X * _Y);

      _size = _GetSize(_X, _Y, _Z);
   }

   size_type _GetSize(size_type _Z, size_type _Y, size_type _X) const
   {
      if (_Z > 0)
         return (_X * _Y * _Z);
      else if (_Y > 0)
         return (_X * _Y);

      return (_X);
   }

   void _BoundsExp(size_type _X)
   {
      if (_X >= _size)
      {
         zc_message("Array indices out of range.");
         throw ("Array indices out of range.");
      }
   }

   void _BoundsExp(size_type _X, size_type _Y) const
   {
      if (Offset(_Y, _X) >= _size)
      {
         zc_message("Array indices out of range.");
         throw ("Array indices out of range.");
      }
   }

   void _BoundsExp(size_type _X, size_type _Y, size_type _Z) const
   {
      if (Offset(_Z, _Y, _X) >= _size)
      {
         zc_message("Array indices out of range.");
         throw ("Array indices out of range.");
      }
   }

   bool _Bounds(size_type _X)
   {
      if (_X >= _size)
      {
         zc_message("Array index (%i) out of range (%i).", _X, _size);
         return false;
      }

      return true;
   }

   bool _Bounds(size_type _X, size_type _Y) const
   {
      if (Offset(_Y, _X) >= _size)
      {
         zc_message("Array indices out of range.");
         return false;
      }

      return true;
   }

   bool _Bounds(size_type _X, size_type _Y, size_type _Z) const
   {
      if (Offset(_Z, _Y, _X) >= _size)
      {
         zc_message("Array indices out of range.");
         return false;
      }

      return true;
   }

private:
   pointer _ptr;
   size_type _size;
   size_type _dim[ 4 ];

};

#endif
