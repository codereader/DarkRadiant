#include "Primitives.h"

#include <fstream>

#include "i18n.h"
#include "igroupnode.h"
#include "ientity.h"
#include "itextstream.h"
#include "iundo.h"
#include "imainframe.h"
#include "brush/BrushModule.h"
#include "brush/BrushVisit.h"
#include "patch/PatchSceneWalk.h"
#include "patch/PatchNode.h"
#include "string/string.h"
#include "brush/export/CollisionModel.h"
#include "gtkutil/dialog/MessageBox.h"
#include "map/Map.h"
#include "gamelib.h"
#include "ui/modelselector/ModelSelector.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "ui/brush/QuerySidesDialog.h"
#include "settings/GameManager.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "scenelib.h"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/format.hpp>

#include "ModelFinder.h"
#include "xyview/GlobalXYWnd.h"

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

	const std::string ERRSTR_WRONG_SELECTION =
			"Can't export, create and select a func_* entity\
				containing the collision hull primitives.";

	// Filesystem path typedef
	typedef boost::filesystem::path Path;
}

void forEachSelectedFaceComponent(const std::function<void(Face&)>& functor)
{
	std::for_each(FaceInstance::Selection().begin(), FaceInstance::Selection().end(),
		[&] (FaceInstance* instance) { functor(instance->getFace()); });
}

int selectedFaceCount()
{
	return static_cast<int>(FaceInstance::Selection().size());
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

	GlobalSelectionSystem().foreachPatch([&] (Patch& patch)
	{
		returnVector.push_back(boost::static_pointer_cast<PatchNode>(patch.getPatchNode().shared_from_this()));
	});

	return returnVector;
}

BrushPtrVector getSelectedBrushes()
{
	BrushPtrVector returnVector;

	GlobalSelectionSystem().foreachBrush([&] (Brush& brush)
	{
		returnVector.push_back(boost::static_pointer_cast<BrushNode>(brush.getBrushNode().shared_from_this()));
	});

	return returnVector;
}

