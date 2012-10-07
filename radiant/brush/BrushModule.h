#pragma once

#include "iregistry.h"
#include "imodule.h"

#include "brush/TexDef.h"
#include "ibrush.h"

class BrushModuleImpl : 
	public BrushCreator
{
private:
	bool _textureLockEnabled;

private:
	void keyChanged();

	void registerBrushCommands();

public:
    // destructor
	virtual ~BrushModuleImpl() {}

	// This constructs the brush preferences, initialises static variables, etc.
	void construct();

	// The opposite of the above construct()
	void destroy();

	// Adds the preference settings to the prefdialog
	void constructPreferences();

	// --------------- BrushCreator methods ---------------------------------------------

	// Creates a new brush node on the heap and returns it
	scene::INodePtr createBrush();

	// ----------------------------------------------------------------------------------

	// returns true if the texture lock is enabled
	bool textureLockEnabled() const;
	void setTextureLock(bool enabled);

	// Switches the texture lock on/off
	void toggleTextureLock();

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();

}; // class BrushModuleImpl

// The accessor function declaration
BrushModuleImpl& GlobalBrush();
