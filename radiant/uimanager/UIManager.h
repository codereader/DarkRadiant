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
public:
	IGroupDialog& getGroupDialog() override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
};

} // namespace ui
