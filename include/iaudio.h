#ifndef IAUDIO_H_
#define IAUDIO_H_

#include <string>
#include "generic/constant.h"

/** Abstract base class for the Audio manager
 */
class IAudioManager
{
public:
	INTEGER_CONSTANT(Version, 1);
	STRING_CONSTANT(Name, "audio");

	// Tries to load and play the sound from the given <fileName>
	virtual void playSound(const std::string& fileName) = 0;
};

// Module definitions

#include "modulesystem.h"

template<typename Type>
class GlobalModule;
typedef GlobalModule<IAudioManager> GlobalAudioManagerModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<IAudioManager> GlobalAudioManagerModuleRef;

// This is the accessor function to the audio module
inline IAudioManager& GlobalAudioManager() {
	return GlobalAudioManagerModule::getTable();
}

#endif /*IAUDIO_H_*/
