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

#if !defined(INCLUDED_TARGETABLE_H)
#define INCLUDED_TARGETABLE_H

#include <set>
#include <map>

#include "cullable.h"
#include "renderable.h"

#include "math/line.h"
#include "render.h"
#include "generic/callback.h"
#include "selectionlib.h"
#include "entitylib.h"
#include "stringio.h"
#include "Doom3Entity.h"

#include "target/TargetableInstance.h"

/**
 * greebo: This is a "container" for all TargetableInstances. These register
 *         themselves at construction time and will get invoked during
 *         the frontend render pass.
 *
 * This object is also a Renderable which is always attached to the GlobalShaderCache()
 * during the entire module lifetime.
 */
class RenderableConnectionLines : 
	public Renderable
{
	typedef std::set<entity::TargetableInstance*> TargetableInstances;
	TargetableInstances m_instances;
public:
	// Add a TargetableInstance to this set
	void attach(entity::TargetableInstance& instance) {
		ASSERT_MESSAGE(m_instances.find(&instance) == m_instances.end(), "cannot attach instance");
		m_instances.insert(&instance);
	}

	void detach(entity::TargetableInstance& instance) {
		ASSERT_MESSAGE(m_instances.find(&instance) != m_instances.end(), "cannot detach instance");
		m_instances.erase(&instance);
	}

	void renderSolid(Renderer& renderer, const VolumeTest& volume) const {
		for (TargetableInstances::const_iterator i = m_instances.begin(); i != m_instances.end(); ++i) {
			if((*i)->path().top()->visible())
			{
				(*i)->render(renderer, volume);
			}
		}
	}

	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
		renderSolid(renderer, volume);
	}
};

typedef Static<RenderableConnectionLines> StaticRenderableConnectionLines;

#endif
