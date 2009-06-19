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

#if !defined(INCLUDED_GENERIC_STATIC_H)
#define INCLUDED_GENERIC_STATIC_H

/// \file
/// \brief Template techniques for instantiating singletons.

#include <cstddef>

class Null
{
};

/// \brief A singleton which is statically initialised.
///
/// \param Type The singleton object type.
/// \param Type The type distinguishing this instance from others of the same type.
///
/// \dontinclude generic/static.cpp
/// \skipline Static example
/// \until end example
template<typename Type, typename Context = Null>
class Static
{
  static Type m_instance;
public:
  static Type& instance()
  {
    return m_instance;
  }
};

template<typename Type, typename Context>
Type Static<Type, Context>::m_instance;

#endif
