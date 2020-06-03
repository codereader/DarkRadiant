#pragma once

#include <sigc++/connection.h>

#include "imodule.h"
#include "iorthocontextmenu.h"
#include "icommandsystem.h"

#include "EntityClassColourManager.h"
#include "LongRunningOperationHandler.h"
#include "AutoSaveRequestHandler.h"
#include "MapFileProgressHandler.h"
#include "messages/CommandExecutionFailed.h"
#include "messages/TextureChanged.h"
#include "messages/NotificationMessage.h"

namespace ui
{

/**
 * Module responsible of registering and intialising the various
 * UI classes in DarkRadiant, e.g. the LayerSystem.
 * 
 * Currently many UI classes are spread and initialised all across
 * the main binary, so there's still work left to do.
 */
class UserInterfaceModule :
	public RegisterableModule
{
private:
	std::unique_ptr<EntityClassColourManager> _eClassColourManager;
	std::unique_ptr<LongRunningOperationHandler> _longOperationHandler;
	std::unique_ptr<MapFileProgressHandler> _mapFileProgressHandler;
	std::unique_ptr<AutoSaveRequestHandler> _autoSaveRequestHandler;

	sigc::connection _entitySettingsConn;
	sigc::connection _coloursUpdatedConn;

	std::size_t _execFailedListener;
	std::size_t _textureChangedListener;
	std::size_t _notificationListener;

public:
	// RegisterableModule
	const std::string & getName() const override;
	const StringSet & getDependencies() const override;
	void initialiseModule(const ApplicationContext & ctx) override;
	void shutdownModule() override;

private:
	void registerUICommands();
	void initialiseEntitySettings();
	void applyEntityVertexColours();
	void applyBrushVertexColours();
	void applyPatchVertexColours();
	void refreshShadersCmd(const cmd::ArgumentList& args);

	void handleCommandExecutionFailure(radiant::CommandExecutionFailedMessage& msg);
	static void HandleTextureChanged(radiant::TextureChangedMessage& msg);
	static void HandleNotificationMessage(radiant::NotificationMessage& msg);
};

}
