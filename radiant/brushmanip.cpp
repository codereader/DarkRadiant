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

#include "brushmanip.h"

#include "i18n.h"
#include "imainframe.h"
#include "iclipper.h"
#include "icommandsystem.h"
#include "ieventmanager.h"

#include "gtkutil/dialog/MessageBox.h"
#include "brush/BrushNode.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "ui/brush/QuerySidesDialog.h"
#include "shaderlib.h"

#include "brush/BrushVisit.h"
#include "brush/BrushModule.h"
#include "brush/FaceInstance.h"

#include "selection/algorithm/Shader.h"
#include "selection/algorithm/Primitives.h"
#include "xyview/GlobalXYWnd.h"

#include <list>

FaceInstanceSet g_SelectedFaceInstances;

int GetViewAxis()
{
  switch(GlobalXYWnd().getActiveViewType())
  {
    case XY:
      return 2;
    case XZ:
      return 1;
    case YZ:
      return 0;
  }
  return 2;
}

void Brush_ConstructPrefab(Brush& brush, EBrushPrefab type, const AABB& bounds, std::size_t sides, const std::string& shader, const TextureProjection& projection)
{
	switch(type)
	{
	case eBrushCuboid:
	{
		UndoableCommand undo("brushCuboid");

		brush.constructCuboid(bounds, shader, projection);
	}
	break;

	case eBrushPrism:
	{
		int axis = GetViewAxis();
		std::ostringstream command;
		command << "brushPrism -sides " << sides << " -axis " << axis;
		UndoableCommand undo(command.str());

		brush.constructPrism(bounds, sides, axis, shader, projection);
	}
	break;

	case eBrushCone:
	{
		std::ostringstream command;
		command << "brushCone -sides " << sides;
		UndoableCommand undo(command.str());

		brush.constructCone(bounds, sides, shader, projection);
	}
	break;

	case eBrushSphere:
	{
		std::ostringstream command;
		command << "brushSphere -sides " << sides;
		UndoableCommand undo(command.str());

		brush.constructSphere(bounds, sides, shader, projection);
	}
	break;

	default:
		break;
	}
}


void ConstructRegionBrushes(scene::INodePtr brushes[6], const Vector3& region_mins, const Vector3& region_maxs)
{
	const float THICKNESS = 10;
  {
    // set mins
    Vector3 mins(region_mins[0]-THICKNESS, region_mins[1]-THICKNESS, region_mins[2]-THICKNESS);

    // vary maxs
    for(std::size_t i=0; i<3; i++)
    {
      Vector3 maxs(region_maxs[0]+THICKNESS, region_maxs[1]+THICKNESS, region_maxs[2]+THICKNESS);
      maxs[i] = region_mins[i];

	  Brush& brush = *Node_getBrush(brushes[i]);
	  brush.constructCuboid(AABB::createFromMinMax(mins, maxs),
      						texdef_name_default(),
      						TextureProjection());
    }
  }

  {
    // set maxs
    Vector3 maxs(region_maxs[0]+THICKNESS, region_maxs[1]+THICKNESS, region_maxs[2]+THICKNESS);

    // vary mins
    for(std::size_t i=0; i<3; i++)
    {
      Vector3 mins(region_mins[0]-THICKNESS, region_mins[1]-THICKNESS, region_mins[2]-THICKNESS);
      mins[i] = region_maxs[i];
      Brush& brush = *Node_getBrush(brushes[i+3]);
	  
	  brush.constructCuboid(AABB::createFromMinMax(mins, maxs),
      						texdef_name_default(),
      						TextureProjection());
    }
  }
}

class FaceSetShader
{
  const std::string& m_name;
public:
  FaceSetShader(const std::string& name) : m_name(name) {}

  void operator()(Face& face) const {
    face.setShader(m_name);
  }
};

void Scene_BrushSetShader_Selected(scene::Graph& graph, const std::string& name)
{
  Scene_ForEachSelectedBrush_ForEachFace(graph, FaceSetShader(name));
  SceneChangeNotify();
}

void Scene_BrushSetShader_Component_Selected(scene::Graph& graph, const std::string& name)
{
  Scene_ForEachSelectedBrushFace(graph, FaceSetShader(name));
  SceneChangeNotify();
}

