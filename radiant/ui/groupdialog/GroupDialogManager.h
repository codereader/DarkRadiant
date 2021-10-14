#pragma once

#include "ui/igroupdialog.h"
#include <memory>

namespace ui
{

class GroupDialogManager :
	public IGroupDialogManager
{
public:
	IGroupDialog& get() override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
};

} // namespace ui
