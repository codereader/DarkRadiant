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

	// Internal method to walk over the registered pages
	void foreachPage(const std::function<void(PreferencePage&)>& functor);

	// RegisterableModule implementation
	virtual const std::string& getName() const override;
	virtual const StringSet& getDependencies() const override;
	virtual void initialiseModule(const ApplicationContext& ctx) override;

private:
	void ensureRootPage();
};

} // namespace

/** 
 * greebo: Direct accessor method for the preferencesystem for situations
 * where the actual PreferenceSystemModule is not loaded yet.
 * Everything else should use the GlobalPreferenceSystem() access method.
 */
settings::PreferenceSystem& GetPreferenceSystem();
