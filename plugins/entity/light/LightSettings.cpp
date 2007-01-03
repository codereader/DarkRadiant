#include "LightSettings.h"

LightSettingsManager& LightSettings() {
	static LightSettingsManager _lightSettingsManager;
	return _lightSettingsManager;
}
