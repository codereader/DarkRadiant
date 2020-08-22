#pragma once

#include <sigc++/connection.h>

#include "iregistry.h"
#include "imodule.h"

#include "brush/TexDef.h"
#include "ibrush.h"
#include "BrushSettings.h"

namespace brush
{

class BrushModuleImpl :
	public BrushCreator
{
private:
	bool _textureLockEnabled;

	std::unique_ptr<BrushSettings> _settings;

	sigc::connection _brushFaceShaderChanged;
	sigc::connection _faceTexDefChanged;

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
	scene::INodePtr createBrush() override;

	IBrushSettings& getSettings() override;

	// ----------------------------------------------------------------------------------

	// returns true if the texture lock is enabled
	bool textureLockEnabled() const;
	void setTextureLock(bool enabled);

	// Switches the texture lock on/off
	void toggleTextureLock();

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const IApplicationContext& ctx);
	virtual void shutdownModule();

}; // class BrushModuleImpl

}

// The accessor function declaration
brush::BrushModuleImpl& GlobalBrush();
