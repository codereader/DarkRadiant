#include "Primitives.h"

#include <fstream>

#include "ientity.h"
#include "brush/FaceInstance.h"
#include "brush/BrushVisit.h"
#include "patch/PatchSceneWalk.h"
#include "string/string.h"
#include "brush/export/CollisionModel.h"
#include "gtkutil/dialog.h"
#include "mainframe.h"
#include "ui/modelselector/ModelSelector.h"
#include "qe3.h"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>

// greebo: Nasty global that contains all the selected face instances
extern FaceInstanceSet g_SelectedFaceInstances;

namespace selection {
	namespace algorithm {

	namespace {
		const std::string RKEY_CM_EXT = "game/defaults/collisionModelExt";
		// Filesystem path typedef
		typedef boost::filesystem::path Path;
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
			
				// Add all the brushes to the collision model
				for (unsigned int i = 0; i < brushes.size(); i++) {
					cm->addBrush(*brushes[i]);
				}
				
				ui::ModelAndSkin modelAndSkin = ui::ModelSelector::chooseModel(); 
				std::string basePath = g_qeglobals.m_userGamePath.c_str();
				
				std::string modelPath = basePath + modelAndSkin.model;
				
				std::string newExtension = "." + GlobalRegistry().get(RKEY_CM_EXT);
				
				try {
					// create the new autosave filename by changing the extension
					Path cmPath = boost::filesystem::change_extension(
							Path(modelPath, boost::filesystem::native), 
							newExtension
						);
					
					globalOutputStream() << "Saving CollisionModel to " << cmPath.string().c_str() << "\n";
				
					// Open the stream to the output file
					std::ofstream outfile(cmPath.string().c_str());
					
					if (outfile.is_open()) {
						// Insert the CollisionModel into the stream
						outfile << *cm;
						// Close the file
						outfile.close();
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
			gtkutil::errorDialog("Can't export, wrong entity (func_clipmodel required).",
							 MainFrame_getWindow());
		}
	}
	else {
		gtkutil::errorDialog("Can't export, create and selecte a func_clipmodel entity.",
							 MainFrame_getWindow());
	}
}

	} // namespace algorithm
} // namespace selection
