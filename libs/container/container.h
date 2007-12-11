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

#if !defined(INCLUDED_CONTAINER_CONTAINER_H)
#define INCLUDED_CONTAINER_CONTAINER_H

#include <list>
#include <set>

#include "generic/static.h"

/// \brief A single-value container, which can either be empty or full.
template<typename Type>
class Single
{
  Type* m_value;
public:
  Single() : m_value(0)
  {
  }
  bool empty()
  {
    return m_value == 0;
  }
  Type* insert(const Type& other)
  {
    m_value = new Type(other);
    return m_value;
  }
  void clear()
  {
    delete m_value;
    m_value = 0;
  }
  Type& get()
  {
    //ASSERT_MESSAGE(!empty(), "Single: must be initialised before being accessed");
    return *m_value;
  }
  const Type& get() const
  {
    //ASSERT_MESSAGE(!empty(), "Single: must be initialised before being accessed");
    return *m_value;
  }
};

/// An adaptor to assert when duplicate values are added, or non-existent values removed from a std::set.
template<typename Value>
class UniqueSet
{
  typedef std::set<Value> Values;
  Values m_values;
public:
  typedef typename Values::iterator iterator;
  typedef typename Values::const_iterator const_iterator;
  typedef typename Values::reverse_iterator reverse_iterator;
  typedef typename Values::const_reverse_iterator const_reverse_iterator;


  iterator begin()
  {
    return m_values.begin();
  }
  const_iterator begin() const
  {
    return m_values.begin();
  }
  iterator end()
  {
    return m_values.end();
  }
  const_iterator end() const
  {
    return m_values.end();
  }
  reverse_iterator rbegin()
  {
    return m_values.rbegin();
  }
  const_reverse_iterator rbegin() const
  {
    return m_values.rbegin();
  }
  reverse_iterator rend()
  {
    return m_values.rend();
  }
  const_reverse_iterator rend() const
  {
    return m_values.rend();
  }

  bool empty() const
  {
    return m_values.empty();
  }
  std::size_t size() const
  {
    return m_values.size();
  }
  void clear()
  {
    m_values.clear();
  }

  void swap(UniqueSet& other)
  {
    std::swap(m_values, other.m_values);
  }
  iterator insert(const Value& value)
  {
    std::pair<iterator, bool> result = m_values.insert(value);
    ASSERT_MESSAGE(result.second, "UniqueSet::insert: already added");
    return result.first;
  }
  void erase(const Value& value)
  {
    iterator i = find(value);
    ASSERT_MESSAGE(i != end(), "UniqueSet::erase: not found");
    m_values.erase(i);
  }
  iterator find(const Value& value)
  {
    return std::find(begin(), end(), value);
  }
};

namespace std
{
  /// \brief Swaps the values of \p self and \p other.
  /// Overloads std::swap.
  template<typename Value>
  inline void swap(UniqueSet<Value>& self, UniqueSet<Value>& other)
  {
    self.swap(other);
  }
}

template<typename Type>
class ReferencePair
{
  Type* m_first;
  Type* m_second;
public:
  ReferencePair() : m_first(0), m_second(0)
  {
  }
  void attach(Type& t)
  {
    ASSERT_MESSAGE(m_first == 0 || m_second == 0, "ReferencePair::insert: pointer already exists");
    if(m_first == 0)
    {
      m_first = &t;
    }
    else if(m_second == 0)
    {
      m_second = &t;
    }
  }
  void detach(Type& t)
  {
    ASSERT_MESSAGE(m_first == &t || m_second == &t, "ReferencePair::erase: pointer not found");
    if(m_first == &t)
    {
      m_first = 0;
    }
    else if(m_second == &t)
    {
      m_second = 0;
    }
  }
  template<typename Functor>
  void forEach(const Functor& functor)
  {
    if(m_second != 0)
    {
      functor(*m_second);
    }
    if(m_first != 0)
    {
      functor(*m_first);
    }
  }
};


#endif
