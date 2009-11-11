#ifndef IMAINFRAME_LAYOUT_H_
#define IMAINFRAME_LAYOUT_H_

#include "imodule.h"
#include <boost/function.hpp>

class IMainFrameLayout
{
public:
    /**
	 * Destructor
	 */
	virtual ~IMainFrameLayout() {}

	/**
	 * Each MainFrame layout has a unique name.
	 */
	virtual std::string getName() = 0;

	/**
	 * Use this to let the Layout construct its widgets and
	 * restore its state from the Registry, if appropriate.
	 */
	virtual void activate() = 0;

	/**
	 * Advises this layout to destruct its widgets and remove itself
	 * from the MainFrame.
	 */
	virtual void deactivate() = 0;
};
typedef boost::shared_ptr<IMainFrameLayout> IMainFrameLayoutPtr;

/**
 * This represents a function to create a mainframe layout like this:
 *
 * IMainFrameLayoutPtr createInstance();
 */
typedef boost::function<IMainFrameLayoutPtr()> CreateMainFrameLayoutFunc;

const std::string MODULE_MAINFRAME_LAYOUT_MANAGER("MainFrameLayoutManager");

class IMainFrameLayoutManager :
	public RegisterableModule
{
public:
	/**
	 * Retrieves a layout with the given name. Returns NULL if not found.
	 */
	virtual IMainFrameLayoutPtr getLayout(const std::string& name) = 0;

	/**
	 * Register a layout by passing a name and a function to create such a layout.
 	 */
	virtual void registerLayout(const std::string& name, const CreateMainFrameLayoutFunc& func) = 0;

	/**
	 * greebo: Registers all layout commands to the eventmanager.
	 */
	virtual void registerCommands() = 0;
};

// This is the accessor for the mainframe module
inline IMainFrameLayoutManager& GlobalMainFrameLayoutManager() {
	// Cache the reference locally
	static IMainFrameLayoutManager& _mainFrameLayoutManager(
		*boost::static_pointer_cast<IMainFrameLayoutManager>(
			module::GlobalModuleRegistry().getModule(MODULE_MAINFRAME_LAYOUT_MANAGER)
		)
	);
	return _mainFrameLayoutManager;
}

#endif /* IMAINFRAME_LAYOUT_H_ */