TextureProjection g_defaultTextureProjection;
const TextureProjection& TextureTransform_getDefault()
{
  g_defaultTextureProjection.constructDefault();
  return g_defaultTextureProjection;
}

void Scene_BrushConstructPrefab(scene::Graph& graph, EBrushPrefab type, std::size_t sides, const std::string& shader)
{
  if(GlobalSelectionSystem().countSelected() != 0)
  {
    const scene::INodePtr& node = GlobalSelectionSystem().ultimateSelected();

    Brush* brush = Node_getBrush(node);
    if(brush != 0)
    {
      AABB bounds = brush->localAABB(); // copy bounds because the brush will be modified
      Brush_ConstructPrefab(*brush, type, bounds, sides, shader, TextureTransform_getDefault());
      SceneChangeNotify();
    }
  }
}

void Scene_BrushResize_Selected(scene::Graph& graph, const AABB& bounds, const std::string& shader)
{
  if(GlobalSelectionSystem().countSelected() != 0)
  {
    const scene::INodePtr& node = GlobalSelectionSystem().ultimateSelected();

    Brush* brush = Node_getBrush(node);
    if(brush != 0)
    {
		brush->constructCuboid(bounds, shader, TextureTransform_getDefault());
		SceneChangeNotify();
    }
  }
}

void Scene_BrushResize(Brush& brush, const AABB& bounds, const std::string& shader)
{
	brush.constructCuboid(bounds, shader, TextureTransform_getDefault());
	SceneChangeNotify();
}

/**
 * Selects all visible brushes which carry the shader name as passed
 * to the constructor.
 */
class BrushSelectByShaderWalker :
	public scene::NodeVisitor
{
	std::string _name;
public:
	BrushSelectByShaderWalker(const std::string& name) :
		_name(name)
	{}

	bool pre(const scene::INodePtr& node) {
		if (node->visible()) {
			Brush* brush = Node_getBrush(node);

			if (brush != NULL && brush->hasShader(_name)) {
				Node_setSelected(node, true);
				return false; // don't traverse brushes
			}

			// not a suitable brush, traverse further
			return true;
		}
		else {
			return false; // don't traverse invisible nodes
		}
	}
};

void Scene_BrushSelectByShader(scene::Graph& graph, const std::string& name) {
	BrushSelectByShaderWalker walker(name);
	Node_traverseSubgraph(graph.root(), walker);
}

class FaceSelectByShader
{
  std::string m_name;
public:
  FaceSelectByShader(const std::string& name)
    : m_name(name)
  {
  }
  void operator()(FaceInstance& face) const
  {
    if(shader_equal(face.getFace().getShader(), m_name))
    {
      face.setSelected(SelectionSystem::eFace, true);
    }
  }
};

void Scene_BrushSelectByShader_Component(scene::Graph& graph, const std::string& name)
{
  Scene_ForEachSelectedBrush_ForEachFaceInstance(graph, FaceSelectByShader(name));
}

class FaceTextureFlipper
{
	unsigned int _flipAxis;
public:
	FaceTextureFlipper(unsigned int flipAxis) :
		_flipAxis(flipAxis)
	{}

	void operator()(Face& face) const {
		face.flipTexture(_flipAxis);
	}
};

void Scene_BrushFlipTexture_Selected(unsigned int flipAxis) {
	Scene_ForEachSelectedBrush_ForEachFace(GlobalSceneGraph(), FaceTextureFlipper(flipAxis));
}

void Scene_BrushFlipTexture_Component_Selected(unsigned int flipAxis) {
	Scene_ForEachSelectedBrushFace(GlobalSceneGraph(), FaceTextureFlipper(flipAxis));
}

void brushMakeSided(const cmd::ArgumentList& args) {
	if (args.size() != 1) {
		return;
	}

	// First argument contains the number of sides
	int input = args[0].getInt();

	if (input < 0) {
		rError() << "BrushMakeSide: invalid number of sides: " << input << std::endl;
		return;
	}

	std::size_t numSides = static_cast<std::size_t>(input);
	Scene_BrushConstructPrefab(GlobalSceneGraph(), eBrushPrism, numSides, GlobalTextureBrowser().getSelectedShader());
}

