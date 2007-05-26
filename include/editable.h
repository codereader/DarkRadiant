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

#if !defined(INCLUDED_EDITABLE_H)
#define INCLUDED_EDITABLE_H

template<typename Element> class BasicVector3;
typedef BasicVector3<double> Vector3;
template<typename Element> class BasicVector4;
typedef BasicVector4<double> Vector4;
class Matrix4;
typedef Vector4 Quaternion;

#include "scenelib.h"

class Editable
{
public:
  STRING_CONSTANT(Name, "Editable");

  virtual const Matrix4& getLocalPivot() const = 0;
};
typedef boost::shared_ptr<Editable> EditablePtr;

inline EditablePtr Node_getEditable(scene::INodePtr node) {
	return boost::dynamic_pointer_cast<Editable>(node);
}

class Snappable
{
public:
  STRING_CONSTANT(Name, "Snappable");

  virtual void snapto(float snap) = 0;
};
typedef boost::shared_ptr<Snappable> SnappablePtr;

inline SnappablePtr Node_getSnappable(scene::INodePtr node) {
	return boost::dynamic_pointer_cast<Snappable>(node);
}

class ComponentEditable {
public:
	STRING_CONSTANT(Name, "ComponentEditable");

	virtual const AABB& getSelectedComponentsBounds() const = 0;
};
typedef boost::shared_ptr<ComponentEditable> ComponentEditablePtr;

inline ComponentEditable* Instance_getComponentEditable(scene::Instance& instance) {
	return dynamic_cast<ComponentEditable*>(&instance);
}

class ComponentSnappable {
public:
	STRING_CONSTANT(Name, "ComponentSnappable");

	virtual void snapComponents(float snap) = 0;
};
typedef boost::shared_ptr<ComponentSnappable> ComponentSnappablePtr;

inline ComponentSnappable* Instance_getComponentSnappable(scene::Instance& instance) {
	return dynamic_cast<ComponentSnappable*>(&instance);
}


#endif
