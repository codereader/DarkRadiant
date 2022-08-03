#include "LocalisationModule.h"

#include "i18n.h"
#include "ipreferencesystem.h"
#include "registry/registry.h"

#include "module/StaticModule.h"
#include "LocalisationProvider.h"

namespace settings
{

const std::string& LocalisationModule::getName() const
{
	static std::string _name("LocalisationModule");
	return _name;
}

const StringSet& LocalisationModule::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_XMLREGISTRY);
	}

	return _dependencies;
}

void LocalisationModule::initialiseModule(const IApplicationContext& ctx)
{
	// Construct the list of available languages
	ComboBoxValueList langs;

	LocalisationProvider::Instance()->foreachAvailableLanguage([&](const LocalisationProvider::Language& lang)
	{
		langs.emplace_back(lang.twoDigitCode + " - " + lang.displayName);
	});

	auto registryKey = LocalisationProvider::RKEY_LANGUAGE;

	// Load the currently selected index into the registry
	registry::setValue(registryKey, LocalisationProvider::Instance()->getCurrentLanguageIndex());
	GlobalRegistry().setAttribute(registryKey, "volatile", "1"); // don't save this to user.xml

	// Add Preferences
	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Language"));
	page.appendCombo(_("Language"), registryKey, langs);

	page.appendLabel(_("<b>Note:</b> You'll need to restart DarkRadiant\nafter changing the language setting."));
}

void LocalisationModule::shutdownModule()
{
	LocalisationProvider::Instance()->saveLanguageSetting();
}

module::StaticModuleRegistration<LocalisationModule> localisationModule;

}
