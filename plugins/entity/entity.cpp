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

#include "entity.h"

#include "ieclass.h"
#include "ifilter.h"
#include "selectable.h"
#include "inamespace.h"

#include "scenelib.h"
#include "entitylib.h"
#include "pivot.h"

#include "targetable.h"
#include "uniquenames.h"
#include "namekeys.h"
#include "stream/stringstream.h"

#include "doom3group/Doom3GroupNode.h"
#include "light/LightShader.h"

// Initialise the static variables of the entitylibraries (we're in a module here)
EntityCreator::KeyValueChangedFunc entity::Doom3Entity::m_entityKeyValueChanged = 0;
EntityCreator::KeyValueChangedFunc entity::KeyValue::m_entityKeyValueChanged = 0;

#include "preferencesystem.h"

namespace entity {

/* greebo: Constructs the entity environment
 */
void constructStatic() {
	LightShader::m_defaultShader = "lights/defaultpointlight";

	// Construct Doom3Group stuff
	CurveEdit::Type::instance().m_controlsShader = GlobalShaderCache().capture("$POINT");
	CurveEdit::Type::instance().m_selectedShader = GlobalShaderCache().capture("$SELPOINT");

	RenderablePivot::StaticShader::instance() = GlobalShaderCache().capture("$PIVOT");

	GlobalShaderCache().attachRenderable(StaticRenderableConnectionLines::instance());
}

void destroyStatic() {
	GlobalShaderCache().detachRenderable(StaticRenderableConnectionLines::instance());
}

} // namespace entity
