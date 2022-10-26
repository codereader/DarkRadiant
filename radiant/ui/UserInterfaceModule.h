#pragma once

#include <sigc++/connection.h>
#include <wx/event.h>

#include "ui/iuserinterface.h"
#include "ui/iorthocontextmenu.h"
#include "icommandsystem.h"

#include "LongRunningOperationHandler.h"
#include "FileSelectionRequestHandler.h"
#include "FileOverwriteConfirmationHandler.h"
#include "FileSaveConfirmationHandler.h"
#include "AutoSaveRequestHandler.h"
#include "MapFileProgressHandler.h"
#include "ManipulatorToggle.h"
#include "SelectionModeToggle.h"
#include "statusbar/ShaderClipboardStatus.h"
#include "statusbar/EditingStopwatchStatus.h"
#include "statusbar/CommandStatus.h"
#include "statusbar/MapStatistics.h"
#include "messages/CommandExecutionFailed.h"
#include "messages/TextureChanged.h"
#include "messages/NotificationMessage.h"
#include "ui/mru/MRUMenu.h"
#include "DispatchEvent.h"
#include "mainframe/ViewMenu.h"
#include "map/AutoSaveTimer.h"
#include "textool/TexToolModeToggles.h"

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
	public wxEvtHandler,
	public IUserInterfaceModule
{
private:
    std::map<std::string, IUserControl::Ptr> _userControls;

	std::unique_ptr<LongRunningOperationHandler> _longOperationHandler;
	std::unique_ptr<MapFileProgressHandler> _mapFileProgressHandler;
	std::unique_ptr<AutoSaveRequestHandler> _autoSaveRequestHandler;
	std::unique_ptr<FileSelectionRequestHandler> _fileSelectionRequestHandler;
	std::unique_ptr<FileOverwriteConfirmationHandler> _fileOverwriteConfirmationHandler;
	std::unique_ptr<FileSaveConfirmationHandler> _fileSaveConfirmationHandler;
	std::unique_ptr<statusbar::ShaderClipboardStatus> _shaderClipboardStatus;
	std::unique_ptr<statusbar::EditingStopwatchStatus> _editStopwatchStatus;
	std::unique_ptr<statusbar::CommandStatus> _commandStatus;
	std::unique_ptr<statusbar::MapStatistics> _mapStatisticsStatus;
	std::unique_ptr<ManipulatorToggle> _manipulatorToggle;
	std::unique_ptr<SelectionModeToggle> _selectionModeToggle;
	std::unique_ptr<TexToolModeToggles> _textureToolModeToggles;

	sigc::connection _entitySettingsConn;
	sigc::connection _coloursUpdatedConn;
    sigc::connection _mapEditModeChangedConn;
    sigc::connection _reloadMaterialsConn;

	std::size_t _execFailedListener;
	std::size_t _notificationListener;

	std::unique_ptr<MRUMenu> _mruMenu;

	std::unique_ptr<map::AutoSaveTimer> _autosaveTimer;

    std::unique_ptr<ViewMenu> _viewMenu;

public:
	// RegisterableModule
	const std::string & getName() const override;
	const StringSet & getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;

	// Runs the specified action in the UI thread 
	// this happens when the application has a chance to, usually during event processing
	// This method is safe to be called from any thread.
	void dispatch(const std::function<void()>& action) override;

    void registerControl(const IUserControl::Ptr& control) override;
    IUserControl::Ptr findControl(const std::string& name) override;
    void unregisterControl(const std::string& controlName) override;
    void foreachControl(const std::function<void(const std::string&)>& functor) override;

private:
	void registerUICommands();
	void initialiseEntitySettings();
	void applyEntityVertexColours();
	void applyBrushVertexColours();
	void applyPatchVertexColours();

	void handleCommandExecutionFailure(radiant::CommandExecutionFailedMessage& msg);
	static void HandleNotificationMessage(radiant::NotificationMessage& msg);

	void onDispatchEvent(DispatchEvent& evt);
};

// Binary-internal accessor to the UI module
UserInterfaceModule& GetUserInterfaceModule();

}
