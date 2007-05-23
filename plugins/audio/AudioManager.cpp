
#include "iaudio.h"		// The Abstract Base Class
#include <iostream>

#include <AL/al.h>
#include <AL/alut.h>

class AudioManager : 
	public IAudioManager
{
public:
	// Radiant Module stuff
	typedef IAudioManager Type;
	STRING_CONSTANT(Name, "*");

	// Return the static instance
	IAudioManager* getTable() {
		return this;
	}

private:
	// The buffer containing the currently played audio data
	ALuint _buffer;
	
	// The source playing the buffer
	ALuint _source;
	
public:
	// Constructor
	AudioManager() :
		_buffer(0),
		_source(0)
	{
		// Initialise the ALUT library with two NULL pointers 
		// (yes, this is allowed)
		alutInit(NULL, NULL);
		alGenSources(1, &_source);
		
		if (alGetError() != AL_FALSE) {
			std::cout << "AudioManager: Error while initialising.\n";
		}
		else {
			std::cout << "AudioManager initialised.\n";
		}
	}
	
	~AudioManager() {
		alutExit();
	}

	virtual void playSound(const std::string& fileName) {
		std::cout << "Playing sound.\n";
		
		_buffer = alutCreateBufferFromFile(fileName.c_str());
		
		// Assign the buffer to the source and play it
		alSourcei(_source, AL_BUFFER, _buffer);
		alSourcePlay(_source);
		alutSleep(0);
	}

}; // class AudioManager

/* AudioManager dependencies class. 
 */
class AudioManagerDependencies
{};

/* Required code to register the module with the ModuleServer.
 */
#include "modulesystem/singletonmodule.h"

typedef SingletonModule<AudioManager, AudioManagerDependencies> AudioManagerModule;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server) {
	static AudioManagerModule _theManager;
	initialiseModule(server);
	_theManager.selfRegister();
}
