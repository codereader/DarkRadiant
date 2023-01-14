#include "Primitives.h"

#include <fstream>

#include "i18n.h"
#include "igroupnode.h"
#include "ientity.h"
#include "itextstream.h"
#include "iundo.h"
#include "ibrush.h"
#include "igame.h"
#include "iorthoview.h"

#include "brush/BrushModule.h"
#include "brush/BrushVisit.h"
#include "patch/PatchNode.h"
#include "string/string.h"
#include "brush/export/CollisionModel.h"
#include "os/fs.h"
#include "gamelib.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "scenelib.h"
#include "scene/ModelFinder.h"
#include "selectionlib.h"
#include "command/ExecutionFailure.h"
#include "command/ExecutionNotPossible.h"
#include "messages/NotificationMessage.h"

#include "string/case_conv.h"
#include <fmt/format.h>

namespace selection
{

namespace algorithm
{

namespace
{
	const char* const GKEY_CM_EXT = "/defaults/collisionModelExt";
	const char* const GKEY_NODRAW_SHADER = "/defaults/nodrawShader";
	const char* const GKEY_VISPORTAL_SHADER = "/defaults/visportalShader";
	const char* const GKEY_MONSTERCLIP_SHADER = "/defaults/monsterClipShader";
}

void forEachSelectedFaceComponent(const std::function<void(IFace&)>& functor)
{
	std::for_each(FaceInstance::Selection().begin(), FaceInstance::Selection().end(),
		[&] (FaceInstance* instance) { functor(instance->getFace()); });
}

Patch& getLastSelectedPatch() {
	if (GlobalSelectionSystem().getSelectionInfo().totalCount > 0 &&
		GlobalSelectionSystem().getSelectionInfo().patchCount > 0)
	{
		// Retrieve the last selected instance
		const scene::INodePtr& node = GlobalSelectionSystem().ultimateSelected();
		// Try to cast it onto a patch
		Patch* patch = Node_getPatch(node);

		// Return or throw
		if (patch != NULL) {
			return *patch;
		}
		else {
			throw selection::InvalidSelectionException(_("No patches selected."));
		}
	}
	else {
		throw selection::InvalidSelectionException(_("No patches selected."));
	}
}

PatchPtrVector getSelectedPatches()
{
	PatchPtrVector returnVector;

	GlobalSelectionSystem().foreachSelected([&] (const scene::INodePtr& node)
	{
		if (Node_isPatch(node))
		{
			returnVector.emplace_back(std::dynamic_pointer_cast<PatchNode>(node));
		}
	});

	return returnVector;
}

BrushPtrVector getSelectedBrushes()
{
	BrushPtrVector returnVector;

	GlobalSelectionSystem().foreachBrush([&] (Brush& brush)
	{
		returnVector.push_back(std::static_pointer_cast<BrushNode>(brush.getBrushNode().shared_from_this()));
	});

	return returnVector;
}

FacePtrVector getSelectedFaces()
{
	FacePtrVector vector;

	// Cycle through all selected faces and fill the vector
	std::for_each(FaceInstance::Selection().begin(), FaceInstance::Selection().end(),
		[&] (FaceInstance* instance) { vector.push_back(&instance->getFace()); });

	return vector;
}

// Try to create a CM from the selected entity
void createCMFromSelection(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rWarning() << "Usage: ExportSelectedAsCollisionModel <modelPath>" << std::endl;
		return;
	}

	// Check the current selection state
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.totalCount != info.entityCount || info.entityCount != 1)
	{
		throw cmd::ExecutionNotPossible(_("Can't export, create and select a func_* entity\
				containing the collision hull primitives."));
	}

	std::string model = args[0].getString();

	// Retrieve the node, instance and entity
	const scene::INodePtr& entityNode = GlobalSelectionSystem().ultimateSelected();

	// Try to retrieve the group node
	scene::GroupNodePtr groupNode = Node_getGroupNode(entityNode);

