#ifndef SELECTIONSYSTEMMODULE_CPP_
#define SELECTIONSYSTEMMODULE_CPP_

#include "igl.h"
#include "iscenegraph.h"
#include "irender.h"
#include "igrid.h"

// Module stuff
#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"
#include <boost/shared_ptr.hpp>

#include "selection/RadiantSelectionSystem.h"

namespace selection {

class SelectionDependencies :
	public GlobalSceneGraphModuleRef,
	public GlobalShaderCacheModuleRef,
	public GlobalOpenGLModuleRef,
	public GlobalGridModuleRef
{};

/* greebo: This is the class that handles the selectionSystem instantiation
 * The constructor allocates a RadiantSelectionSystem instance on the heap
 * which is freed again by the destructor 
 */
class SelectionAPI
{
	typedef boost::shared_ptr<RadiantSelectionSystem> SelectionSystemPtr;
	SelectionSystemPtr _selectionSystem;
	
	SignalHandlerId _boundsChangedHandler;
public:
	typedef SelectionSystem Type;
	STRING_CONSTANT(Name, "*");

	// Constructor
	SelectionAPI() {
		RadiantSelectionSystem::constructStatic();
		
		// allocate a new selection system instance on the heap 
		_selectionSystem = SelectionSystemPtr(new RadiantSelectionSystem);
	
		// Connect the bounds changed caller 
		_boundsChangedHandler =	GlobalSceneGraph().addBoundsChangedCallback(
			MemberCaller<SelectionAPI, &SelectionAPI::onBoundsChanged>(*this)
		);
	
		GlobalShaderCache().attachRenderable(*_selectionSystem);
	}
	
	void onBoundsChanged() {
		if (_selectionSystem != NULL) {
			_selectionSystem->pivotChanged();
		}
	}
	
	~SelectionAPI() {
		GlobalShaderCache().detachRenderable(*_selectionSystem);
		GlobalSceneGraph().removeBoundsChangedCallback(_boundsChangedHandler);
		
		// release the selection system from memory
		_selectionSystem = SelectionSystemPtr();
	
		RadiantSelectionSystem::destroyStatic();
	}
  
	SelectionSystem* getTable() {
		// Return the raw pointer contained within the shared_ptr
		return _selectionSystem.get();
	}
};

typedef SingletonModule<SelectionAPI, SelectionDependencies> SelectionModule;
typedef Static<SelectionModule> StaticSelectionModule;
StaticRegisterModule staticRegisterSelection(StaticSelectionModule::instance());

} // namespace selection

#endif /*SELECTIONSYSTEMMODULE_CPP_*/