void brushMakePrefab(const cmd::ArgumentList& args)
{
	if (args.size() != 1) {
		return;
	}

	if (GlobalSelectionSystem().getSelectionInfo().brushCount != 1)
	{
		// Display a modal error dialog
		gtkutil::MessageBox::ShowError(_("Exactly one brush must be selected for this operation."), GlobalMainFrame().getTopLevelWindow());
		return;
	}

	// First argument contains the number of sides
	int input = args[0].getInt();

	if (input >= eBrushCuboid && input < eNumPrefabTypes) {
		// Boundary checks passed
		EBrushPrefab type = static_cast<EBrushPrefab>(input);

		int minSides = 3;
		int maxSides = Brush::PRISM_MAX_SIDES;

		switch (type)
		{
		case eBrushCuboid:
			// Cuboids don't need to query the number of sides
			Scene_BrushConstructPrefab(GlobalSceneGraph(), type, 0, GlobalTextureBrowser().getSelectedShader());
			return;

		case eBrushPrism:
			minSides = Brush::PRISM_MIN_SIDES;
			maxSides = Brush::PRISM_MAX_SIDES;
			break;

		case eBrushCone:
			minSides = Brush::CONE_MIN_SIDES;
			maxSides = Brush::CONE_MAX_SIDES;
			break;

		case eBrushSphere:
			minSides = Brush::SPHERE_MIN_SIDES;
			maxSides = Brush::SPHERE_MAX_SIDES;
			break;
		default:
			maxSides = 9999;
		};

		ui::QuerySidesDialog dialog(minSides, maxSides);

		int sides = dialog.queryNumberOfSides();

		if (sides != -1)
		{
			Scene_BrushConstructPrefab(GlobalSceneGraph(), type, sides, GlobalTextureBrowser().getSelectedShader());
		}
	}
	else {
		rError() << "BrushMakePrefab: invalid prefab type. Allowed types are: " << std::endl
			<< eBrushCuboid << " = cuboid " << std::endl
			<< eBrushPrism  << " = prism " << std::endl
			<< eBrushCone  << " = cone " << std::endl
			<< eBrushSphere << " = sphere " << std::endl;
	}
}

void Brush_registerCommands()
{
	GlobalEventManager().addRegistryToggle("TogTexLock", RKEY_ENABLE_TEXTURE_LOCK);

	GlobalCommandSystem().addCommand("BrushMakePrefab", brushMakePrefab, cmd::ARGTYPE_INT);

	GlobalEventManager().addCommand("BrushCuboid", "BrushCuboid");
	GlobalEventManager().addCommand("BrushPrism", "BrushPrism");
	GlobalEventManager().addCommand("BrushCone", "BrushCone");
	GlobalEventManager().addCommand("BrushSphere", "BrushSphere");

	GlobalCommandSystem().addCommand("BrushMakeSided", brushMakeSided, cmd::ARGTYPE_INT);

	// Link the Events to the corresponding statements
	GlobalEventManager().addCommand("Brush3Sided", "Brush3Sided");
	GlobalEventManager().addCommand("Brush4Sided", "Brush4Sided");
	GlobalEventManager().addCommand("Brush5Sided", "Brush5Sided");
	GlobalEventManager().addCommand("Brush6Sided", "Brush6Sided");
	GlobalEventManager().addCommand("Brush7Sided", "Brush7Sided");
	GlobalEventManager().addCommand("Brush8Sided", "Brush8Sided");
	GlobalEventManager().addCommand("Brush9Sided", "Brush9Sided");

	GlobalCommandSystem().addCommand("TextureNatural", selection::algorithm::naturalTexture);
	GlobalCommandSystem().addCommand("MakeVisportal", selection::algorithm::makeVisportal);
	GlobalCommandSystem().addCommand("SurroundWithMonsterclip", selection::algorithm::surroundWithMonsterclip);
	GlobalEventManager().addCommand("TextureNatural", "TextureNatural");
	GlobalEventManager().addCommand("MakeVisportal", "MakeVisportal");
	GlobalEventManager().addCommand("SurroundWithMonsterclip", "SurroundWithMonsterclip");
}
