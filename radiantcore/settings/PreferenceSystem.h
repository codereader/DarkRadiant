#pragma once

#include "ipreferencesystem.h"
#include "PreferencePage.h"

namespace settings
{

class PreferenceSystem :
	public IPreferenceSystem
{
private:
	PreferencePagePtr _rootPage;

public:
	// Looks up a page for the given path and returns it to the client
	IPreferencePage& getPage(const std::string& path) override;
	void foreachPage(const std::function<void(IPreferencePage&)>& functor) override;

	// RegisterableModule implementation
	virtual const std::string& getName() const override;
	virtual const StringSet& getDependencies() const override;
	virtual void initialiseModule(const IApplicationContext& ctx) override;

private:
	void ensureRootPage();
};

} // namespace
