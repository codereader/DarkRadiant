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
///\brief Represents any light entity (e.g. light).
///
/// This entity dislays a special 'light' model.
/// The "origin" key directly controls the position of the light model in local space.
/// The "_color" key controls the colour of the light model.
/// The "light" key is visualised with a sphere representing the approximate coverage of the light (except Doom3).
/// Doom3 special behaviour:
/// The entity behaves as a group.
/// The "origin" key is the translation to be applied to all brushes (not patches) grouped under this entity.
/// The "light_center" and "light_radius" keys are visualised with a point and a box when the light is selected.
/// The "rotation" key directly controls the orientation of the light bounding box in local space.
/// The "light_origin" key controls the position of the light independently of the "origin" key if it is specified.
/// The "light_rotation" key duplicates the behaviour of the "rotation" key if it is specified. This appears to be an unfinished feature in Doom3.

#include "light.h"
#include "light/RenderableLightCentre.h"
#include "light/Renderables.h"
#include "light/Doom3LightRadius.h"
#include "light/LightShader.h"
#include "light/Light.h"	// contains the Light class
#include "light/LightInstance.h"
#include "light/LightNode.h"

void Light_Construct(LightType lightType) {
	g_lightType = lightType;
	LightShader::m_defaultShader = "lights/defaultPointLight";  
}

void Light_Destroy() {
	// nothing to destroy here
}

scene::Node& New_Light(IEntityClass* eclass) {
	return (new LightNode(eclass))->node();
}
