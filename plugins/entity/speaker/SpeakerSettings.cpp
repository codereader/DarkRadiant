#include "SpeakerSettings.h"

namespace entity {

SpeakerSettingsManager& SpeakerSettings() {
	static SpeakerSettingsManager _speakerSettingsManager;
	return _speakerSettingsManager;
}

} // namespace entity
