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

#if !defined(INCLUDED_ANGLE_H)
#define INCLUDED_ANGLE_H

#include "ientity.h"

#include "math/quaternion.h"
#include "generic/callback.h"
#include "stringio.h"

const float ANGLEKEY_IDENTITY = 0;

inline void default_angle(float& angle)
{
  angle = ANGLEKEY_IDENTITY;
}
inline void normalise_angle(float& angle)
{
  angle = static_cast<float>(float_mod(angle, 360.0));
}
inline void read_angle(float& angle, const std::string& value)
{
  if(!string_parse_float(value.c_str(), angle))
  {
    angle = 0;
  }
  else
  {
    normalise_angle(angle);
  }
}
inline void write_angle(double angle, Entity* entity)
{
  if(angle == 0)
  {
    entity->setKeyValue("angle", "");
  }
  else
  {
    entity->setKeyValue("angle", doubleToStr(angle));
  }
}

class AngleKey
{
  Callback m_angleChanged;
public:
  float m_angle;


  AngleKey(const Callback& angleChanged)
    : m_angleChanged(angleChanged), m_angle(ANGLEKEY_IDENTITY)
  {
  }

  void angleChanged(const std::string& value)
  {
    read_angle(m_angle, value);
    m_angleChanged();
  }
  typedef MemberCaller1<AngleKey, const std::string&, &AngleKey::angleChanged> AngleChangedCaller;

  void write(Entity* entity) const
  {
    write_angle(m_angle, entity);
  }
};

inline float angle_rotated(float angle, const Quaternion& rotation)
{
  return static_cast<float>(matrix4_get_rotation_euler_xyz_degrees(
    matrix4_multiplied_by_matrix4(
      matrix4_rotation_for_z_degrees(angle),
      matrix4_rotation_for_quaternion_quantised(rotation)
    )
  ).z());
}

#endif
