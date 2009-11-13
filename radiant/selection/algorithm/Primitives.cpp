#include "Primitives.h"

#include <fstream>

#include "igroupnode.h"
#include "ientity.h"
#include "iundo.h"
#include "itraversable.h"
#include "brush/BrushModule.h"
#include "brush/BrushVisit.h"
#include "patch/PatchSceneWalk.h"
#include "patch/PatchNode.h"
#include "string/string.h"
#include "brush/export/CollisionModel.h"
#include "gtkutil/dialog.h"
#include "map/Map.h"
#include "ui/modelselector/ModelSelector.h"
#include "settings/GameManager.h" 
#include "selection/shaderclipboard/ShaderClipboard.h"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>

// greebo: Nasty global that contains all the selected face instances
extern FaceInstanceSet g_SelectedFaceInstances;

namespace selection {
	namespace algorithm {

	namespace {
		const std::string RKEY_CM_EXT = "game/defaults/collisionModelExt";
		const std::string RKEY_NODRAW_SHADER = "game/defaults/nodrawShader";
		const std::string RKEY_VISPORTAL_SHADER = "game/defaults/visportalShader";
		
		const std::string ERRSTR_WRONG_SELECTION = 
				"Can't export, create and select a func_* entity\
				 containing the collision hull primitives.";
		
		// Filesystem path typedef
		typedef boost::filesystem::path Path;
	}

/** 
 * greebo: Traverses the selection and invokes the PrimitiveVisitor on
 *         each encountered primitive. This class implements several 
 *         interfaces to avoid having multiple walker classes.
 *
 * The SelectionWalker traverses the currently selected instances and
 * passes Brushes and Patches right to the PrimitiveVisitor. When
 * GroupNodes are encountered, the GroupNode itself is traversed 
 * and all child primitives are passed to the PrimitiveVisitor as well.
 */
class SelectionWalker :
	public SelectionSystem::Visitor,
	public scene::NodeVisitor,
	public BrushVisitor
{
	PrimitiveVisitor& _visitor;
public:
	SelectionWalker(PrimitiveVisitor& visitor) : 
		_visitor(visitor) 
	{}
	
	// SelectionSystem::Visitor implementation
	virtual void visit(const scene::INodePtr& node) const {
		// Check if we have an entity
		scene::GroupNodePtr groupNode = Node_getGroupNode(node);

		if (groupNode != NULL) {
			// We have a selected groupnode, traverse it using self as walker
			const scene::NodeVisitor& visitor = *this;
			node->traverse(const_cast<scene::NodeVisitor&>(visitor));
			return;
		}

		Brush* brush = Node_getBrush(node);

		if (brush != NULL) {
			// We have a brush, visit and traverse each face
			_visitor.visit(*brush);
			brush->forEachFace(*this);
			return;
		}

		Patch* patch = Node_getPatch(node);
		if (patch != NULL) {
			_visitor.visit(*patch);
		}
	}

	// BrushVisitor implemenatation
	virtual void visit(Face& face) const {
		_visitor.visit(face);
	}

	// NodeVisitor implemenatation
	virtual bool pre(const scene::INodePtr& node) {
		Brush* brush = Node_getBrush(node);

		if (brush != NULL) {
			// We have a brush, visit and traverse each face
			_visitor.visit(*brush);
			brush->forEachFace(*this);
			return false;
		}

		Patch* patch = Node_getPatch(node);
		if (patch != NULL) {
			_visitor.visit(*patch);
			return false;
		}

		return true; // traverse further
	}

	// Functor for the selected face instances
	void operator() (FaceInstance& faceInstance) {
		// Pass the call to the visit function
		visit(faceInstance.getFace());
	}
};

void forEachSelectedPrimitive(PrimitiveVisitor& visitor) {
	// First walk all selected instances
	SelectionWalker walker(visitor);

	if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent) {
		// We are not in component mode, so let's walk the actual scene::Instances
		GlobalSelectionSystem().foreachSelected(walker);
	}

	// Now traverse the selected face instances
	g_SelectedFaceInstances.foreach(walker);
}

// PatchTesselationUpdater implementation
void PatchTesselationUpdater::visit(const scene::INodePtr& node) const {
	Patch* patch = Node_getPatch(node);

	if (patch != NULL) {
		patch->setFixedSubdivisions(_fixed, _tess);
	}
}

int selectedFaceCount() {
	return static_cast<int>(g_SelectedFaceInstances.size());
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
			throw selection::InvalidSelectionException("No patches selected.");
		}
	}
	else {
		throw selection::InvalidSelectionException("No patches selected.");
	}
}

