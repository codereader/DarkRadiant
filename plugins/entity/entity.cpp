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
#include "namespace.h"

#include "scenelib.h"
#include "entitylib.h"
#include "pivot.h"

#include "targetable.h"
#include "uniquenames.h"
#include "namekeys.h"
#include "stream/stringstream.h"

#include "light.h"
#include "eclassmodel.h"
#include "generic.h"
#include "doom3group.h"

// Initialise the static variables of the entitylibraries (we're in a module here)
EntityCreator::KeyValueChangedFunc EntityKeyValues::m_entityKeyValueChanged = 0;
EntityCreator::KeyValueChangedFunc KeyValue::m_entityKeyValueChanged = 0;
Counter* EntityKeyValues::m_counter = 0;

#include "preferencesystem.h"

/* greebo: Constructs the entity environment according to the given game type (is Doom3 anyway)
 */
void Entity_Construct() {
  Light_Construct(LIGHTTYPE_DOOM3);
  Doom3Group_construct();

  RenderablePivot::StaticShader::instance() = GlobalShaderCache().capture("$PIVOT");

  GlobalShaderCache().attachRenderable(StaticRenderableConnectionLines::instance());
}

void Entity_Destroy() {
  GlobalShaderCache().detachRenderable(StaticRenderableConnectionLines::instance());

  Doom3Group_destroy();
  Light_Destroy();
}
