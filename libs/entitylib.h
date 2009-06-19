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

#if !defined (INCLUDED_ENTITYLIB_H)
#define INCLUDED_ENTITYLIB_H

#include "debugging/debugging.h"

#include "ientity.h"
#include "ieclass.h"
#include "irender.h"
#include "igl.h"
#include "iselectable.h"

#include "generic/callback.h"
#include "math/aabb.h"
#include "undolib.h"
#include "scenelib.h"

#include <list>
#include <set>

/* greebo: draws a pyramid defined by 5 vertices
 * points[0] is the top of the pyramid
 * points[1] to points[4] is the base rectangle
 */
inline void drawPyramid(const Vector3 points[5]) {
  typedef unsigned int index_t;
  index_t indices[16] = {
    0, 1, // top to first
    0, 2, // top to second
    0, 3, // top to third
    0, 4, // top to fourth
    1, 2, // first to second
    2, 3, // second to third
    3, 4, // third to second
    4, 1, // fourth to first 
  };
  glVertexPointer(3, GL_DOUBLE, 0, points);
  glDrawElements(GL_LINES, sizeof(indices)/sizeof(index_t), GL_UNSIGNED_INT, indices);
}

/* greebo: draws a frustum defined by 8 vertices
 * points[0] to points[3] define the top area vertices (clockwise starting from the "upper right" corner)
 * points[4] to points[7] define the base rectangle (clockwise starting from the "upper right" corner)
 */
inline void drawFrustum(const Vector3 points[8]) {
  typedef unsigned int index_t;
  index_t indices[24] = {
  	0, 4, // top up right to bottom up right
  	1, 5, // top down right to bottom down right
  	2, 6, // top down left to bottom down left
  	3, 7, // top up left to bottom up left
  	
  	0, 1, // top up right to top down right
  	1, 2, // top down right to top down left
  	2, 3, // top down left to top up left
  	3, 0, // top up left to top up right
  	
  	4, 5, // bottom up right to bottom down right
  	5, 6, // bottom down right to bottom down left
  	6, 7, // bottom down left to bottom up left
  	7, 4, // bottom up left to bottom up right
  };
  glVertexPointer(3, GL_DOUBLE, 0, points);
  glDrawElements(GL_LINES, sizeof(indices)/sizeof(index_t), GL_UNSIGNED_INT, indices);
}

inline void arrow_draw(const Vector3& origin, const Vector3& direction)
{
  Vector3 up(0, 0, 1);
  Vector3 left(-direction[1], direction[0], 0);

	Vector3 endpoint(origin + direction*32.0);

  Vector3 tip1(endpoint + direction *(-8.0) + up*(-4.0));
	Vector3 tip2(tip1 + up*8.0);
  Vector3 tip3(endpoint + direction*(-8.0) + left*(-4.0));
	Vector3 tip4(tip3 + left*8.0);

  glBegin (GL_LINES);

  glVertex3dv(origin);
  glVertex3dv(endpoint);

  glVertex3dv(endpoint);
  glVertex3dv(tip1);

  glVertex3dv(endpoint);
  glVertex3dv(tip2);

  glVertex3dv(endpoint);
  glVertex3dv(tip3);

  glVertex3dv(endpoint);
  glVertex3dv(tip4);

  glVertex3dv(tip1);
  glVertex3dv(tip3);

  glVertex3dv(tip3);
  glVertex3dv(tip2);

  glVertex3dv(tip2);
  glVertex3dv(tip4);

  glVertex3dv(tip4);
  glVertex3dv(tip1);

  glEnd();
}

class SelectionIntersection;

inline void aabb_testselect(const AABB& aabb, SelectionTest& test, SelectionIntersection& best)
{
  const IndexPointer::index_type indices[24] = {
    2, 1, 5, 6,
    1, 0, 4, 5,
    0, 1, 2, 3,
    3, 7, 4, 0,
    3, 2, 6, 7,
    7, 6, 5, 4,
  };

  Vector3 points[8];
  aabb_corners(aabb, points);
  test.TestQuads(VertexPointer(reinterpret_cast<VertexPointer::pointer>(points), sizeof(Vector3)), IndexPointer(indices, 24), best);
}

