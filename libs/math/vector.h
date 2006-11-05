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

#if !defined(INCLUDED_MATH_VECTOR_H)
#define INCLUDED_MATH_VECTOR_H

/// \file
/// \brief Vector data types and related operations.

#include "generic/vector.h"

#include <iostream>

#if defined (_MSC_VER)

inline int lrint (double flt)
{
  int i;

	_asm
	{
    fld flt
		fistp i
  };
			
	return i;
} 

#elif defined(__FreeBSD__)

inline int lrint(double f)
{
  return static_cast<int>(f + 0.5);
}

#elif defined(__GNUC__)

 // lrint is part of ISO C99
#define	_ISOC9X_SOURCE	1
#define _ISOC99_SOURCE	1

#define	__USE_ISOC9X	1
#define	__USE_ISOC99	1

#else
#error "unsupported platform"
#endif

#include <cmath>
#include <float.h>
#include <algorithm>


//#include "debugging/debugging.h"

/// \brief Returns true if \p self is equal to other \p other within \p epsilon.
template<typename Element, typename OtherElement>
inline bool float_equal_epsilon(const Element& self, const OtherElement& other, const Element& epsilon)
{
  return fabs(other - self) < epsilon;
}

/// \brief Returns the value midway between \p self and \p other.
template<typename Element>
inline Element float_mid(const Element& self, const Element& other)
{
  return Element((self + other) * 0.5);
}

/// \brief Returns \p f rounded to the nearest integer. Note that this is not the same behaviour as casting from float to int.
template<typename Element>
inline int float_to_integer(const Element& f)
{
  return lrint(f);
}

/// \brief Returns \p f rounded to the nearest multiple of \p snap.
template<typename Element, typename OtherElement>
inline Element float_snapped(const Element& f, const OtherElement& snap)
{
  return Element(float_to_integer(f / snap) * snap);
}

/// \brief Returns true if \p f has no decimal fraction part.
template<typename Element>
inline bool float_is_integer(const Element& f)
{
  return f == Element(float_to_integer(f));
}

/// \brief Returns \p self modulated by the range [0, \p modulus)
/// \p self must be in the range [\p -modulus, \p modulus)
template<typename Element, typename ModulusElement>
inline Element float_mod_range(const Element& self, const ModulusElement& modulus)
{
  return Element((self < 0.0) ? self + modulus : self);
}

/// \brief Returns \p self modulated by the range [0, \p modulus)
template<typename Element, typename ModulusElement>
inline Element float_mod(const Element& self, const ModulusElement& modulus)
{
  return float_mod_range(Element(fmod(static_cast<double>(self), static_cast<double>(modulus))), modulus);
}

// =============== Vector 4 Methods ======================================================


template<typename Element, typename OtherElement>
inline bool vector4_equal(const BasicVector4<Element>& self, const BasicVector4<OtherElement>& other)
{
  return self.x() == other.x() && self.y() == other.y() && self.z() == other.z() && self.w() == other.w();
}
template<typename Element, typename OtherElement>
inline bool operator==(const BasicVector4<Element>& self, const BasicVector4<OtherElement>& other)
{
  return vector4_equal(self, other);
}
template<typename Element, typename OtherElement>
inline bool operator!=(const BasicVector4<Element>& self, const BasicVector4<OtherElement>& other)
{
  return !vector4_equal(self, other);
}

template<typename Element, typename OtherElement>
inline bool vector4_equal_epsilon(const BasicVector4<Element>& self, const BasicVector4<OtherElement>& other, Element epsilon)
{
  return float_equal_epsilon(self.x(), other.x(), epsilon)
    && float_equal_epsilon(self.y(), other.y(), epsilon)
    && float_equal_epsilon(self.z(), other.z(), epsilon)
    && float_equal_epsilon(self.w(), other.w(), epsilon);
}

template<typename Element, typename OtherElement>
inline BasicVector4<Element> vector4_added(const BasicVector4<Element>& self, const BasicVector4<OtherElement>& other)
{
  return BasicVector4<Element>(
    float(self.x() + other.x()),
    float(self.y() + other.y()),
    float(self.z() + other.z()),
    float(self.w() + other.w())
  );
}
template<typename Element, typename OtherElement>
inline BasicVector4<Element> operator+(const BasicVector4<Element>& self, const BasicVector4<OtherElement>& other)
{
  return vector4_added(self, other);
}
template<typename Element, typename OtherElement>
inline void vector4_add(BasicVector4<Element>& self, const BasicVector4<OtherElement>& other)
{
  self.x() += static_cast<float>(other.x());
  self.y() += static_cast<float>(other.y());
  self.z() += static_cast<float>(other.z());
  self.w() += static_cast<float>(other.w());
}
template<typename Element, typename OtherElement>
inline void operator+=(BasicVector4<Element>& self, const BasicVector4<OtherElement>& other)
{
  vector4_add(self, other);
}

