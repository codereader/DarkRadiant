#pragma once

#include "imodule.h"
#include "iradiant.h"
#include "iuimanager.h"
#include "idialogmanager.h"

#include "DialogManager.h"

#include <memory>

namespace wxutil { class LocalBitmapArtProvider; }

namespace ui
{

class UIManager :
	public IUIManager,
	public std::enable_shared_from_this<UIManager>
{
    // Sub-manager classes, constructed in initialiseModule to avoid being
    // called before the main window is ready.
	DialogManagerPtr _dialogManager;

	std::unique_ptr<wxutil::LocalBitmapArtProvider> _bitmapArtProvider;

public:
	IGroupDialog& getGroupDialog() override;

	IDialogManager& getDialogManager() override;

	const std::string& ArtIdPrefix() const override;

	// Called on radiant shutdown
	void clear();

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
};

} // namespace ui
