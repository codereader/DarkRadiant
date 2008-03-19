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

void Face_makeBrush(Face& face, const Brush& brush, BrushVector& out, float offset, bool makeRoom)
{
  if(face.contributes())
  {
    out.push_back(new Brush(brush));
    FacePtr newFace = out.back()->addFace(face);
    if(newFace != 0)
    {
      newFace->flipWinding();
      newFace->getPlane().offset(offset);
      newFace->planeChanged();
      
		if (makeRoom) {
			// Retrieve the normal vector of the "source" face
			out.back()->transform(
				Matrix4::getTranslation(face.getPlane().plane3().normal()*offset)
			);
			out.back()->freezeTransform();
		}
    }
  }
}

class FaceMakeBrush
{
  const Brush& brush;
  BrushVector& out;
  float offset;
  bool _makeRoom;
public:
  FaceMakeBrush(const Brush& brush, BrushVector& out, float offset, bool makeRoom = false)
    : brush(brush), out(out), offset(offset), _makeRoom(makeRoom)
  {
  }
  void operator()(Face& face) const
  {
    Face_makeBrush(face, brush, out, offset, _makeRoom);
  }
};

void Brush_makeHollow(const Brush& brush, BrushVector& out, float offset, bool makeRoom)
{
  Brush_forEachFace(brush, FaceMakeBrush(brush, out, offset, makeRoom));
}

class BrushHollowSelectedWalker : public scene::Graph::Walker
{
  float m_offset;
  bool _makeRoom;
public:
	/** greebo: Hollows each visited selected brush
	 * 
	 * @makeRoom: set this to true if the brushes should be moved towards the outside
	 * 			  so that the overlapping corners are resolved (works only for 4sided brushes). 
	 */
  BrushHollowSelectedWalker(float offset, bool makeRoom = false)
    : m_offset(offset),
    	_makeRoom(makeRoom)
  {
  }
  bool pre(const scene::Path& path, const scene::INodePtr& node) const
  {
    if(path.top()->visible())
    {
      Brush* brush = Node_getBrush(node);
      if(brush != NULL && Node_getSelectable(node)->isSelected() && path.size() > 1)
      {
        BrushVector out;
        Brush_makeHollow(*brush, out, m_offset, _makeRoom);
        for(BrushVector::const_iterator i = out.begin(); i != out.end(); ++i)
        {
          (*i)->removeEmptyFaces();
          scene::INodePtr node = GlobalBrushCreator().createBrush();
          Node_getBrush(node)->copy(*(*i));
          delete (*i);
          path.parent()->addChildNode(node);
        }
      }
    }
    return true;
  }
};

typedef std::list<Brush*> brushlist_t;

class BrushGatherSelected : public scene::Graph::Walker
{
  BrushVector& m_brushlist;
public:
  BrushGatherSelected(BrushVector& brushlist)
    : m_brushlist(brushlist)
  {
  }
  bool pre(const scene::Path& path, const scene::INodePtr& node) const
  {
    if(path.top()->visible())
    {
      Brush* brush = Node_getBrush(node);
      if(brush != NULL && Node_getSelectable(node)->isSelected())
      {
        m_brushlist.push_back(brush);
      }
    }
    return true;
  }
};

class BrushDeleteSelected : 
	public scene::Graph::Walker
{
	mutable std::list<scene::Path> _deleteList;
public:
	~BrushDeleteSelected() {
		for (std::list<scene::Path>::iterator i = _deleteList.begin(); i != _deleteList.end(); i++) {
			Path_deleteTop(*i);
		}
	}

	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		return true;
	}

	void post(const scene::Path& path, const scene::INodePtr& node) const {
		if (node->visible()) {
			Brush* brush = Node_getBrush(node);
			if (brush != NULL && Node_isSelected(node) && path.size() > 1) {
				_deleteList.push_back(path);
			}
		}
	}
};

void Scene_BrushMakeHollow_Selected(scene::Graph& graph)
{
  GlobalSceneGraph().traverse(BrushHollowSelectedWalker(GlobalGrid().getGridSize()));
  GlobalSceneGraph().traverse(BrushDeleteSelected());
}

/*
=============
CSG_MakeHollow
=============
*/

void CSG_MakeHollow (void)
{
  UndoableCommand undo("brushHollow");

  Scene_BrushMakeHollow_Selected(GlobalSceneGraph());

  SceneChangeNotify();
}

void CSG_MakeRoom() {
	UndoableCommand undo("brushRoom");

	GlobalSceneGraph().traverse(BrushHollowSelectedWalker(GlobalGrid().getGridSize(), true));
	GlobalSceneGraph().traverse(BrushDeleteSelected());

	SceneChangeNotify();
}