class SelectedPatchFinder :
	public SelectionSystem::Visitor
{
	// The target list that gets populated
	PatchPtrVector& _vector;
public:
	SelectedPatchFinder(PatchPtrVector& targetVector) :
		_vector(targetVector)
	{}
	
	void visit(const scene::INodePtr& node) const {
		PatchNodePtr patchNode = boost::dynamic_pointer_cast<PatchNode>(node);
		if (patchNode != NULL) {
			_vector.push_back(patchNode);
		}
	}
};

class SelectedBrushFinder :
	public SelectionSystem::Visitor
{
	// The target list that gets populated
	BrushPtrVector& _vector;
public:
	SelectedBrushFinder(BrushPtrVector& targetVector) :
		_vector(targetVector)
	{}
	
	void visit(const scene::INodePtr& node) const {
		BrushNodePtr brushNode = boost::dynamic_pointer_cast<BrushNode>(node);
		if (brushNode != NULL) {
			_vector.push_back(brushNode);
		}
	}
};

PatchPtrVector getSelectedPatches() {
	PatchPtrVector returnVector;
	
	GlobalSelectionSystem().foreachSelected(
		SelectedPatchFinder(returnVector)
	);
	
	return returnVector;
}

BrushPtrVector getSelectedBrushes() {
	BrushPtrVector returnVector;
	
	GlobalSelectionSystem().foreachSelected(
		SelectedBrushFinder(returnVector)
	);
	
	return returnVector;
}

Face& getLastSelectedFace() {
	if (selectedFaceCount() == 1) {
		return g_SelectedFaceInstances.last().getFace();
	}
	else {
		throw selection::InvalidSelectionException(intToStr(selectedFaceCount()));
	}
}

class FaceVectorPopulator
{
	// The target list that gets populated
	FacePtrVector& _vector;
public:
	FaceVectorPopulator(FacePtrVector& targetVector) :
		_vector(targetVector)
	{}
	
	void operator() (FaceInstance& faceInstance) {
		_vector.push_back(&faceInstance.getFace());
	}
};

FacePtrVector getSelectedFaces() {
	FacePtrVector vector;
	
	// Cycle through all selected faces and fill the vector 
	FaceVectorPopulator populator(vector);
	g_SelectedFaceInstances.foreach(populator);
	
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
			NodeSelector visitor;
			entityNode->traverse(visitor);
			
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
			
			std::string newExtension = "." + GlobalRegistry().get(RKEY_CM_EXT);
			
			// Set the model string to correctly associate the clipmodel
			cm->setModel(modelAndSkin.model);
			
			try {
				// create the new autosave filename by changing the extension
				Path cmPath = boost::filesystem::change_extension(
						Path(modelPath, boost::filesystem::native), 
						newExtension
					);
				
				// Open the stream to the output file
				std::ofstream outfile(cmPath.string().c_str());
				
				if (outfile.is_open()) {
					// Insert the CollisionModel into the stream
					outfile << *cm;
					// Close the file
					outfile.close();
					
					globalOutputStream() << "CollisionModel saved to " << cmPath.string() << "\n";
				}
				else {
					gtkutil::errorDialog("Couldn't save to file: " + cmPath.string(),
						 GlobalRadiant().getMainWindow());
				}
			}
			catch (boost::filesystem::filesystem_error f) {
				globalErrorStream() << "CollisionModel: " << f.what() << "\n";
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
		gtkutil::errorDialog(ERRSTR_WRONG_SELECTION, GlobalRadiant().getMainWindow());
	}
}

namespace {

	/** Walker class to count the number of selected brushes in the current
	 * scene.
	 */

	class CountSelectedPrimitives : 
		public scene::NodeVisitor
	{
		int _count;
		std::size_t _depth;
	public:
		CountSelectedPrimitives() : 
			_count(0), 
			_depth(0)
		{}

		bool pre(const scene::INodePtr& node) 
		{
			if (++_depth != 1 && node->isRoot())
			{
				return false;
			}
		
			if (Node_isSelected(node) && Node_isPrimitive(node)) {
				++_count;
			}
	    
			return true;
		}

		void post(const scene::INodePtr& node)
		{
			--_depth;
		}

		int getCount() const
		{
			return _count;
		}
	};
	
	/** greebo: Counts the selected brushes in the scenegraph
	 */
	class BrushCounter : 
		public scene::NodeVisitor
	{
		int _count;
		std::size_t _depth;
	public:
		BrushCounter() : 
			_count(0), 
			_depth(0)
		{}
		
		bool pre(const scene::INodePtr& node)
		{
			if (++_depth != 1 && node->isRoot())
			{
				return false;
			}
			
			if (Node_isSelected(node) && Node_isBrush(node)) 
			{
				++_count;
			}
			
			return true;
		}
		
		void post(const scene::INodePtr& node)
		{
			--_depth;
		}

		int getCount() const
		{
			return _count;
		}
	};

} // namespace

/* Return the number of selected primitives in the map, using the
 * CountSelectedPrimitives walker.
 */
