/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

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

#if !defined(INCLUDED_ENTITY_H)
#define INCLUDED_ENTITY_H

#include "icommandsystem.h"
#include "scenelib.h"

// CONSTANTS

// Default radius of lights is 320 (Q4) rather than 300 (D3)
// since the grid is usually a multiple of 8.

const float DEFAULT_LIGHT_RADIUS = 320;

//

template<typename Element> class BasicVector3;
typedef BasicVector3<double> Vector3;

/** Exception thrown when the incorrect number of brushes is selected when
 * creating an entity.
 */
 
class EntityCreationException
: public std::runtime_error
{
public:
	EntityCreationException(const std::string& what)
	: std::runtime_error(what) {}
};


/** Create an instance of the given entity at the given position, and return
 * the Node containing the new entity. If the incorrect number of brushes
 * is selected, an EntityCreationException will be thrown.
 * 
 * @returns
 * A NodeSmartReference containing the new entity.
 */

scene::INodePtr Entity_createFromSelection(const char* name, const Vector3& origin);

void Scene_EntitySetKeyValue_Selected(const char* key, const char* value);

/**
 * greebo: Changes the classname of the given instance. This is a nontrivial
 *         operation, as the Entity needs to be created afresh and all the
 *         spawnargs need to be copied over. Child primitives need to be 
 *         considered as well.
 * 
 * @node: The entity node to change the classname of.
 * @classname: The new classname
 * 
 * @returns: The new entity node.
 */
scene::INodePtr changeEntityClassname(const scene::INodePtr& node, const std::string& classname);

void Entity_Construct();
void Entity_Destroy();

// Triggers a SkinCache refresh
void ReloadSkins(const cmd::ArgumentList& args);

#endif
