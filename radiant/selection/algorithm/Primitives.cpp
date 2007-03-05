#include "Primitives.h"

#include "ientity.h"
#include "brush/FaceInstance.h"
#include "brush/BrushVisit.h"
#include "patch/PatchSceneWalk.h"
#include "string/string.h"
#include "brush/export/CollisionModel.h"

// greebo: Nasty global that contains all the selected face instances
extern FaceInstanceSet g_SelectedFaceInstances;

namespace selection {
	namespace algorithm {

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
	globalOutputStream() << "CollisionModel::createFromSelection started.\n";
	
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	if (info.totalCount == info.entityCount && info.totalCount == 1) {
		// Retrieve the node, instance and entity
		scene::Instance& entityInstance = GlobalSelectionSystem().ultimateSelected();
		scene::Node& entityNode = entityInstance.path().top();
		Entity* entity = Node_getEntity(entityNode);
		
		if (entity != NULL && 
			entity->getKeyValue("classname") == cmutil::ECLASS_CLIPMODEL) 
		{
			// Try to retrieve the group node
			scene::GroupNode* groupNode = Node_getGroupNode(entityNode);
			
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
			
				globalOutputStream() << "Brushes found: " << brushes.size() << "\n";
			
				// Add all the brushes to the collision model
				for (unsigned int i = 0; i < brushes.size(); i++) {
					cm->addBrush(*brushes[i]);
				}
				
				// De-select the child brushes
				GlobalSelectionSystem().setSelectedAll(false);
				
				// Re-add the origin to the brushes
				groupNode->addOriginToChildren();
			
				// Re-select the instance
				Instance_setSelected(entityInstance, true);
			
				// Output the data of the cm (debug)
				std::cout << *cm;
			}
		}
		else {
			globalErrorStream() << "Cannot export, wrong entity (func_clipmodel required).\n";
		}
	}
	else {
		globalErrorStream() << "Cannot export, create and selecte a func_clipmodel entity.\n";
	}
}

	} // namespace algorithm
} // namespace selection
