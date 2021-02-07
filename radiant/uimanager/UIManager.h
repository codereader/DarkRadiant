#pragma once

#include "iuimanager.h"
#include "idialogmanager.h"

#include "DialogManager.h"

#include <memory>

namespace ui
{

class UIManager :
	public IUIManager,
	public std::enable_shared_from_this<UIManager>
{
    // Sub-manager classes, constructed in initialiseModule to avoid being
    // called before the main window is ready.
	DialogManagerPtr _dialogManager;

public:
	IGroupDialog& getGroupDialog() override;

	IDialogManager& getDialogManager() override;

	// Called on radiant shutdown
	void clear();

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
};

} // namespace ui
