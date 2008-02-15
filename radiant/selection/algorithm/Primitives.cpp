#include "Primitives.h"

#include <fstream>

#include "igroupnode.h"
#include "ientity.h"
#include "iundo.h"
#include "itraversable.h"
#include "brush/BrushModule.h"
#include "brush/BrushInstance.h"
#include "brush/BrushVisit.h"
#include "patch/PatchSceneWalk.h"
#include "patch/Patch.h"
#include "string/string.h"
#include "brush/export/CollisionModel.h"
#include "gtkutil/dialog.h"
#include "mainframe.h"
#include "map/Map.h"
#include "ui/modelselector/ModelSelector.h"
#include "settings/GameManager.h" 

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>

// greebo: Nasty global that contains all the selected face instances
extern FaceInstanceSet g_SelectedFaceInstances;

namespace selection {
	namespace algorithm {

	namespace {
		const std::string RKEY_CM_EXT = "game/defaults/collisionModelExt";
		
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
	public scene::Traversable::Walker,
	public BrushVisitor
{
	PrimitiveVisitor& _visitor;
public:
	SelectionWalker(PrimitiveVisitor& visitor) : 
		_visitor(visitor) 
	{}
	
	// SelectionSystem::Visitor implementation
	virtual void visit(scene::Instance& instance) const {
		// Check if we have an entity
		scene::GroupNodePtr groupNode = Node_getGroupNode(instance.path().top());

		if (groupNode != NULL) {
			// We have a selected groupnode, traverse it using self as walker
			scene::TraversablePtr traversable = Node_getTraversable(instance.path().top());
			if (traversable != NULL) {
				traversable->traverse(*this);
			}
			return;
		}

		Brush* brush = Node_getBrush(instance.path().top());

		if (brush != NULL) {
			// We have a brush, visit and traverse each face
			_visitor.visit(*brush);
			brush->forEachFace(*this);
			return;
		}

		Patch* patch = Node_getPatch(instance.path().top());
		if (patch != NULL) {
			_visitor.visit(*patch);
		}
	}

	// BrushVisitor implemenatation
	virtual void visit(Face& face) const {
		_visitor.visit(face);
	}

	// scene::Traversable::Walker implemenatation
	virtual bool pre(scene::INodePtr node) const {
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

int selectedFaceCount() {
	return static_cast<int>(g_SelectedFaceInstances.size());
}

Patch& getLastSelectedPatch() {
	if (GlobalSelectionSystem().getSelectionInfo().totalCount > 0 &&
		GlobalSelectionSystem().getSelectionInfo().patchCount > 0)
	{
		// Retrieve the last selected instance
		scene::Instance& instance = GlobalSelectionSystem().ultimateSelected();
		// Try to cast it onto a patch
		PatchInstance* patchInstance = Instance_getPatch(instance);
		
		// Return or throw
		if (patchInstance != NULL) {
			return patchInstance->getPatch();
		}
		else {
			throw selection::InvalidSelectionException("No patches selected.");
		}
	}
	else {
		throw selection::InvalidSelectionException("No patches selected.");
	}
}

class SelectedPatchFinder
{
	// The target list that gets populated
	PatchPtrVector& _vector;
public:
	SelectedPatchFinder(PatchPtrVector& targetVector) :
		_vector(targetVector)
	{}
	
	void operator()(PatchInstance& patchInstance) const {
		_vector.push_back(&patchInstance.getPatch());
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
	
	void visit(scene::Instance& instance) const {
		BrushInstance* brushInstance = Instance_getBrush(instance);
		if (brushInstance != NULL) {
			_vector.push_back(&brushInstance->getBrush());
		}
	}
};

PatchPtrVector getSelectedPatches() {
	PatchPtrVector returnVector;
	
	Scene_forEachSelectedPatch(
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
	g_SelectedFaceInstances.foreach(FaceVectorPopulator(vector));
	
	return vector;
}

// Try to create a CM from the selected entity
void createCMFromSelection() {
	// Check the current selection state
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	if (info.totalCount == info.entityCount && info.totalCount == 1) {
		// Retrieve the node, instance and entity
		scene::Instance& entityInstance = GlobalSelectionSystem().ultimateSelected();
		scene::INodePtr entityNode = entityInstance.path().top();
		
		// Try to retrieve the group node
		scene::GroupNodePtr groupNode = Node_getGroupNode(entityNode);
		
		// Remove the entity origin from the brushes
		if (groupNode != NULL) {
			groupNode->removeOriginFromChildren();
			
			// Deselect the instance
			Instance_setSelected(entityInstance, false);
			
			// Select all the child nodes
			Node_getTraversable(entityNode)->traverse(
				SelectChildren(entityInstance.path())
			);
			
			BrushPtrVector brushes = algorithm::getSelectedBrushes();
		
			// Create a new collisionmodel on the heap using a shared_ptr
			cmutil::CollisionModelPtr cm(new cmutil::CollisionModel());
		
			// Add all the brushes to the collision model
			for (unsigned int i = 0; i < brushes.size(); i++) {
				cm->addBrush(*brushes[i]);
			}
			
			ui::ModelSelectorResult modelAndSkin = ui::ModelSelector::chooseModel();
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
					
					globalOutputStream() << "CollisionModel saved to " << cmPath.string().c_str() << "\n";
				}
				else {
					gtkutil::errorDialog("Couldn't save to file: " + cmPath.string(),
						 MainFrame_getWindow());
				}
			}
			catch (boost::filesystem::filesystem_error f) {
				globalErrorStream() << "CollisionModel: " << f.what() << "\n";
			}
			
			// De-select the child brushes
			GlobalSelectionSystem().setSelectedAll(false);
			
			// Re-add the origin to the brushes
			groupNode->addOriginToChildren();
		
			// Re-select the instance
			Instance_setSelected(entityInstance, true);
		}
	}
	else {
		gtkutil::errorDialog(ERRSTR_WRONG_SELECTION, MainFrame_getWindow());
	}
}

namespace {

	/** Walker class to count the number of selected brushes in the current
	 * scene.
	 */

	class CountSelectedPrimitives : public scene::Graph::Walker
	{
	  int& m_count;
	  mutable std::size_t m_depth;
	public:
	  CountSelectedPrimitives(int& count) : m_count(count), m_depth(0)
	  {
	    m_count = 0;
	  }
	  bool pre(const scene::Path& path, scene::Instance& instance) const
	  {
	    if(++m_depth != 1 && path.top()->isRoot())
	    {
	      return false;
	    }
	    Selectable* selectable = Instance_getSelectable(instance);
	    if(selectable != 0
	      && selectable->isSelected()
	      && Node_isPrimitive(path.top()))
	    {
	      ++m_count;
	    }
	    return true;
	  }
	  void post(const scene::Path& path, scene::Instance& instance) const
	  {
	    --m_depth;
	  }
	};
	
	/** greebo: Counts the selected brushes in the scenegraph
	 */
	class BrushCounter : public scene::Graph::Walker
	{
		int& _count;
		mutable std::size_t _depth;
	public:
		BrushCounter(int& count) : 
			_count(count), 
			_depth(0) 
		{
			_count = 0;
		}
		
		bool pre(const scene::Path& path, scene::Instance& instance) const {
			
			if (++_depth != 1 && path.top()->isRoot()) {
				return false;
			}
			
			Selectable* selectable = Instance_getSelectable(instance);
			if (selectable != NULL && selectable->isSelected()
			        && Node_isBrush(path.top())) 
			{
				++_count;
			}
			
			return true;
		}
		
		void post(const scene::Path& path, scene::Instance& instance) const {
			--_depth;
		}
	};

} // namespace

/* Return the number of selected primitives in the map, using the
 * CountSelectedPrimitives walker.
 */
int countSelectedPrimitives() {
	int count;
	GlobalSceneGraph().traverse(CountSelectedPrimitives(count));
	return count;
}

/* Return the number of selected brushes in the map, using the
 * CountSelectedBrushes walker.
 */
int countSelectedBrushes() {
	int count;
	GlobalSceneGraph().traverse(BrushCounter(count));
	return count;
}

class OriginRemover :
	public scene::Graph::Walker 
{
public:
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		Entity* entity = Node_getEntity(path.top());
		
		// Check for an entity
		if (entity != NULL) {
			// greebo: Check for a Doom3Group
			scene::GroupNodePtr groupNode = Node_getGroupNode(path.top());
			
			// Don't handle the worldspawn children, they're safe&sound
			if (groupNode != NULL && entity->getKeyValue("classname") != "worldspawn") {
				groupNode->removeOriginFromChildren();
				// Don't traverse the children
				return false;
			}
		}
		
		return true;
	}
};

// Graph::Walker implementation
bool OriginAdder::pre(const scene::Path& path, scene::Instance& instance) const {
	Entity* entity = Node_getEntity(path.top());
	
	// Check for an entity
	if (entity != NULL) {
		// greebo: Check for a Doom3Group
		scene::GroupNodePtr groupNode = Node_getGroupNode(path.top());
		
		// Don't handle the worldspawn children, they're safe&sound
		if (groupNode != NULL && entity->getKeyValue("classname") != "worldspawn") {
			groupNode->addOriginToChildren();
			// Don't traverse the children
			return false;
		}
	}
	
	return true;
}
	
// Traversable::Walker implementation
bool OriginAdder::pre(scene::INodePtr node) const {
	Entity* entity = Node_getEntity(node);
	
	// Check for an entity
	if (entity != NULL) {
		// greebo: Check for a Doom3Group
		scene::GroupNodePtr groupNode = Node_getGroupNode(node);
		
		// Don't handle the worldspawn children, they're safe&sound
		if (groupNode != NULL && entity->getKeyValue("classname") != "worldspawn") {
			groupNode->addOriginToChildren();
			// Don't traverse the children
			return false;
		}
	}
	return true;
}


void removeOriginFromChildPrimitives() {
	bool textureLockStatus = GlobalBrush()->textureLockEnabled();
	GlobalBrush()->setTextureLock(false);
	GlobalSceneGraph().traverse(OriginRemover());
	GlobalBrush()->setTextureLock(textureLockStatus);
}

void addOriginToChildPrimitives() {
	bool textureLockStatus = GlobalBrush()->textureLockEnabled();
	GlobalBrush()->setTextureLock(false);
	GlobalSceneGraph().traverse(OriginAdder());
	GlobalBrush()->setTextureLock(textureLockStatus);
}

void createDecalsForSelectedFaces() {
	FacePtrVector faces = getSelectedFaces();
	
	if (faces.size() <= 0) {
		gtkutil::errorDialog("No faces selected.", GlobalRadiant().getMainWindow());
		return;
	}
	
	// Create a scoped undocmd object
	UndoableCommand cmd("createDecalsForSelectedFaces");
	
	int unsuitableWindings(0);
	
	// greebo: For each face, create a patch with fixed tesselation
	for (std::size_t i = 0; i < faces.size(); i++) {
		const Winding& winding = faces[i]->getWinding();
		
		// For now, only windings with four edges are supported
		if (winding.numpoints == 4) {
			scene::INodePtr patchNode = GlobalPatchCreator(DEF3).createPatch();
			
			if (patchNode == NULL) {
				gtkutil::errorDialog("Could not create patch.", GlobalRadiant().getMainWindow());
				return;
			}
			
			Patch* patch = Node_getPatch(patchNode);
			assert(patch != NULL); // must not fail
			
			// Set the tesselation of that 3x3 patch
			patch->setDims(3,3);
			patch->setFixedSubdivisions(true, BasicVector2<unsigned int>(1,1));
			
			// Set the coordinates
			patch->ctrlAt(0,0).m_vertex = winding[0].vertex;
			patch->ctrlAt(2,0).m_vertex = winding[1].vertex;
			patch->ctrlAt(1,0).m_vertex = (patch->ctrlAt(0,0).m_vertex + patch->ctrlAt(2,0).m_vertex)/2;
			
			patch->ctrlAt(0,1).m_vertex = (winding[0].vertex + winding[3].vertex)/2;
			patch->ctrlAt(2,1).m_vertex = (winding[1].vertex + winding[2].vertex)/2;
			
			patch->ctrlAt(1,1).m_vertex = (patch->ctrlAt(0,1).m_vertex + patch->ctrlAt(2,1).m_vertex)/2;
			
			patch->ctrlAt(2,2).m_vertex = winding[2].vertex;
			patch->ctrlAt(0,2).m_vertex = winding[3].vertex;
			patch->ctrlAt(1,2).m_vertex = (patch->ctrlAt(2,2).m_vertex + patch->ctrlAt(0,2).m_vertex)/2;
			
			// Set the texture coordinates to something useful
			patch->NaturalTexture();
			
			// Insert the patch into worldspawn
			scene::INodePtr worldSpawnNode = GlobalMap().getWorldspawn();
			assert(worldSpawnNode != NULL); // This must be non-NULL, otherwise we won't have faces
			
			scene::TraversablePtr traversable = Node_getTraversable(worldSpawnNode);
			traversable->insert(patchNode);
		}
	}
	
	if (unsuitableWindings > 0) {
		gtkutil::errorDialog(
			intToStr(unsuitableWindings) + " faces were not suitable (had more than 4 edges).", 
			GlobalRadiant().getMainWindow()
		);
	}
}

	} // namespace algorithm
} // namespace selection