Face& getLastSelectedFace()
{
	if (selectedFaceCount() == 1)
	{
		return FaceInstance::Selection().back()->getFace();
	}
	else
	{
		throw selection::InvalidSelectionException(string::to_string(selectedFaceCount()));
	}
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
void createCMFromSelection(const cmd::ArgumentList& args) {
	// Check the current selection state
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.totalCount == info.entityCount && info.totalCount == 1) {
		// Retrieve the node, instance and entity
		const scene::INodePtr& entityNode = GlobalSelectionSystem().ultimateSelected();

		// Try to retrieve the group node
		scene::GroupNodePtr groupNode = Node_getGroupNode(entityNode);

		// Remove the entity origin from the brushes
		if (groupNode != NULL) {
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

			ui::ModelSelectorResult modelAndSkin = ui::ModelSelector::chooseModel("", false, false);
			std::string basePath = GlobalGameManager().getModPath();

			std::string modelPath = basePath + modelAndSkin.model;

			std::string newExtension = "." + game::current::getValue<std::string>(GKEY_CM_EXT);

			// Set the model string to correctly associate the clipmodel
			cm->setModel(modelAndSkin.model);

			try {
				// create the new autosave filename by changing the extension
				Path cmPath = modelPath;
				cmPath.replace_extension(newExtension);

				// Open the stream to the output file
				std::ofstream outfile(cmPath.string().c_str());

				if (outfile.is_open()) {
					// Insert the CollisionModel into the stream
					outfile << *cm;
					// Close the file
					outfile.close();

					rMessage() << "CollisionModel saved to " << cmPath.string() << std::endl;
				}
				else {
					gtkutil::MessageBox::ShowError(
						(boost::format("Couldn't save to file: %s") % cmPath.string()).str(),
						 GlobalMainFrame().getTopLevelWindow());
				}
			}
			catch (boost::filesystem::filesystem_error f) {
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
	else {
		gtkutil::MessageBox::ShowError(
			_(ERRSTR_WRONG_SELECTION.c_str()),
			GlobalMainFrame().getTopLevelWindow());
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
			scene::INodePtr patchNode = GlobalPatchCreator(DEF3).createPatch();

			if (patchNode == NULL) {
				gtkutil::MessageBox::ShowError(_("Could not create patch."), GlobalMainFrame().getTopLevelWindow());
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
			Texturable& clipboard = GlobalShaderClipboard().getSource();

			if (!clipboard.empty())
			{
				if (clipboard.getShader().find("decals") != std::string::npos)
				{
					patch->setShader(clipboard.getShader());
				}
			}

			// Fit the texture on it
			patch->SetTextureRepeat(1,1);
			patch->FlipTexture(1);

			// Insert the patch into worldspawn
			scene::INodePtr worldSpawnNode = GlobalMap().getWorldspawn();
			assert(worldSpawnNode != NULL); // This must be non-NULL, otherwise we won't have faces

			worldSpawnNode->addChildNode(patchNode);

			// Deselect the face instance
			(*i)->setSelected(SelectionSystem::eFace, false);

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
			faceInstance.setSelected(SelectionSystem::eFace, false);
			_unsuitableWindings++;
		}
	}

	int getNumUnsuitableWindings() const
	{
		return _unsuitableWindings;
	}
};

void createDecalsForSelectedFaces(const cmd::ArgumentList& args) {
	// Sanity check
	if (FaceInstance::Selection().empty())
	{
		gtkutil::MessageBox::ShowError(_("No faces selected."), GlobalMainFrame().getTopLevelWindow());
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

	if (unsuitableWindings > 0) {
		gtkutil::MessageBox::ShowError(
			(boost::format(_("%d faces were not suitable (had more than 4 vertices).")) % unsuitableWindings).str(),
			GlobalMainFrame().getTopLevelWindow()
		);
	}
}

void makeVisportal(const cmd::ArgumentList& args)
{
	BrushPtrVector brushes = getSelectedBrushes();

	if (brushes.size() <= 0)
	{
		gtkutil::MessageBox::ShowError(_("No brushes selected."), GlobalMainFrame().getTopLevelWindow());
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
		float largestArea = 0;

		brush.forEachFace([&] (Face& face)
		{
			if (largestFace == NULL)
			{
				largestFace = &face;
			}

			// Calculate face area
			float area = 0;
			Winding& winding = face.getWinding();
			const Vector3& centroid = face.centroid();

			for (std::size_t i = 0; i < winding.size(); i++)
			{
				Vector3 edge1 = centroid - winding[i].vertex;
				Vector3 edge2 = centroid - winding[(i+1) % winding.size()].vertex;
				area += edge1.crossProduct(edge2).getLength() * 0.5f;
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
	ModelFinder visitor;
	GlobalSelectionSystem().foreachSelected(visitor);

	// Retrieve the list with all the found models from the visitor
	ModelFinder::ModelList list = visitor.getList();

	ModelFinder::ModelList::iterator iter;
	for (iter = list.begin(); iter != list.end(); ++iter)
	{
		// one of the models in the SelectionStack
		const scene::INodePtr& node = *iter;

		// retrieve the AABB
		AABB brushAABB(node->worldAABB());

		// create the brush
		scene::INodePtr brushNode(GlobalBrushCreator().createBrush());

		if (brushNode != NULL) {
			scene::addNodeToContainer(brushNode, GlobalMap().findOrInsertWorldspawn());

			Brush* theBrush = Node_getBrush(brushNode);

			std::string clipShader = game::current::getValue<std::string>(GKEY_MONSTERCLIP_SHADER);

			resizeBrushToBounds(*theBrush, brushAABB, clipShader);
		}
	}
}

void resizeBrushToBounds(Brush& brush, const AABB& aabb, const std::string& shader)
{
	brush.constructCuboid(aabb, shader, TextureProjection());
	SceneChangeNotify();
}

void resizeBrushesToBounds(const AABB& aabb, const std::string& shader)
{
	if (GlobalSelectionSystem().getSelectionInfo().brushCount == 0)
	{
		gtkutil::MessageBox::ShowError(_("No brushes selected."), GlobalMainFrame().getTopLevelWindow());
		return;
	}

	GlobalSelectionSystem().foreachBrush([&] (Brush& brush)
	{ 
		brush.constructCuboid(aabb, shader, TextureProjection());
	});

	SceneChangeNotify();
}

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

void constructBrushPrefab(Brush& brush, EBrushPrefab type, const AABB& bounds, std::size_t sides, const std::string& shader, const TextureProjection& projection)
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

void constructBrushPrefabs(EBrushPrefab type, std::size_t sides, const std::string& shader)
{
	GlobalSelectionSystem().foreachBrush([&] (Brush& brush)
	{
		AABB bounds = brush.localAABB(); // copy bounds because the brush will be modified
		constructBrushPrefab(brush, type, bounds, sides, shader, TextureProjection());
	});

	SceneChangeNotify();
}

void brushMakePrefab(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rError() << "Usage: " << std::endl
			<< "BrushMakePrefab " << eBrushCuboid << " --> cuboid " << std::endl
			<< "BrushMakePrefab " << eBrushPrism  << " --> prism " << std::endl
			<< "BrushMakePrefab " << eBrushCone  << " --> cone " << std::endl
			<< "BrushMakePrefab " << eBrushSphere << " --> sphere " << std::endl;
		return;
	}

	if (GlobalSelectionSystem().getSelectionInfo().brushCount == 0)
	{
		// Display a modal error dialog
		gtkutil::MessageBox::ShowError(_("At least one brush must be selected for this operation."), GlobalMainFrame().getTopLevelWindow());
		return;
	}

	// First argument contains the number of sides
	int input = args[0].getInt();

	if (input >= eBrushCuboid && input < eNumPrefabTypes)
	{
		// Boundary checks passed
		EBrushPrefab type = static_cast<EBrushPrefab>(input);

		int minSides = 3;
		int maxSides = static_cast<int>(Brush::PRISM_MAX_SIDES);

		const std::string& shader = GlobalTextureBrowser().getSelectedShader();

		switch (type)
		{
		case eBrushCuboid:
			// Cuboids don't need to query the number of sides
			constructBrushPrefabs(type, 0, shader);
			return;

		case eBrushPrism:
			minSides = static_cast<int>(Brush::PRISM_MIN_SIDES);
			maxSides = static_cast<int>(Brush::PRISM_MAX_SIDES);
			break;

		case eBrushCone:
			minSides = static_cast<int>(Brush::CONE_MIN_SIDES);
			maxSides = static_cast<int>(Brush::CONE_MAX_SIDES);
			break;

		case eBrushSphere:
			minSides = static_cast<int>(Brush::SPHERE_MIN_SIDES);
			maxSides = static_cast<int>(Brush::SPHERE_MAX_SIDES);
			break;
		default:
			maxSides = 9999;
		};

		ui::QuerySidesDialog dialog(minSides, maxSides);

		int sides = dialog.queryNumberOfSides();

		if (sides != -1)
		{
			constructBrushPrefabs(type, sides, shader);
		}
	}
	else
	{
		rError() << "BrushMakePrefab: invalid prefab type. Allowed types are: " << std::endl
			<< eBrushCuboid << " = cuboid " << std::endl
			<< eBrushPrism  << " = prism " << std::endl
			<< eBrushCone  << " = cone " << std::endl
			<< eBrushSphere << " = sphere " << std::endl;
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
	constructBrushPrefabs(eBrushPrism, numSides, GlobalTextureBrowser().getSelectedShader());
}

void brushSetDetailFlag(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rError() << "Usage: BrushSetDetailFlag [detail|structural]" << std::endl;
		return;
	}

	// First argument contains the number of sides
	std::string arg = boost::algorithm::to_lower_copy(args[0].getString());

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
