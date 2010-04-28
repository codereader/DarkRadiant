#ifndef PATCHCREATOR_H_
#define PATCHCREATOR_H_

#include "i18n.h"
#include "ipatch.h"
#include "ifilter.h"
#include "ilayer.h"
#include "ipreferencesystem.h"
#include "itextstream.h"

namespace
{
	const char* const RKEY_PATCH_SUBDIVIDE_THRESHOLD = "user/ui/patch/subdivideThreshold";
}

/**
 * greebo: The Doom3PatchCreator implements the method createPatch(),
 * as required by the abstract base class PatchCreator (see ipatch.h). 
 */
class Doom3PatchCreator : 
	public PatchCreator
{
public:
	// PatchCreator implementation
	scene::INodePtr createPatch()
	{
		// Note the true as function argument: 
		// this means that patchDef3 = true in the PatchNode constructor.  
		scene::INodePtr node(new PatchNode(true));

		// Determine the first visible layer
		int layer = GlobalLayerSystem().getFirstVisibleLayer();

		if (layer != -1)
		{	
			// Move it to the first visible layer
			node->moveToLayer(layer);
		}
		
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
			_dependencies.insert(MODULE_RENDERSYSTEM);
			_dependencies.insert(MODULE_PREFERENCESYSTEM);
		}
		
		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "Doom3PatchDef3Creator::initialiseModule called." << std::endl;
		
		// Construct and Register the patch-related preferences
		PreferencesPagePtr page = GlobalPreferenceSystem().getPage(_("Settings/Patch"));
		page->appendEntry(_("Patch Subdivide Threshold"), RKEY_PATCH_SUBDIVIDE_THRESHOLD);

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
		// The PatchNodeDoom3 constructor takes false == patchDef2
		scene::INodePtr node(new PatchNode(false));

		// Determine the first visible layer
		int layer = GlobalLayerSystem().getFirstVisibleLayer();

		if (layer != -1)
		{	
			// Move it to the first visible layer
			node->moveToLayer(layer);
		}
		
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
