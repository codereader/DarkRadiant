#ifndef LIGHTSETTINGS_H_
#define LIGHTSETTINGS_H_

#include <string>
#include "iregistry.h"
#include "iradiant.h"

namespace entity {

	namespace {
		const std::string RKEY_SHOW_ALL_LIGHT_RADII = "user/ui/showAllLightRadii";
	}

class LightSettingsManager :
	public RegistryKeyObserver
{
	bool _showAllLightRadii;
public:
	LightSettingsManager() :
		_showAllLightRadii(GlobalRegistry().get(RKEY_SHOW_ALL_LIGHT_RADII) == "1")
	{
		// Register self as KeyObserver to get notified on key changes
		GlobalRegistry().addKeyObserver(this, RKEY_SHOW_ALL_LIGHT_RADII);
	}
	
	// Update the internal bool on registrykey change
	void keyChanged(const std::string& key, const std::string& val) {
		_showAllLightRadii = (val == "1");
		GlobalRadiant().updateAllWindows();
	}
	
	bool showAllLightRadii() const {
		return _showAllLightRadii;
	}
	
}; // class LightSettingsManager

LightSettingsManager& LightSettings();

} // namespace entity

#endif /*LIGHTSETTINGS_H_*/