inline void aabb_draw_wire(const Vector3 points[8])
{
  typedef unsigned int index_t;
  index_t indices[24] = {
    0, 1, 1, 2, 2, 3, 3, 0,
    4, 5, 5, 6, 6, 7, 7, 4,
    0, 4, 1, 5, 2, 6, 3, 7,
  };
#if 1
  glVertexPointer(3, GL_DOUBLE, 0, points);
  glDrawElements(GL_LINES, sizeof(indices)/sizeof(index_t), GL_UNSIGNED_INT, indices);
#else
  glBegin(GL_LINES);
  for(std::size_t i = 0; i < sizeof(indices)/sizeof(index_t); ++i)
  {
    glVertex3dv(points[indices[i]]);
  }
  glEnd();
#endif
}

inline void aabb_draw_flatshade(const Vector3 points[8])
{
  glBegin(GL_QUADS);

  glNormal3dv(aabb_normals[0]);
  glVertex3dv(points[2]);
  glVertex3dv(points[1]);
  glVertex3dv(points[5]);
  glVertex3dv(points[6]);

  glNormal3dv(aabb_normals[1]);
  glVertex3dv(points[1]);
  glVertex3dv(points[0]);
  glVertex3dv(points[4]);
  glVertex3dv(points[5]);

  glNormal3dv(aabb_normals[2]);
  glVertex3dv(points[0]);
  glVertex3dv(points[1]);
  glVertex3dv(points[2]);
  glVertex3dv(points[3]);

  glNormal3dv(aabb_normals[3]);
  glVertex3dv(points[0]);
  glVertex3dv(points[3]);
  glVertex3dv(points[7]);
  glVertex3dv(points[4]);

  glNormal3dv(aabb_normals[4]);
  glVertex3dv(points[3]);
  glVertex3dv(points[2]);
  glVertex3dv(points[6]);
  glVertex3dv(points[7]);

  glNormal3dv(aabb_normals[5]);
  glVertex3dv(points[7]);
  glVertex3dv(points[6]);
  glVertex3dv(points[5]);
  glVertex3dv(points[4]);

  glEnd();
}

inline void aabb_draw_wire(const AABB& aabb)
{
  Vector3 points[8];
	aabb_corners(aabb, points);
  aabb_draw_wire(points);
}

inline void aabb_draw_flatshade(const AABB& aabb)
{
  Vector3 points[8];
	aabb_corners(aabb, points);
  aabb_draw_flatshade(points);
}

inline void aabb_draw_textured(const AABB& aabb)
{
  Vector3 points[8];
	aabb_corners(aabb, points);

  glBegin(GL_QUADS);

  glNormal3dv(aabb_normals[0]);
  glTexCoord2dv(aabb_texcoord_topleft);
  glVertex3dv(points[2]);
  glTexCoord2dv(aabb_texcoord_topright);
  glVertex3dv(points[1]);
  glTexCoord2dv(aabb_texcoord_botright);
  glVertex3dv(points[5]);
  glTexCoord2dv(aabb_texcoord_botleft);
  glVertex3dv(points[6]);

  glNormal3dv(aabb_normals[1]);
  glTexCoord2dv(aabb_texcoord_topleft);
  glVertex3dv(points[1]);
  glTexCoord2dv(aabb_texcoord_topright);
  glVertex3dv(points[0]);
  glTexCoord2dv(aabb_texcoord_botright);
  glVertex3dv(points[4]);
  glTexCoord2dv(aabb_texcoord_botleft);
  glVertex3dv(points[5]);

  glNormal3dv(aabb_normals[2]);
  glTexCoord2dv(aabb_texcoord_topleft);
  glVertex3dv(points[0]);
  glTexCoord2dv(aabb_texcoord_topright);
  glVertex3dv(points[1]);
  glTexCoord2dv(aabb_texcoord_botright);
  glVertex3dv(points[2]);
  glTexCoord2dv(aabb_texcoord_botleft);
  glVertex3dv(points[3]);

  glNormal3dv(aabb_normals[3]);
  glTexCoord2dv(aabb_texcoord_topleft);
  glVertex3dv(points[0]);
  glTexCoord2dv(aabb_texcoord_topright);
  glVertex3dv(points[3]);
  glTexCoord2dv(aabb_texcoord_botright);
  glVertex3dv(points[7]);
  glTexCoord2dv(aabb_texcoord_botleft);
  glVertex3dv(points[4]);

  glNormal3dv(aabb_normals[4]);
  glTexCoord2dv(aabb_texcoord_topleft);
  glVertex3dv(points[3]);
  glTexCoord2dv(aabb_texcoord_topright);
  glVertex3dv(points[2]);
  glTexCoord2dv(aabb_texcoord_botright);
  glVertex3dv(points[6]);
  glTexCoord2dv(aabb_texcoord_botleft);
  glVertex3dv(points[7]);

  glNormal3dv(aabb_normals[5]);
  glTexCoord2dv(aabb_texcoord_topleft);
  glVertex3dv(points[7]);
  glTexCoord2dv(aabb_texcoord_topright);
  glVertex3dv(points[6]);
  glTexCoord2dv(aabb_texcoord_botright);
  glVertex3dv(points[5]);
  glTexCoord2dv(aabb_texcoord_botleft);
  glVertex3dv(points[4]);

  glEnd();
}