int countSelectedPrimitives()
{
	CountSelectedPrimitives counter;
	Node_traverseSubgraph(GlobalSceneGraph().root(), counter);
	return counter.getCount();
}

/* Return the number of selected brushes in the map, using the
 * CountSelectedBrushes walker.
 */
int countSelectedBrushes()
{
	BrushCounter counter;
	Node_traverseSubgraph(GlobalSceneGraph().root(), counter);
	return counter.getCount();
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
				gtkutil::errorDialog("Could not create patch.", GlobalRadiant().getMainWindow());
				return;
			}
			
			Patch* patch = Node_getPatch(patchNode);
			assert(patch != NULL); // must not fail
			
			// Set the tesselation of that 3x3 patch
			patch->setDims(3,3);
			patch->setFixedSubdivisions(true, Subdivisions(1,1));
			
			// Set the coordinates
			patch->ctrlAt(0,0).vertex = winding[0].vertex;
			patch->ctrlAt(2,0).vertex = winding[1].vertex;
			patch->ctrlAt(1,0).vertex = (patch->ctrlAt(0,0).vertex + patch->ctrlAt(2,0).vertex)/2;
			
			patch->ctrlAt(0,1).vertex = (winding[0].vertex + winding[3].vertex)/2;
			patch->ctrlAt(2,1).vertex = (winding[1].vertex + winding[2].vertex)/2;
			
			patch->ctrlAt(1,1).vertex = (patch->ctrlAt(0,1).vertex + patch->ctrlAt(2,1).vertex)/2;
			
			patch->ctrlAt(2,2).vertex = winding[2].vertex;
			patch->ctrlAt(0,2).vertex = winding[3].vertex;
			patch->ctrlAt(1,2).vertex = (patch->ctrlAt(2,2).vertex + patch->ctrlAt(0,2).vertex)/2;

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

	void operator() (FaceInstance& faceInstance) {
		// Get the winding
		const Winding& winding = faceInstance.getFace().getWinding();

		// For now, only windings with four edges are supported
		if (winding.numpoints == 4) {
			_faceInstances.push_back(&faceInstance);
		}
		else {
			_unsuitableWindings++;
		}
	}

	int getNumUnsuitableWindings() const {
		return _unsuitableWindings;
	}
};

void createDecalsForSelectedFaces(const cmd::ArgumentList& args) {
	// Sanity check	
	if (g_SelectedFaceInstances.empty()) {
		gtkutil::errorDialog("No faces selected.", GlobalRadiant().getMainWindow());
		return;
	}
	
	// Create a scoped undocmd object
	UndoableCommand cmd("createDecalsForSelectedFaces");
	
	// greebo: For each face, create a patch with fixed tesselation
	DecalPatchCreator creator;
	g_SelectedFaceInstances.foreach(creator);

	// Issue the command
	creator.createDecals();
	
	// Check how many faces were not suitable
	int unsuitableWindings = creator.getNumUnsuitableWindings();

	if (unsuitableWindings > 0) {
		gtkutil::errorDialog(
			intToStr(unsuitableWindings) + " faces were not suitable (had more than 4 vertices).", 
			GlobalRadiant().getMainWindow()
		);
	}
}

class LargestFaceFinder :
	public BrushVisitor
{
	mutable float _largestArea;
	mutable Face* _largestFace;
public:
	LargestFaceFinder() :
		_largestArea(0),
		_largestFace(NULL)
	{}

	void visit(Face& face) const {
		if (_largestFace == NULL) {
			_largestFace = &face;
		}

		// Calculate face area
		float area = 0;
		Winding& winding = face.getWinding();
		const Vector3& centroid = face.centroid();

		for (std::size_t i = 0; i < winding.size(); i++) {
			Vector3 edge1 = centroid - winding[i].vertex;
			Vector3 edge2 = centroid - winding[(i+1) % winding.size()].vertex;
			area += edge1.crossProduct(edge2).getLength() * 0.5f;
		}

		if (area > _largestArea) {
			_largestArea = area;
			_largestFace = &face;
		}
	}

	Face& getLargestFace() {
		return *_largestFace;
	}
};

void makeVisportal(const cmd::ArgumentList& args) {
	BrushPtrVector brushes = getSelectedBrushes();

	if (brushes.size() <= 0) {
		gtkutil::errorDialog("No brushes selected.", GlobalRadiant().getMainWindow());
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
		brush.setShader(GlobalRegistry().get(RKEY_NODRAW_SHADER));

		// Find the largest face (in terms of area)
		LargestFaceFinder finder;
		brush.forEachFace(finder);
		
		finder.getLargestFace().SetShader(GlobalRegistry().get(RKEY_VISPORTAL_SHADER));
	}
}

	} // namespace algorithm
} // namespace selection