template<typename Element, typename OtherElement>
inline BasicVector4<Element> vector4_subtracted(const BasicVector4<Element>& self, const BasicVector4<OtherElement>& other)
{
  return BasicVector4<Element>(
    float(self.x() - other.x()),
    float(self.y() - other.y()),
    float(self.z() - other.z()),
    float(self.w() - other.w())
  );
}
template<typename Element, typename OtherElement>
inline BasicVector4<Element> operator-(const BasicVector4<Element>& self, const BasicVector4<OtherElement>& other)
{
  return vector4_subtracted(self, other);
}
template<typename Element, typename OtherElement>
inline void vector4_subtract(BasicVector4<Element>& self, const BasicVector4<OtherElement>& other)
{
  self.x() -= static_cast<float>(other.x());
  self.y() -= static_cast<float>(other.y());
  self.z() -= static_cast<float>(other.z());
  self.w() -= static_cast<float>(other.w());
}
template<typename Element, typename OtherElement>
inline void operator-=(BasicVector4<Element>& self, const BasicVector4<OtherElement>& other)
{
  vector4_subtract(self, other);
}

template<typename Element, typename OtherElement>
inline BasicVector4<Element> vector4_scaled(const BasicVector4<Element>& self, const BasicVector4<OtherElement>& other)
{
  return BasicVector4<Element>(
    float(self.x() * other.x()),
    float(self.y() * other.y()),
    float(self.z() * other.z()),
    float(self.w() * other.w())
  );
}
template<typename Element, typename OtherElement>
inline BasicVector4<Element> operator*(const BasicVector4<Element>& self, const BasicVector4<OtherElement>& other)
{
  return vector4_scaled(self, other);
}
template<typename Element, typename OtherElement>
inline void vector4_scale(BasicVector4<Element>& self, const BasicVector4<OtherElement>& other)
{
  self.x() *= static_cast<float>(other.x());
  self.y() *= static_cast<float>(other.y());
  self.z() *= static_cast<float>(other.z());
  self.w() *= static_cast<float>(other.w());
}
template<typename Element, typename OtherElement>
inline void operator*=(BasicVector4<Element>& self, const BasicVector4<OtherElement>& other)
{
  vector4_scale(self, other);
}

template<typename Element, typename OtherElement>
inline BasicVector4<Element> vector4_scaled(const BasicVector4<Element>& self, OtherElement scale)
{
  return BasicVector4<Element>(
    float(self.x() * scale),
    float(self.y() * scale),
    float(self.z() * scale),
    float(self.w() * scale)
  );
}
template<typename Element, typename OtherElement>
inline BasicVector4<Element> operator*(const BasicVector4<Element>& self, OtherElement scale)
{
  return vector4_scaled(self, scale);
}
template<typename Element, typename OtherElement>
inline void vector4_scale(BasicVector4<Element>& self, OtherElement scale)
{
  self.x() *= static_cast<float>(scale);
  self.y() *= static_cast<float>(scale);
  self.z() *= static_cast<float>(scale);
  self.w() *= static_cast<float>(scale);
}
template<typename Element, typename OtherElement>
inline void operator*=(BasicVector4<Element>& self, OtherElement scale)
{
  vector4_scale(self, scale);
}

template<typename Element, typename OtherElement>
inline BasicVector4<Element> vector4_divided(const BasicVector4<Element>& self, OtherElement divisor)
{
  return BasicVector4<Element>(
    float(self.x() / divisor),
    float(self.y() / divisor),
    float(self.z() / divisor),
    float(self.w() / divisor)
  );
}
template<typename Element, typename OtherElement>
inline BasicVector4<Element> operator/(const BasicVector4<Element>& self, OtherElement divisor)
{
  return vector4_divided(self, divisor);
}
template<typename Element, typename OtherElement>
inline void vector4_divide(BasicVector4<Element>& self, OtherElement divisor)
{
  self.x() /= divisor;
  self.y() /= divisor;
  self.z() /= divisor;
  self.w() /= divisor;
}
template<typename Element, typename OtherElement>
inline void operator/=(BasicVector4<Element>& self, OtherElement divisor)
{
  vector4_divide(self, divisor);
}

template<typename Element, typename OtherElement>
inline double vector4_dot(const BasicVector4<Element>& self, const BasicVector4<OtherElement>& other)
{
  return self.x() * other.x() + self.y() * other.y() + self.z() * other.z() + self.w() * other.w();
}

template<typename Element>
inline BasicVector3<Element> vector4_projected(const BasicVector4<Element>& self)
{
  return vector4_to_vector3(self) * (1.0 / self[3]);
}

#endif