inline void aabb_draw_solid(const AABB& aabb, RenderStateFlags state)
{
  if(state & RENDER_TEXTURE_2D)
  {
    aabb_draw_textured(aabb);
  }
  else
  {
    aabb_draw_flatshade(aabb);
  }
}

inline void aabb_draw(const AABB& aabb, RenderStateFlags state)
{
  if(state & RENDER_FILL)
  {
    aabb_draw_solid(aabb, state);
  }
  else
  {
    aabb_draw_wire(aabb);
  }
}

class RenderableSolidAABB : public OpenGLRenderable
{
  const AABB& m_aabb;
public:
  RenderableSolidAABB(const AABB& aabb) : m_aabb(aabb)
  {
  }
  void render(const RenderInfo& info) const
  {
    aabb_draw_solid(m_aabb, info.getFlags());
  }
};

class RenderableWireframeAABB : public OpenGLRenderable
{
  const AABB& m_aabb;
public:
  RenderableWireframeAABB(const AABB& aabb) : m_aabb(aabb)
  {
  }
  void render(const RenderInfo& info) const
  {
    aabb_draw_wire(m_aabb);
  }
};

/**
 * Stream insertion for Entity objects.
 */
inline std::ostream& operator<< (std::ostream& os, const Entity& entity) {
	os << "Entity { name=\"" << entity.getKeyValue("name") << "\", "
	   << "classname=\"" << entity.getKeyValue("classname") << "\", "
	   << "origin=\"" << entity.getKeyValue("origin") << "\" }";
	
	return os;	
}

class EntityNodeFindByClassnameWalker : 
	public scene::NodeVisitor
{
protected:
	// Name to search for
	std::string _name;
	
	// The search result
	scene::INodePtr _entityNode;
	
public:
	// Constructor
	EntityNodeFindByClassnameWalker(const std::string& name) : 
		_name(name)
	{}
	
	scene::INodePtr getEntityNode() {
		return _entityNode;
	}

	Entity* getEntity() {
		return _entityNode != NULL ? Node_getEntity(_entityNode) : NULL;
	}
	
	// Pre-descent callback
	bool pre(const scene::INodePtr& node) {
		if (_entityNode == NULL) {
			// Entity not found yet
			Entity* entity = Node_getEntity(node);
			
			if (entity != NULL) {
				// Got an entity, let's see if the name matches
				if (entity->getKeyValue("classname") == _name) {
					_entityNode = node;
				}

				return false; // don't traverse entities
			}
			else {
				// Not an entity, traverse
				return true;
			}
		}
		else {
			// Entity already found, don't traverse any further
			return false;
		}
	}
};

/* greebo: Finds an entity with the given classname
 */
inline Entity* Scene_FindEntityByClass(const std::string& className) {
	// Instantiate a walker to find the entity
	EntityNodeFindByClassnameWalker walker(className);
	
	// Walk the scenegraph
	Node_traverseSubgraph(GlobalSceneGraph().root(), walker);
	
	return walker.getEntity();
}

/* Check if a node is the worldspawn.
 */
inline bool node_is_worldspawn(scene::INodePtr node) {
	Entity* entity = Node_getEntity(node);
	return entity != 0 && entity->getKeyValue("classname") == "worldspawn";
}

#endif
