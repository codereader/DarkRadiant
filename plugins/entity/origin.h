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
inline void read_origin(Vector3& origin, const char* value)
{
  if(!string_parse_vector3(value, origin))
  {
    default_origin(origin);
  }
}
inline void write_origin(const Vector3& origin, Entity* entity, const char* key)
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

  void originChanged(const char* value)
  {
    read_origin(m_origin, value);
    m_originChanged();
  }
  typedef MemberCaller1<OriginKey, const char*, &OriginKey::originChanged> OriginChangedCaller;


  void write(Entity* entity) const
  {
    write_origin(m_origin, entity, "origin");
  }
};


#include "scenelib.h"

/** greebo: This checks for primitive nodes of types BrushDoom3/PatchDoom3 and passes 
 * the new origin to them so they can take the according actions like
 * TexDef translation in case of an active texture lock, etc.
 */
inline void Primitives_setDoom3GroupOrigin(scene::Node& node, const Vector3& origin) {
	// Check for BrushDoom3
	BrushDoom3* brush = Node_getBrushDoom3(node);
	if (brush != NULL) {
		brush->setDoom3GroupOrigin(origin);
	}
}

class SetDoom3GroupOriginWalker : public scene::Traversable::Walker
{
  const Vector3& m_origin;
public:
  SetDoom3GroupOriginWalker(const Vector3& origin) : m_origin(origin)
  {
  }
  bool pre(scene::Node& node) const
  {
    Primitives_setDoom3GroupOrigin(node, m_origin);
    return true;
  }
};

class Doom3GroupOrigin : public scene::Traversable::Observer
{
  scene::Traversable& m_set;
  const Vector3& m_origin;
  bool m_enabled;

public:
  Doom3GroupOrigin(scene::Traversable& set, const Vector3& origin) : m_set(set), m_origin(origin), m_enabled(false)
  {
  }

  void enable(bool triggerOriginChange = true)
  {
    m_enabled = true;
    if (triggerOriginChange) {
    	originChanged();
    }
  }
  void disable()
  {
    m_enabled = false;
  }

  void originChanged()
  {
    if(m_enabled)
    {
      m_set.traverse(SetDoom3GroupOriginWalker(m_origin));
    }
  }

  void insert(scene::Node& node)
  {
    if(m_enabled)
    {
      Primitives_setDoom3GroupOrigin(node, m_origin);
    }
  }
  void erase(scene::Node& node)
  {
    if(m_enabled)
    {
      Primitives_setDoom3GroupOrigin(node, Vector3(0, 0, 0));
    }
  }
};


#endif
