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
///\brief Represents any Doom3 entity which does not have a fixed size specified in its entity-definition (e.g. func_static).
///
/// This entity behaves as a group only when the "model" key is empty or is the same as the "name" key. Otherwise it behaves as a model.
/// When behaving as a group, the "origin" key is the translation to be applied to all brushes (not patches) grouped under this entity.
/// When behaving as a model, the "origin", "angle" and "rotation" keys directly control the entity's local-to-parent transform.
/// When either the "curve_Nurbs" or "curve_CatmullRomSpline" keys define a curve, the curve is rendered and can be edited.

#include "doom3group.h"

#include "doom3group/Doom3GroupInstance.h"
#include "doom3group/Doom3GroupNode.h"

void Doom3Group_construct() {
	CurveEdit::Type::instance().m_controlsShader = GlobalShaderCache().capture("$POINT");
	CurveEdit::Type::instance().m_selectedShader = GlobalShaderCache().capture("$SELPOINT");
}

void Doom3Group_destroy() {

}

scene::Node& New_Doom3Group(IEntityClassPtr eclass) {
	return (new entity::Doom3GroupNode(eclass))->node();
}
