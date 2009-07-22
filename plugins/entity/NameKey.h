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

#if !defined(INCLUDED_NAMEDENTITY_H)
#define INCLUDED_NAMEDENTITY_H

#include "entitylib.h"
#include "generic/callback.h"
#include "nameable.h"
#include "Doom3Entity.h"

namespace entity {

class NameKey
{
	// The reference to the spawnarg structure
	Doom3Entity& m_entity;

	// Cached "name" keyvalue
	std::string _name;

public:
	NameKey(Doom3Entity& entity) : 
		m_entity(entity)
	{}

	std::string name() const
	{
		if (_name.empty()) {
			return m_entity.getEntityClass()->getName();
		}
		return _name;
	}

	void nameChanged(const std::string& value)
	{
		_name = value;
	}
	typedef MemberCaller1<NameKey, const std::string&, &NameKey::nameChanged> NameChangedCaller;
};

class RenderableNameKey : 
	public OpenGLRenderable
{
	const NameKey& _nameKey;

	// The transformation matrix to be passed to the renderer
	// for some Entity types (D3Group), this can be non-identity
	Matrix4 _localToParent;

	// The "working set" of the matrix
	Matrix4 _localToParentTransformed;

	// Always 0,0,0
	Vector3 _origin;
	
public:
	RenderableNameKey(const NameKey& nameKey) : 
		_nameKey(nameKey), 
		_localToParent(Matrix4::getIdentity()),
		_localToParentTransformed(_localToParent),
		_origin(0,0,0)
	{}

	void render(const RenderInfo& info) const
	{
		glRasterPos3dv(_origin);
		GlobalOpenGL().drawString(_nameKey.name());
	}

	const Matrix4& getLocalToParent() const
	{
		return _localToParentTransformed;
	}

	void setLocalToParent(const Matrix4& localToParent)
	{
		_localToParent = localToParent;
	}

	void translate(const Vector3& translation)
	{
		_localToParentTransformed.translateBy(translation);
	}

	void revertTransform()
	{
		_localToParentTransformed = _localToParent;
	}

	void freezeTransform()
	{
		_localToParent = _localToParentTransformed;
	}
};

} // namespace entity

#endif