	// Remove the entity origin from the brushes
	if (groupNode)
	{
		groupNode->removeOriginFromChildren();

		// Deselect the node
		Node_setSelected(entityNode, false);

		// Select all the child nodes
		entityNode->foreachNode([] (const scene::INodePtr& child)->bool
		{
			Node_setSelected(child, true);
			return true;
		});

		BrushPtrVector brushes = algorithm::getSelectedBrushes();

		// Create a new collisionmodel on the heap using a shared_ptr
		cmutil::CollisionModelPtr cm(new cmutil::CollisionModel());

		// Add all the brushes to the collision model
		for (std::size_t i = 0; i < brushes.size(); i++) {
			cm->addBrush(brushes[i]->getBrush());
		}

		std::string basePath = game::current::getWriteableGameResourcePath();

		std::string modelPath = basePath + model;

		std::string newExtension = "." + game::current::getValue<std::string>(GKEY_CM_EXT);

		// Set the model string to correctly associate the clipmodel
		cm->setModel(model);

		try
		{
            // Make sure the output folder exists
            fs::create_directories(os::getDirectory(modelPath));

			// create the new filename by changing the extension
			fs::path cmPath = os::replaceExtension(modelPath, newExtension);

			// Open the stream to the output file
			std::ofstream outfile(cmPath.string());

			if (outfile.is_open())
			{
				// Insert the CollisionModel into the stream
				outfile << *cm;

				// Close the file
				outfile.close();

				rMessage() << "CollisionModel saved to " << cmPath.string() << std::endl;
			}
			else
			{
				throw cmd::ExecutionFailure(fmt::format(_("Couldn't save to file: {0}"), cmPath.string()));
			}
		}
		catch (const fs::filesystem_error& f)
		{
			rError() << "CollisionModel: " << f.what() << std::endl;
		}

		// De-select the child brushes
		GlobalSelectionSystem().setSelectedAll(false);

		// Re-add the origin to the brushes
		groupNode->addOriginToChildren();

		// Re-select the node
		Node_setSelected(entityNode, true);
	}
}

/**
 * greebo: Functor class which creates a decal patch for each visited face instance.
 */
class DecalPatchCreator
{
	int _unsuitableWindings;

	typedef std::list<FaceInstance*> FaceInstanceList;
	FaceInstanceList _faceInstances;

public:
	DecalPatchCreator() :
		_unsuitableWindings(0)
	{}

