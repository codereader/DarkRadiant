#ifndef PATCHCREATOR_H_
#define PATCHCREATOR_H_

#include "ipatch.h"
#include "ifilter.h"
#include "ipreferencesystem.h"

namespace {
	const std::string RKEY_PATCH_SUBDIVIDE_THRESHOLD = "user/ui/patch/subdivideThreshold";
}

/* greebo: This is the common patch creator class (for non-Doom3 patches), the base implementation
 * for the more specific patch creators (like Doom3PatchCreator).
 * 
 * Note: the implementation of createPatch() method is missing, this is implemented in Doom3PatchCreator,
 * so it will not be possible to create an actual instance of this very class.
 * 
 * Note: The abstract base class PatchCreator is defined in ipatch.h.  
 */ 
class CommonPatchCreator : public PatchCreator {
public:
	// Save the current state of the patch.
	void Patch_undoSave(scene::INodePtr patch) const {
		// greebo: Get the patch member variable from the node and call its undoSave() function
		// This creates an UndoMemento class where all the state-relevant variables of the patch are saved.  
		Node_getPatch(patch)->undoSave();
	}
	
	// Resize the patch (width and height of the control instances, e.g. 3x3 patch)
	void Patch_resize(scene::INodePtr patch, std::size_t width, std::size_t height) const {
		// Pass this call to the patch member variable of this node
		Node_getPatch(patch)->setDims(width, height);
	}
	
	// returns a PatchControlMatrix with width, height and the control vertices itself.
	PatchControlMatrix Patch_getControlPoints(scene::INodePtr node) const {
		// Retrieve a reference to the patch member variable of this node
		Patch& patch = *Node_getPatch(node);
		// Create a PatchControlMatrix with <height> and <width> and the list of control points.
		return PatchControlMatrix(patch.getHeight(), patch.getWidth(), patch.getControlPoints().data());
	}
	
	// Notify the patch, that the control points have changed, so that the patch class can update itself accordingly
	void Patch_controlPointsChanged(scene::INodePtr patch) const {
		return Node_getPatch(patch)->controlPointsChanged();
	}
	
	// Get the shadername of the patch stored in this node 
	const std::string& Patch_getShader(scene::INodePtr patch) const {
		return Node_getPatch(patch)->GetShader();
	}
  	
  	// Set the shadername of the patch in this node
	void Patch_setShader(scene::INodePtr patch, const std::string& shader) const {
		Node_getPatch(patch)->SetShader(shader);
	}
}; // class CommonPatchCreator

// ----------------------------------------------------------------------------------------------

/* greebo: The Doom3PatchCreator inherits all implementations from CommonPatchCreator and adds the method createPatch(),
 * as required by the abstract base class PatchCreator (see ipatch.h). The nodes are allocated on the heap and can be 
 * released by calling the PatchNode::release() method (deletes itself).
 */
class Doom3PatchCreator : 
	public CommonPatchCreator
{
public:
	scene::INodePtr createPatch() {
		// Note the true as function argument: this means that patchDef3 = true in the PatchNode constructor.  
		scene::INodePtr node(new PatchNode(true));
		node->setSelf(node);
		return node;
	}
	
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name(MODULE_PATCH + DEF3);
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;
		
		if (_dependencies.empty()) {
			_dependencies.insert(MODULE_SHADERCACHE);
			_dependencies.insert(MODULE_PREFERENCESYSTEM);
		}
		
		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "Doom3PatchDef3Creator::initialiseModule called.\n";
		
		// Construct and Register the patch-related preferences
		PreferencesPagePtr page = GlobalPreferenceSystem().getPage("Settings/Patch");
		page->appendEntry("Patch Subdivide Threshold", RKEY_PATCH_SUBDIVIDE_THRESHOLD);

		// Initialise the static member variables of the Patch and PatchNode classes
		Patch::constructStatic(ePatchTypeDoom3);
		PatchNode::constructStatic();
	}
	
	virtual void shutdownModule() {
		// Release the static member variables of the classes Patch and PatchNode 
		Patch::destroyStatic();
		PatchNode::destroyStatic();
	}
};

/* greebo: This is the same as the above, but makes sure that a patchDef2 node is created. 
 */
class Doom3PatchDef2Creator : 
	public CommonPatchCreator
{
public:
	scene::INodePtr createPatch() {
		// The PatchNodeDoom3 constructor normally expects a bool, which defaults to false.
		// this means that the patch is node def3, but def2
		scene::INodePtr node(new PatchNode());
		node->setSelf(node);
		return node;
	}
	
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name(MODULE_PATCH + DEF2);
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;
		
		if (_dependencies.empty()) {
			_dependencies.insert(MODULE_SHADERCACHE);
			_dependencies.insert(MODULE_PREFERENCESYSTEM);
		}
		
		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "Doom3PatchDef2Creator::initialiseModule called.\n";
	}
};

#endif /*PATCHCREATOR_H_*/
