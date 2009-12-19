#ifndef IRADIANT_H_
#define IRADIANT_H_

#include "imodule.h"
#include "imodelpreview.h"
#include <boost/weak_ptr.hpp>

/** greebo: An EventListener gets notified by the Radiant module
 *          on global events like shutdown, startup and such.
 * 
 *          EventListener classes must register themselves using
 *          the GlobalRadiant().addEventListener() method in order
 *          to get notified about the events.
 * 
 * Note: Default implementations are empty, deriving classes are
 *       supposed to pick the events they want to listen to.
 */
class RadiantEventListener {
public:
    /** Destructor
	 */
	virtual ~RadiantEventListener() {}

	/** This gets called AFTER the MainFrame window has been constructed.
	 */
	virtual void onRadiantStartup() {}
	
	/** Gets called when BEFORE the MainFrame window is destroyed.
	 *  Note: After this call, the EventListeners are deregistered from the
	 *        Radiant module, all the internally held shared_ptrs are cleared.
	 */
	virtual void onRadiantShutdown() {}
};
typedef boost::shared_ptr<RadiantEventListener> RadiantEventListenerPtr;
typedef boost::weak_ptr<RadiantEventListener> RadiantEventListenerWeakPtr;

const std::string MODULE_RADIANT("Radiant");

/** greebo: This abstract class defines the interface to the core application.
 * 			Use this to access methods from the main codebase in radiant/
 */
class IRadiant :
	public RegisterableModule
{
public:
	// Creates a new model preview (GL view with draggable viewpoint, zoom and filter functionality)
	virtual ui::IModelPreviewPtr createModelPreview() = 0;

	// Registers/de-registers an event listener class
	virtual void addEventListener(RadiantEventListenerPtr listener) = 0;
	virtual void removeEventListener(RadiantEventListenerPtr listener) = 0;
};

inline IRadiant& GlobalRadiant() {
	// Cache the reference locally
	static IRadiant& _radiant(
		*boost::static_pointer_cast<IRadiant>(
			module::GlobalModuleRegistry().getModule(MODULE_RADIANT)
		)
	);
	return _radiant;
}

#endif
