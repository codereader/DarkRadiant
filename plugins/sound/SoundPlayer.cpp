#include "SoundPlayer.h"

#include <iostream>

namespace sound {

// Constructor
SoundPlayer::SoundPlayer() :
	_buffer(0),
	_source(0)
{
	// Initialise the ALUT library with two NULL pointers 
	// (yes, this is allowed)
	alutInit(NULL, NULL);
	alGenSources(1, &_source);
	
	if (alGetError() != AL_FALSE) {
		std::cout << "SoundPlayer: Error while initialising.\n";
	}
	else {
		std::cout << "SoundPlayer initialised.\n";
	}
}

SoundPlayer::~SoundPlayer() {
	alutExit();
}

void SoundPlayer::play(const std::string& vfsFile) {
	std::cout << "Playing sound.\n";
	
	// Code for decoding possible OGG files goes here
	
	_buffer = alutCreateBufferFromFile(vfsFile.c_str());
	
	// Assign the buffer to the source and play it
	alSourcei(_source, AL_BUFFER, _buffer);
	alSourcePlay(_source);
	alutSleep(0);
}

} // namespace sound