	void createDecals() {
		for (FaceInstanceList::iterator i = _faceInstances.begin(); i != _faceInstances.end(); ++i) {
			// Get the winding
			const Winding& winding = (*i)->getFace().getWinding();

			// Create a new decal patch
			scene::INodePtr patchNode = GlobalPatchModule().createPatch(patch::PatchDefType::Def3);

			if (patchNode == NULL)
			{
				throw cmd::ExecutionFailure(_("Could not create patch."));
				return;
			}

			Patch* patch = Node_getPatch(patchNode);
			assert(patch != NULL); // must not fail

			// Set the tesselation of that 3x3 patch
			patch->setDims(3,3);
			patch->setFixedSubdivisions(true, Subdivisions(1,1));

			if (winding.size() == 4)
			{
				// Rectangular face, set the coordinates
				patch->ctrlAt(0,0).vertex = winding[0].vertex;
				patch->ctrlAt(2,0).vertex = winding[1].vertex;
				patch->ctrlAt(1,0).vertex = (patch->ctrlAt(0,0).vertex + patch->ctrlAt(2,0).vertex)/2;

				patch->ctrlAt(0,1).vertex = (winding[0].vertex + winding[3].vertex)/2;
				patch->ctrlAt(2,1).vertex = (winding[1].vertex + winding[2].vertex)/2;

				patch->ctrlAt(1,1).vertex = (patch->ctrlAt(0,1).vertex + patch->ctrlAt(2,1).vertex)/2;

				patch->ctrlAt(2,2).vertex = winding[2].vertex;
				patch->ctrlAt(0,2).vertex = winding[3].vertex;
				patch->ctrlAt(1,2).vertex = (patch->ctrlAt(2,2).vertex + patch->ctrlAt(0,2).vertex)/2;
			}
			else
			{
				// Non-rectangular face, try to find 4 vertices co-planar with the face

				// We have at least 3 points, so use them
				Vector3 points[4] = {
					winding[0].vertex,
					winding[1].vertex,
					winding[2].vertex,
					Vector3(0,0,0) // to be calculated
				};

				// Triangular patches are supported, collapse the fourth point with the third one
				if (winding.size() == 3)
				{
					points[3] = points[2];
				}
				else
				{
					// Generic polygon, just assume a fourth point in the same plane
					points[3] = points[1] + (points[0] - points[1]) + (points[2] - points[1]);
				}

				patch->ctrlAt(0,0).vertex = points[0];
				patch->ctrlAt(2,0).vertex = points[1];
				patch->ctrlAt(1,0).vertex = (patch->ctrlAt(0,0).vertex + patch->ctrlAt(2,0).vertex)/2;

				patch->ctrlAt(0,1).vertex = (points[0] + points[3])/2;
				patch->ctrlAt(2,1).vertex = (points[1] + points[2])/2;

				patch->ctrlAt(1,1).vertex = (patch->ctrlAt(0,1).vertex + patch->ctrlAt(2,1).vertex)/2;

				patch->ctrlAt(2,2).vertex = points[2];
				patch->ctrlAt(0,2).vertex = points[3];
				patch->ctrlAt(1,2).vertex = (patch->ctrlAt(2,2).vertex + patch->ctrlAt(0,2).vertex)/2;
			}

			// Use the texture in the clipboard, if it's a decal texture
			Texturable& clipboard = ShaderClipboard::Instance().getSource();

			if (!clipboard.empty())
			{
				if (clipboard.getShader().find("decals") != std::string::npos)
				{
					patch->setShader(clipboard.getShader());
				}
			}

			// Fit the texture on it
			patch->fitTexture(1,1);
			patch->flipTexture(1);

			// Insert the patch into worldspawn
			scene::INodePtr worldSpawnNode = GlobalMapModule().findOrInsertWorldspawn();
			assert(worldSpawnNode); // This must be non-NULL, otherwise we won't have faces

			worldSpawnNode->addChildNode(patchNode);

			// Deselect the face instance
			(*i)->setSelected(selection::ComponentSelectionMode::Face, false);

			// Select the patch
			Node_setSelected(patchNode, true);
		}
	}

	void operator() (FaceInstance& faceInstance)
	{
		// Skip non-contributing faces
		if (faceInstance.getFace().contributes())
		{
			_faceInstances.push_back(&faceInstance);
		}
		else
		{
			// Fail on this winding, de-select and skip
			faceInstance.setSelected(selection::ComponentSelectionMode::Face, false);
			_unsuitableWindings++;
		}
	}

	int getNumUnsuitableWindings() const
	{
		return _unsuitableWindings;
	}
};

void createDecalsForSelectedFaces()
{
	// Sanity check
	if (FaceInstance::Selection().empty())
	{
		throw cmd::ExecutionNotPossible(_("No faces selected."));
		return;
	}

	// Create a scoped undocmd object
	UndoableCommand cmd("createDecalsForSelectedFaces");

	// greebo: For each face, create a patch with fixed tesselation
	DecalPatchCreator creator;

	std::for_each(FaceInstance::Selection().begin(), FaceInstance::Selection().end(),
		[&] (FaceInstance* instance) { creator.operator()(*instance); });

	// Issue the command
	creator.createDecals();

	// Check how many faces were not suitable
	int unsuitableWindings = creator.getNumUnsuitableWindings();

	if (unsuitableWindings > 0)
	{
		radiant::NotificationMessage::SendInformation(
			fmt::format(_("{0:d} faces were not suitable (had more than 4 vertices)."), unsuitableWindings)
		);
	}
}

