#ifndef PATCHCREATOR_H_
#define PATCHCREATOR_H_

#include "ipatch.h"
#include "ifilter.h"
#include "ipreferencesystem.h"
#include "itextstream.h"

namespace {
	const std::string RKEY_PATCH_SUBDIVIDE_THRESHOLD = "user/ui/patch/subdivideThreshold";
}

/* greebo: The Doom3PatchCreator implements the method createPatch(),
 * as required by the abstract base class PatchCreator (see ipatch.h). 
 * The nodes are allocated on the heap and can be released by calling 
 * the PatchNode::release() method (deletes itself).
 */
class Doom3PatchCreator : 
	public PatchCreator
{
public:
	// PatchCreator implementation
	scene::INodePtr createPatch()
	{
		// Note the true as function argument: this means that patchDef3 = true in the PatchNode constructor.  
		return scene::INodePtr(new PatchNode(true));
	}
	
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name(MODULE_PATCH + DEF3);
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;
		
		if (_dependencies.empty()) {
			_dependencies.insert(MODULE_RENDERSYSTEM);
			_dependencies.insert(MODULE_PREFERENCESYSTEM);
		}
		
		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "Doom3PatchDef3Creator::initialiseModule called." << std::endl;
		
		// Construct and Register the patch-related preferences
		PreferencesPagePtr page = GlobalPreferenceSystem().getPage("Settings/Patch");
		page->appendEntry("Patch Subdivide Threshold", RKEY_PATCH_SUBDIVIDE_THRESHOLD);

		// Initialise the static member variables of the Patch and PatchNode classes
		Patch::constructStatic();
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
	public PatchCreator
{
public:
	// PatchCreator implementation
	scene::INodePtr createPatch()
	{
		// The PatchNodeDoom3 constructor normally expects a bool, which defaults to false.
		// this means that the patch is node def3, but def2
		return scene::INodePtr(new PatchNode());
	}
	
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name(MODULE_PATCH + DEF2);
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;
		
		if (_dependencies.empty()) {
			_dependencies.insert(MODULE_RENDERSYSTEM);
			_dependencies.insert(MODULE_PREFERENCESYSTEM);
		}
		
		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "Doom3PatchDef2Creator::initialiseModule called." << std::endl;
	}
};

#endif /*PATCHCREATOR_H_*/
