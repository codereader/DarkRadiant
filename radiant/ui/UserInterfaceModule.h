#pragma once

#include <sigc++/connection.h>
#include <wx/event.h>

#include "imodule.h"
#include "iorthocontextmenu.h"
#include "icommandsystem.h"

#include "EntityClassColourManager.h"
#include "LongRunningOperationHandler.h"
#include "FileSelectionRequestHandler.h"
#include "AutoSaveRequestHandler.h"
#include "MapFileProgressHandler.h"
#include "ManipulatorToggle.h"
#include "shaderclipboard/ShaderClipboardStatus.h"
#include "statusbar/EditingStopwatchStatus.h"
#include "messages/CommandExecutionFailed.h"
#include "messages/TextureChanged.h"
#include "messages/NotificationMessage.h"
#include "ui/mru/MRUMenu.h"
#include "DispatchEvent.h"

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
	public RegisterableModule,
	public wxEvtHandler
{
private:
	std::unique_ptr<EntityClassColourManager> _eClassColourManager;
	std::unique_ptr<LongRunningOperationHandler> _longOperationHandler;
	std::unique_ptr<MapFileProgressHandler> _mapFileProgressHandler;
	std::unique_ptr<AutoSaveRequestHandler> _autoSaveRequestHandler;
	std::unique_ptr<FileSelectionRequestHandler> _fileSelectionRequestHandler;
	std::unique_ptr<ShaderClipboardStatus> _shaderClipboardStatus;
	std::unique_ptr<EditingStopwatchStatus> _editStopwatchStatus;
	std::unique_ptr<ManipulatorToggle> _manipulatorToggle;

	sigc::connection _entitySettingsConn;
	sigc::connection _coloursUpdatedConn;

	std::size_t _execFailedListener;
	std::size_t _textureChangedListener;
	std::size_t _notificationListener;

	std::unique_ptr<MRUMenu> _mruMenu;

public:
	// RegisterableModule
	const std::string & getName() const override;
	const StringSet & getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;

	// Runs the specified action in the UI thread 
	// this happens when the application has a chance to, usually during event processing
	// This method is safe to be called from any thread.
	void dispatch(const std::function<void()>& action);

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

	void onDispatchEvent(DispatchEvent& evt);
};

// Binary-internal accessor to the UI module
UserInterfaceModule& GetUserInterfaceModule();

}
