#pragma once

#include "iradiant.h"
#include "imodule.h"
#include <memory>

namespace map
{

class StartupMapLoader :
	public RegisterableModule
{
public:
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;

private:
	// This gets called as soon as the mainframe is shown
	void onMainFrameReady();

	void loadMapSafe(const std::string& map);
};

} // namespace map
