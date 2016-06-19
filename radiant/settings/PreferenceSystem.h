#pragma once

#include "ipreferencesystem.h"

class PreferenceSystem :
	public IPreferenceSystem
{
public:
	// Looks up a page for the given path and returns it to the client
	IPreferencesPagePtr getPage(const std::string& path) override;

	// RegisterableModule implementation
	virtual const std::string& getName() const override;
	virtual const StringSet& getDependencies() const override;
	virtual void initialiseModule(const ApplicationContext& ctx) override;
};

/** 
 * greebo: Direct accessor method for the preferencesystem for situations
 * where the actual PreferenceSystemModule is not loaded yet.
 * Everything else should use the GlobalPreferenceSystem() access method.
 */
IPreferenceSystem& GetPreferenceSystem();
