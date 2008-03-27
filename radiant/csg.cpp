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

#include "csg.h"

#include "debugging/debugging.h"

#include <list>
#include <map>

#include "ibrush.h"
#include "math/Plane3.h"
#include "brushmanip.h"
#include "brush/BrushVisit.h"
#include "brush/BrushNode.h"
#include "shaderlib.h"
#include "igrid.h"

inline bool Face_testPlane(const Face& face, const Plane3& plane, bool flipped)
{
	return face.contributes() && !face.getWinding().testPlane(plane, flipped);
}

BrushSplitType Brush_classifyPlane(const Brush& brush, const Plane3& plane)
{
  brush.evaluateBRep();
  BrushSplitType split;
  for(Brush::const_iterator i(brush.begin()); i != brush.end(); ++i)
  {
    if((*i)->contributes())
    {
      split += (*i)->getWinding().classifyPlane(plane);
    }
  }
  return split;
}

class BrushSplitByPlaneSelected : 
	public scene::Graph::Walker
{
	const Vector3& m_p0;
	const Vector3& m_p1;
	const Vector3& m_p2;
	std::string m_shader;
	TextureProjection m_projection;
	EBrushSplit m_split;

public:
	BrushSplitByPlaneSelected(const Vector3& p0, const Vector3& p1, const Vector3& p2, 
							  const std::string& shader, 
							  const TextureProjection& projection, EBrushSplit split) : 
		m_p0(p0), 
		m_p1(p1), 
		m_p2(p2), 
		m_shader(shader), 
		m_projection(projection), 
		m_split(split)
	{}

	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		return true;
	}

	void post(const scene::Path& path, const scene::INodePtr& node) const {
		// Don't clip invisible nodes
		if (!node->visible()) {
			return;
		}

		// Try to cast the instance onto a brush
		Brush* brush = Node_getBrush(node);

		// Return if not brush or not selected
		if (brush == NULL || !Node_getSelectable(node)->isSelected()) {
			return;
		}

		Plane3 plane(m_p0, m_p1, m_p2);
		if (!plane.isValid()) {
			return;
		}

		std::map<std::string, int> shaderCount;
		std::string mostUsedShader("");
		int mostUsedShaderCount(0);
		TextureProjection mostUsedTextureProjection;

		// greebo: Get the most used shader of this brush
		for (Brush::const_iterator i = brush->begin(); i != brush->end(); i++) {
			// Get the shadername
			const std::string& shader = (*i)->GetShader();

			// Insert counter, if necessary
			if (shaderCount.find(shader) == shaderCount.end()) {
				shaderCount[shader] = 0;
			}

			// Increase the counter
			shaderCount[shader]++;

			if (shaderCount[shader] > mostUsedShaderCount) {
				mostUsedShader = shader;
				mostUsedShaderCount = shaderCount[shader];

				// Copy the TexDef from the face into the local member
				(*i)->GetTexdef(mostUsedTextureProjection);
			}
		}

		// Fall back to the default shader, if nothing found
		if (mostUsedShader.empty() || mostUsedShaderCount == 1) {
			mostUsedShader = m_shader;
			mostUsedTextureProjection = m_projection;
		}

		BrushSplitType split = Brush_classifyPlane(*brush, m_split == eFront ? -plane : plane);

		if (split.counts[ePlaneBack] && split.counts[ePlaneFront]) {
			// the plane intersects this brush
			if (m_split == eFrontAndBack) {
				scene::INodePtr node = GlobalBrushCreator().createBrush();

				Brush* fragment = Node_getBrush(node);
				fragment->copy(*brush);

				FacePtr newFace = fragment->addPlane(m_p0, m_p1, m_p2, mostUsedShader, mostUsedTextureProjection);

				if (newFace != NULL && m_split != eFront) {
					newFace->flipWinding();
				}

				fragment->removeEmptyFaces();
				ASSERT_MESSAGE(!fragment->empty(), "brush left with no faces after split");

				path.parent()->addChildNode(node);
				Node_setSelected(node, true);
			}

			FacePtr newFace = brush->addPlane(m_p0, m_p1, m_p2, mostUsedShader, mostUsedTextureProjection);

			if (newFace != NULL && m_split == eFront) {
				newFace->flipWinding();
			}

			brush->removeEmptyFaces();
			ASSERT_MESSAGE(!brush->empty(), "brush left with no faces after split");
		}
		// the plane does not intersect this brush
		else if (m_split != eFrontAndBack && split.counts[ePlaneBack] != 0) {
			// the brush is "behind" the plane
			Path_deleteTop(path);
		}
	}
};

void Scene_BrushSplitByPlane(const Vector3 planePoints[3], 
							 const std::string& shader, 
							 EBrushSplit split)
{ 
	TextureProjection projection;
	projection.constructDefault();

	GlobalSceneGraph().traverse(
		BrushSplitByPlaneSelected(planePoints[0], 
								  planePoints[1], 
								  planePoints[2], 
								  shader, projection, split)
	);

	SceneChangeNotify();
}

class BrushSetClipPlane : 
	public scene::Graph::Walker
{
	Plane3 _plane;
public:
	BrushSetClipPlane(const Plane3& plane) : 
		_plane(plane)
	{}

	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		BrushNodePtr brush = boost::dynamic_pointer_cast<BrushNode>(node);

		if (brush != NULL && node->visible() && brush->isSelected()) {
			brush->setClipPlane(_plane);
		}
		return true; 
	}
};

void Scene_BrushSetClipPlane(const Plane3& plane) {
	GlobalSceneGraph().traverse(BrushSetClipPlane(plane));
}
