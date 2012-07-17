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

#include "math/Quaternion.h"
#include "math/Matrix4.h"
#include "string/convert.h"
#include <boost/function.hpp>

const float ANGLEKEY_IDENTITY = 0;

inline float getNormalisedAngle(float angle)
{
	return float_mod(angle, 360.0f);
}

inline void read_angle(float& angle, const std::string& value)
{
	angle = getNormalisedAngle(string::convert<float>(value, 0));
}

inline void write_angle(double angle, Entity* entity)
{
	entity->setKeyValue("angle", (angle == 0) ? "" : string::to_string(angle));
}

class AngleKey
{
	boost::function<void()> m_angleChanged;
public:
  float m_angle;


  AngleKey(const boost::function<void()>& angleChanged)
    : m_angleChanged(angleChanged), m_angle(ANGLEKEY_IDENTITY)
  {
  }

  void angleChanged(const std::string& value)
  {
    read_angle(m_angle, value);
    m_angleChanged();
  }

  void write(Entity* entity) const
  {
    write_angle(m_angle, entity);
  }
};

inline float angle_rotated(float angle, const Quaternion& rotation)
{
	return static_cast<float>(Matrix4::getRotationAboutZDegrees(angle).getMultipliedBy(
		Matrix4::getRotationQuantised(rotation)
	).getEulerAnglesXYZDegrees().z());
}

#endif
