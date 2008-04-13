#ifndef SPEAKERSETTINGS_H_
#define SPEAKERSETTINGS_H_

#include <string>
#include "iregistry.h"
#include "iradiant.h"

namespace entity {

	namespace {
		const std::string RKEY_SHOW_ALL_SPEAKER_RADII = "user/ui/showAllSpeakerRadii";
	}

class SpeakerSettingsManager :
	public RegistryKeyObserver
{
	bool _showAllSpeakerRadii;
public:
	SpeakerSettingsManager() :
		_showAllSpeakerRadii(GlobalRegistry().get(RKEY_SHOW_ALL_SPEAKER_RADII) == "1")
	{
		// Register self as KeyObserver to get notified on key changes
		GlobalRegistry().addKeyObserver(this, RKEY_SHOW_ALL_SPEAKER_RADII);
	}
	
	// Update the internal bool on registrykey change
	void keyChanged(const std::string& key, const std::string& val) {
		_showAllSpeakerRadii = (val == "1");
		GlobalRadiant().updateAllWindows();
	}
	
	bool showAllSpeakerRadii() const {
		return _showAllSpeakerRadii;
	}
	
}; // class SpeakerSettingsManager

SpeakerSettingsManager& SpeakerSettings();

} // namespace entity

#endif /*SPEAKERSETTINGS_H_*/
