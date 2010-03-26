/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(INCLUDED_MEMORY_ALLOCATOR_H)
#define INCLUDED_MEMORY_ALLOCATOR_H

#include <memory>

#if 0

#define DefaultAllocator std::allocator

#else

/// \brief An allocator that uses c++ new/delete.
/// Compliant with the std::allocator interface.
template<typename Type>
class DefaultAllocator
{
public:

  typedef Type value_type;
  typedef value_type* pointer;
  typedef const Type* const_pointer;
  typedef Type& reference;
  typedef const Type& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  template<typename Other>
  struct rebind
  {
    typedef DefaultAllocator<Other> other;
  };

  DefaultAllocator()
  {
  }
  DefaultAllocator(const DefaultAllocator<Type>&)
  {
  }
  template<typename Other> DefaultAllocator(const DefaultAllocator<Other>&)
  {
  }
  virtual ~DefaultAllocator()
  {
  }

  pointer address(reference instance) const
  {
    return &instance;
  }
  const_pointer address(const_reference instance) const
  {
    return &instance;
  }
  Type* allocate(size_type size, const void* = 0)
  { 
    return static_cast<Type*>(::operator new(size * sizeof(Type)));
  }
  void deallocate(pointer p, size_type)
  {
    ::operator delete(p);
  }
  size_type max_size() const
  {
		return std::size_t(-1) / sizeof (Type);
  }
  void construct(pointer p, const Type& value)
  {
    new(p) Type(value);
  }
  void destroy(pointer p)
  {
    p->~Type();
  }
};

template<typename Type, typename Other>
inline bool operator==(const DefaultAllocator<Type>&, const DefaultAllocator<Other>&)
{ 
    return true;
}
template<typename Type, typename OtherAllocator>
inline bool operator==(const DefaultAllocator<Type>&, const OtherAllocator&)
{ 
    return false; 
}

#endif

#endif