void makeVisportal()
{
	BrushPtrVector brushes = getSelectedBrushes();

	if (brushes.size() <= 0)
	{
		throw cmd::ExecutionNotPossible(_("No brushes selected."));
		return;
	}

	// Create a scoped undocmd object
	UndoableCommand cmd("brushMakeVisportal");

	for (std::size_t i = 0; i < brushes.size(); i++)
	{
		Brush& brush = brushes[i]->getBrush();

		// don't allow empty brushes
		if (brush.getNumFaces() == 0) continue;

		// Set all faces to nodraw first
		brush.setShader(game::current::getValue<std::string>(GKEY_NODRAW_SHADER));

		// Find the largest face (in terms of area)
		Face* largestFace = NULL;
		double largestArea = 0;

		brush.forEachFace([&] (Face& face)
		{
			if (largestFace == NULL)
			{
				largestFace = &face;
			}

			// Calculate face area
            double area = 0;
			Winding& winding = face.getWinding();
			const Vector3& centroid = face.centroid();

			for (std::size_t i = 0; i < winding.size(); i++)
			{
				Vector3 edge1 = centroid - winding[i].vertex;
				Vector3 edge2 = centroid - winding[(i+1) % winding.size()].vertex;
				area += edge1.cross(edge2).getLength() * 0.5;
			}

			if (area > largestArea)
			{
				largestArea = area;
				largestFace = &face;
			}
		});

		// We don't allow empty brushes so face must be non-NULL at this point
		assert(largestFace != NULL);

		largestFace->setShader(game::current::getValue<std::string>(GKEY_VISPORTAL_SHADER));
	}
}

void surroundWithMonsterclip(const cmd::ArgumentList& args)
{
	UndoableCommand command("addMonsterclip");

	// create a ModelFinder and retrieve the modelList
	scene::ModelFinder visitor;
	GlobalSelectionSystem().foreachSelected(visitor);

	// Retrieve the list with all the found models from the visitor
	auto list = visitor.getList();

	for (const auto& model : list)
	{
		// retrieve the AABB
		AABB brushAABB(model->worldAABB());

		// create the brush
		scene::INodePtr brushNode(GlobalBrushCreator().createBrush());

		brushNode->assignToLayers(model->getLayers());

		if (brushNode != NULL) {
			scene::addNodeToContainer(brushNode, GlobalMapModule().findOrInsertWorldspawn());

			Brush* theBrush = Node_getBrush(brushNode);

			std::string clipShader = game::current::getValue<std::string>(GKEY_MONSTERCLIP_SHADER);

			resizeBrushToBounds(*theBrush, brushAABB, clipShader);
		}
	}
}

void resizeBrushToBounds(Brush& brush, const AABB& aabb, const std::string& shader)
{
	brush.constructCuboid(aabb, shader);
	SceneChangeNotify();
}

void resizeBrushesToBounds(const AABB& aabb, const std::string& shader)
{
	if (GlobalSelectionSystem().getSelectionInfo().brushCount == 0)
	{
		throw cmd::ExecutionNotPossible(_("No brushes selected."));
	}

	GlobalSelectionSystem().foreachBrush([&] (Brush& brush)
	{
		brush.constructCuboid(aabb, shader);
	});

	SceneChangeNotify();
}

void resizeSelectedBrushesToBounds(const cmd::ArgumentList& args)
{
	if (args.size() != 3)
	{
		rWarning() << "Usage: ResizeSelectedBrushesToBounds <AABBminPoint> <AABBmaxPoint> <shaderName>" << std::endl;
		return;
	}

	auto bounds = AABB::createFromMinMax(args[0].getVector3(), args[1].getVector3());
	resizeBrushesToBounds(bounds, args[2].getString());
}

