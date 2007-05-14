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

///\file
///\brief Represents any entity which has a fixed size specified in its entity-definition and does not display a model (e.g. info_player_start).
///
/// This entity displays an axis-aligned bounding box of the size and colour specified in its entity-definition.
/// The "origin" key directly controls the entity's local-to-parent transform.
/// An arrow is drawn to visualise the "angle" key.

#include "generic/GenericEntity.h"
#include "generic/GenericEntityInstance.h"
#include "generic/GenericEntityNode.h"

scene::Node& New_GenericEntity(IEntityClassPtr eclass) {
	return (new entity::GenericEntityNode(eclass))->node();
}
