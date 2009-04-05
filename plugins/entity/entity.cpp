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

#include "iregistry.h"
#include "ieclass.h"
#include "ieventmanager.h"
#include "ifilter.h"
#include "selectable.h"
#include "inamespace.h"

#include "scenelib.h"
#include "entitylib.h"
#include "pivot.h"

#include "Doom3Entity.h"
#include "light/LightShader.h"
#include "curve/CurveEditInstance.h"
#include "target/RenderableTargetInstances.h"
#include "EntitySettings.h"

// Initialise the static variables of the entitylibraries (we're in a module here)
EntityCreator::KeyValueChangedFunc entity::Doom3Entity::_keyValueChangedNotify = 0;
EntityCreator::KeyValueChangedFunc entity::KeyValue::_keyValueChangedNotify = 0;

namespace entity {

/* greebo: Constructs the entity environment
 */
void constructStatic() {
	LightShader::m_defaultShader = GlobalRegistry().get("game/defaults/lightShader");

	// Construct Doom3Group stuff
	CurveEditInstance::StaticShaders::instance().controlsShader = GlobalRenderSystem().capture("$POINT");
	CurveEditInstance::StaticShaders::instance().selectedShader = GlobalRenderSystem().capture("$SELPOINT");

	RenderablePivot::StaticShader::instance() = GlobalRenderSystem().capture("$PIVOT");

	GlobalRenderSystem().attachRenderable(RenderableTargetInstances::Instance());

	GlobalEventManager().addRegistryToggle("ToggleShowAllLightRadii", RKEY_SHOW_ALL_LIGHT_RADII);
	GlobalEventManager().addRegistryToggle("ToggleShowAllSpeakerRadii", RKEY_SHOW_ALL_SPEAKER_RADII);
}

void destroyStatic() {
	GlobalRenderSystem().detachRenderable(RenderableTargetInstances::Instance());

	// Destroy the settings instance
	EntitySettings::destroy();
}

} // namespace entity