int GetViewAxis()
{
	switch(GlobalXYWndManager().getActiveViewType())
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

void constructBrushPrefab(Brush& brush, brush::PrefabType type, const AABB& bounds, std::size_t sides, const std::string& shader)
{
	switch(type)
	{
	case brush::PrefabType::Cuboid:
	{
		UndoableCommand undo("brushCuboid");

		brush.constructCuboid(bounds, shader);
	}
	break;

	case brush::PrefabType::Prism:
	{
		int axis = GetViewAxis();
		std::ostringstream command;
		command << "brushPrism -sides " << sides << " -axis " << axis;
		UndoableCommand undo(command.str());

		brush.constructPrism(bounds, sides, axis, shader);
	}
	break;

	case brush::PrefabType::Cone:
	{
		std::ostringstream command;
		command << "brushCone -sides " << sides;
		UndoableCommand undo(command.str());

		brush.constructCone(bounds, sides, shader);
	}
	break;

	case brush::PrefabType::Sphere:
	{
		std::ostringstream command;
		command << "brushSphere -sides " << sides;
		UndoableCommand undo(command.str());

		brush.constructSphere(bounds, sides, shader);
	}
	break;

	default:
		break;
	}
}

void constructBrushPrefabs(brush::PrefabType type, std::size_t sides, const std::string& shader)
{
	GlobalSelectionSystem().foreachBrush([&] (Brush& brush)
	{
		AABB bounds = brush.localAABB(); // copy bounds because the brush will be modified
		constructBrushPrefab(brush, type, bounds, sides, shader);
	});

	SceneChangeNotify();
}

void brushMakePrefab(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().getSelectionInfo().brushCount == 0)
	{
		// Display a modal error dialog
		throw cmd::ExecutionNotPossible(_("At least one brush must be selected for this operation."));
	}

	if (args.empty() ||
		args.size() > 2 ||
		(args[0].getInt() == 0 && args.size() > 1) ||
		(args[0].getInt() != 0 && args.size() < 2))
	{
		rError() << "Usage: " << std::endl
			<< "BrushMakePrefab " << static_cast<int>(brush::PrefabType::Cuboid) << " --> cuboid (4 sides)" << std::endl
			<< "BrushMakePrefab " << static_cast<int>(brush::PrefabType::Prism)  << " <numSides> --> prism " << std::endl
			<< "BrushMakePrefab " << static_cast<int>(brush::PrefabType::Cone)  << " <numSides> --> cone " << std::endl
			<< "BrushMakePrefab " << static_cast<int>(brush::PrefabType::Sphere) << " <numSides> --> sphere " << std::endl;
		return;
	}

	// First argument contains the type
	int input = args[0].getInt();

	int sides = input != 0 ? args[1].getInt() : 4; // cuboids always have 4 sides

	if (input >= static_cast<int>(brush::PrefabType::Cuboid) && input < static_cast<int>(brush::PrefabType::NumPrefabTypes))
	{
		// Boundary checks passed
		auto type = static_cast<brush::PrefabType>(input);

		const auto& shader = ShaderClipboard::Instance().getSource().getShader();

		// Cuboids ignore the sides argument
		constructBrushPrefabs(type, sides, shader);
	}
	else
	{
		rError() << "BrushMakePrefab: invalid prefab type. Allowed types are: " << std::endl
			<< static_cast<int>(brush::PrefabType::Cuboid) << " = cuboid " << std::endl
			<< static_cast<int>(brush::PrefabType::Prism)  << " = prism " << std::endl
			<< static_cast<int>(brush::PrefabType::Cone)  << " = cone " << std::endl
			<< static_cast<int>(brush::PrefabType::Sphere) << " = sphere " << std::endl;
	}
}

void brushMakeSided(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rError() << "Usage: BrushMakeSided <numSides>" << std::endl;
		return;
	}

	// First argument contains the number of sides
	int input = args[0].getInt();

	if (input < 0)
	{
		rError() << "BrushMakeSide: invalid number of sides: " << input << std::endl;
		return;
	}

	std::size_t numSides = static_cast<std::size_t>(input);
	constructBrushPrefabs(brush::PrefabType::Prism, numSides, ShaderClipboard::Instance().getSource().getShader());
}

void brushSetDetailFlag(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rError() << "Usage: BrushSetDetailFlag [detail|structural]" << std::endl;
		return;
	}

	// First argument contains the number of sides
	std::string arg = string::to_lower_copy(args[0].getString());

	if (arg == "detail")
	{
		UndoableCommand undo("BrushMakeDetail");

		GlobalSelectionSystem().foreachBrush([&] (Brush& brush)
		{
			brush.setDetailFlag(IBrush::Detail);
		});
	}
	else if (arg == "structural")
	{
		UndoableCommand undo("BrushMakeStructural");

		GlobalSelectionSystem().foreachBrush([&] (Brush& brush)
		{
			brush.setDetailFlag(IBrush::Structural);
		});
	}
	else
	{
		rError() << "Usage: BrushMakeDetail [detail|structural]" << std::endl;
	}
}

} // namespace algorithm

} // namespace selection
