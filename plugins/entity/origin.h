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

#if !defined(INCLUDED_ORIGIN_H)
#define INCLUDED_ORIGIN_H

#include "ientity.h"

#include "math/matrix.h"
#include "generic/callback.h"
#include "stringio.h"

const Vector3 ORIGINKEY_IDENTITY = Vector3(0, 0, 0);

inline void default_origin(Vector3& origin)
{
  origin = ORIGINKEY_IDENTITY;
}
inline void read_origin(Vector3& origin, const std::string& value)
{
  if(!string_parse_vector3(value.c_str(), origin))
  {
    default_origin(origin);
  }
}
inline void write_origin(const Vector3& origin, Entity* entity, const std::string& key)
{
  char value[64];
  sprintf(value, "%g %g %g", origin[0], origin[1], origin[2]);
  entity->setKeyValue(key, value);
}

inline Vector3 origin_translated(const Vector3& origin, const Vector3& translation)
{
  return matrix4_get_translation_vec3(
    matrix4_multiplied_by_matrix4(
      Matrix4::getTranslation(origin),
      Matrix4::getTranslation(translation)
    )
  );
}

inline Vector3 origin_snapped(const Vector3& origin, float snap)
{
  return vector3_snapped(origin, snap);
}

class OriginKey
{
  Callback m_originChanged;
public:
  Vector3 m_origin;


  OriginKey(const Callback& originChanged)
    : m_originChanged(originChanged), m_origin(ORIGINKEY_IDENTITY)
  {
  }

  void originChanged(const std::string& value)
  {
    read_origin(m_origin, value);
    m_originChanged();
  }
  typedef MemberCaller1<OriginKey, const std::string&, &OriginKey::originChanged> OriginChangedCaller;


  void write(Entity* entity) const
  {
    write_origin(m_origin, entity, "origin");
  }
};

#endif
