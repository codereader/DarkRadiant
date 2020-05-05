#pragma once

#include <map>
#include "imodule.h"

namespace ui
{

class FilterUserInterface :
	public RegisterableModule
{
private:
	std::map<std::string, IEventPtr> _toggleFilterEvents;

public:
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	void toggleFilter(const std::string& filterName, bool newState);
};

}