typedef Face* FacePointer;
const FacePointer c_nullFacePointer = 0;

inline bool Face_testPlane(const Face& face, const Plane3& plane, bool flipped)
{
	return face.contributes() && !face.getWinding().testPlane(plane, flipped);
}
typedef Function3<const Face&, const Plane3&, bool, bool, Face_testPlane> FaceTestPlane;

/// \brief Returns true if
/// \li !flipped && brush is BACK or ON
/// \li flipped && brush is FRONT or ON
bool Brush_testPlane(const Brush& brush, const Plane3& plane, bool flipped)
{
  brush.evaluateBRep();
#if 1
  for(Brush::const_iterator i(brush.begin()); i != brush.end(); ++i)
  {
    if(Face_testPlane(*(*i), plane, flipped))
    {
      return false;
    }
  }
  return true;
#else
  //return Brush_findIf(brush, bindArguments(FaceTestPlane(), makeReference(plane), flipped)) == 0;
#endif
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

bool Brush_subtract(const Brush& brush, const Brush& other, BrushVector& ret_fragments)
{
  if(aabb_intersects_aabb(brush.localAABB(), other.localAABB()))
  {
    BrushVector fragments;
    fragments.reserve(other.size());
    Brush back(brush);

    for(Brush::const_iterator i(other.begin()); i != other.end(); ++i)
    {
      if((*i)->contributes())
      {
        BrushSplitType split = Brush_classifyPlane(back, (*i)->plane3());
        if(split.counts[ePlaneFront] != 0
          && split.counts[ePlaneBack] != 0)
        {
          fragments.push_back(new Brush(back));
          FacePtr newFace = fragments.back()->addFace(*(*i));
          if(newFace != 0)
          {
            newFace->flipWinding();
          }
          back.addFace(*(*i));
        }
        else if(split.counts[ePlaneBack] == 0)
        {
          for(BrushVector::iterator i = fragments.begin(); i != fragments.end(); ++i)
          {
            delete(*i);
          }
          return false;
        }
      }
    }
    ret_fragments.insert(ret_fragments.end(), fragments.begin(), fragments.end());
    return true;
  }
  return false;
}

class SubtractBrushesFromUnselected : public scene::Graph::Walker
{
  const BrushVector& m_brushlist;
  std::size_t& m_before;
  std::size_t& m_after;
public:
  SubtractBrushesFromUnselected(const BrushVector& brushlist, std::size_t& before, std::size_t& after)
    : m_brushlist(brushlist), m_before(before), m_after(after)
  {
  }
  bool pre(const scene::Path& path, const scene::INodePtr& node) const
  {
    return true;
  }
  void post(const scene::Path& path, const scene::INodePtr& node) const
  {
    if(path.top()->visible())
    {
      Brush* brush = Node_getBrush(node);
      if(brush != NULL && !Node_getSelectable(node)->isSelected())
      {
        BrushVector buffer[2];
        bool swap = false;
        Brush* original = new Brush(*brush);
        buffer[static_cast<std::size_t>(swap)].push_back(original);
        
        {
          for(BrushVector::const_iterator i(m_brushlist.begin()); i != m_brushlist.end(); ++i)
          {
            for(BrushVector::iterator j(buffer[static_cast<std::size_t>(swap)].begin()); j != buffer[static_cast<std::size_t>(swap)].end(); ++j)
            {
              if(Brush_subtract(*(*j), *(*i), buffer[static_cast<std::size_t>(!swap)]))
              {
                delete (*j);
              }
              else
              {
                buffer[static_cast<std::size_t>(!swap)].push_back((*j));
              }
            }
            buffer[static_cast<std::size_t>(swap)].clear();
            swap = !swap;
          }
        }

        BrushVector& out = buffer[static_cast<std::size_t>(swap)];

        if(out.size() == 1 && out.back() == original)
        {
          delete original;
        }
        else
        {
          ++m_before;
          for(BrushVector::const_iterator i = out.begin(); i != out.end(); ++i)
          {
            ++m_after;
            scene::INodePtr node = GlobalBrushCreator().createBrush();
            (*i)->removeEmptyFaces();
            ASSERT_MESSAGE(!(*i)->empty(), "brush left with no faces after subtract");
            Node_getBrush(node)->copy(*(*i));
            delete (*i);
            path.parent()->addChildNode(node);
          }
          Path_deleteTop(path);
        }
      }
    }
  }
};

void CSG_Subtract()
{
  BrushVector selected_brushes;
  GlobalSceneGraph().traverse(BrushGatherSelected(selected_brushes));

  if (selected_brushes.empty())
  {
    globalOutputStream() << "CSG Subtract: No brushes selected.\n";
  }
  else
  {
    globalOutputStream() << "CSG Subtract: Subtracting " << Unsigned(selected_brushes.size()) << " brushes.\n";

    UndoableCommand undo("brushSubtract");

    // subtract selected from unselected
    std::size_t before = 0;
    std::size_t after = 0;
    GlobalSceneGraph().traverse(SubtractBrushesFromUnselected(selected_brushes, before, after));
    globalOutputStream() << "CSG Subtract: Result: "
      << Unsigned(after) << " fragment" << (after == 1 ? "" : "s")
      << " from " << Unsigned(before) << " brush" << (before == 1? "" : "es") << ".\n";

    SceneChangeNotify();
  }
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

/*
=============
CSG_Merge
=============
*/
bool Brush_merge(Brush& brush, const BrushVector& in, bool onlyshape)
{
  // gather potential outer faces 

  {
    typedef std::vector<const Face*> Faces;
    Faces faces;
    for(BrushVector::const_iterator i(in.begin()); i != in.end(); ++i)
    {
      (*i)->evaluateBRep();
      for(Brush::const_iterator j((*i)->begin()); j != (*i)->end(); ++j)
      {
        if(!(*j)->contributes())
        {
          continue;
        }

        const Face& face1 = *(*j);

        bool skip = false;
        // test faces of all input brushes
        //!\todo SPEEDUP: Flag already-skip faces and only test brushes from i+1 upwards.
        for(BrushVector::const_iterator k(in.begin()); !skip && k != in.end(); ++k)
        {
          if(k != i) // don't test a brush against itself
          {
            for(Brush::const_iterator l((*k)->begin()); !skip && l != (*k)->end(); ++l)
            {
              const Face& face2 = *(*l);

              // face opposes another face
              if (face1.plane3() == -face2.plane3())
              {
                // skip opposing planes
                skip  = true;
                break;
              }
            }
          }
        }

        // check faces already stored
        for(Faces::const_iterator m = faces.begin(); !skip && m != faces.end(); ++m)
        {
          const Face& face2 = *(*m);

          // face equals another face
          if (face1.plane3() == face2.plane3())
          {
            //if the texture/shader references should be the same but are not
            if (!onlyshape && !shader_equal(face1.getShader().getShader(), face2.getShader().getShader()))
            {
              return false;
            }
            // skip duplicate planes
            skip = true;
            break;
          }

          // face1 plane intersects face2 winding or vice versa
          if (Winding::planesConcave(face1.getWinding(), face2.getWinding(), face1.plane3(), face2.plane3()))
          {
            // result would not be convex
            return false;
          }
        }

        if(!skip)
        {
          faces.push_back(&face1);
        }
      }
    }
    for(Faces::const_iterator i = faces.begin(); i != faces.end(); ++i)
    {
      if(!brush.addFace(*(*i)))
      {
        // result would have too many sides
        return false;
      }
    }
  }

  brush.removeEmptyFaces();

  return true;
}

void CSG_Merge(void)
{
  BrushVector selected_brushes;

  // remove selected
  GlobalSceneGraph().traverse(BrushGatherSelected(selected_brushes));

  if (selected_brushes.empty())
  {
    globalOutputStream() << "CSG Merge: No brushes selected.\n";
    return;
  }

  if (selected_brushes.size() < 2)
  {
    globalOutputStream() << "CSG Merge: At least two brushes have to be selected.\n";
    return;
  }

  globalOutputStream() << "CSG Merge: Merging " << Unsigned(selected_brushes.size()) << " brushes.\n";

  UndoableCommand undo("brushMerge");

  //scene::Path merged_path = GlobalSelectionSystem().ultimateSelected().path();
	const scene::INodePtr& merged = GlobalSelectionSystem().ultimateSelected();
	scene::Path mergedPath = findPath(merged);

	// Create a new BrushNode
	scene::INodePtr node = GlobalBrushCreator().createBrush();
	
	// Get the contained brush
	Brush* brush = Node_getBrush(node);

	// Attempt to merge the selected brushes into the new one 
	if (!Brush_merge(*brush, selected_brushes, true)) {
		globalOutputStream() << "CSG Merge: Failed - result would not be convex.\n";
	}
	else {
		ASSERT_MESSAGE(!brush->empty(), "brush left with no faces after merge");

		// free the original brushes
		GlobalSceneGraph().traverse(BrushDeleteSelected());

		// Insert the newly created brush into the (same) parent entity
		mergedPath.pop();
		mergedPath.top()->addChildNode(node);
		mergedPath.push(node);

		Node_setSelected(node, true);

		globalOutputStream() << "CSG Merge: Succeeded.\n";
		SceneChangeNotify();
	}
}
