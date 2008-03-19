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

#if !defined(INCLUDED_RENDERER_H)
#define INCLUDED_RENDERER_H

#include "render/frontend/ForEachVisible.h"

#include "irender.h"
#include "ieclass.h"
#include "irenderable.h"
#include "iselection.h"
#include "cullable.h"
#include "scenelib.h"
#include "math/frustum.h"

inline RenderablePtr Node_getRenderable(const scene::INodePtr& node) {
	return boost::dynamic_pointer_cast<Renderable>(node);
}

/**
 * Test the visibility of the given Node by intersecting with the given
 * VolumeTest, subject to the visibility of the parent. If the parent's
 * intersection is partial, this node is tested, otherwise the parent's
 * visibility is used instead.
 */
inline 
VolumeIntersectionValue Cullable_testVisible(const scene::INodePtr& node, 
											 const VolumeTest& volume, 
											 VolumeIntersectionValue parent)
{
	// Check for partial intersection with the parent
	if(parent == c_volumePartial) {
		
		// Parent has partial visibility, so test this Instance and return
		// its result
		CullablePtr cullable = boost::dynamic_pointer_cast<Cullable>(node);
		if (cullable != NULL) {
			return cullable->intersectVolume(volume, node->localToWorld());
		}
	}
	
	// Parent was not partially visible or this instance is not Cullable, so
	// pass back the parent's visibility
	return parent;
}

#include "render/frontend/CullingWalker.h"

/**
 * Enumeration function for visible objects in the scene. Calls the Functor
 * object on each visible object after performing a volume intersection test
 * with the supplied VolumeTest.
 */
template<typename Functor>
inline void Scene_forEachVisible(scene::Graph& graph, 
								 const VolumeTest& volume, 
								 const Functor& functor)
{
	graph.traverse(
		ForEachVisible< CullingWalker<Functor> >(
			volume, 
			CullingWalker<Functor>(volume, functor)));
}

#include "render/frontend/RenderHighlighted.h"

/**
 * Scene render function. Uses the visibility walkers to traverse the scene
 * graph and submit all visible objects to the provided Renderer.
 */
inline void Scene_Render(Renderer& renderer, const VolumeTest& volume)
{
	// Submit renderables from scene graph
	GlobalSceneGraph().traverse(
		ForEachVisible<RenderHighlighted>(volume, 
										  RenderHighlighted(renderer, volume)));
	
	// Submit renderables directly attached to the ShaderCache
	GlobalShaderCache().forEachRenderable(
		RenderHighlighted::RenderCaller(RenderHighlighted(renderer, volume)));
}

#endif
