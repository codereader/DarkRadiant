#include "LightSettings.h"

namespace entity {

LightSettingsManager& LightSettings() {
	static LightSettingsManager _lightSettingsManager;
	return _lightSettingsManager;
}

} // namespace entity
