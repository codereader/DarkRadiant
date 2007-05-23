#include "SoundPlayer.h"

#include "stream/textstream.h"
#include "stream/textfilestream.h"
#include "archivelib.h"
#include "imagelib.h" // for ScopedArchiveBuffer
#include <iostream>

namespace sound {

// Constructor
SoundPlayer::SoundPlayer() :
	_buffer(0),
	_source(0)
{
	// Initialise the ALUT library with two NULL pointers instead of &argc, argv 
	// (yes, this is allowed)
	alutInit(NULL, NULL);
	alGenSources(1, &_source);
	
	if (alGetError() != AL_FALSE) {
		globalErrorStream() << "SoundPlayer: Error while initialising.\n";
	}
	else {
		globalOutputStream() << "SoundPlayer initialised.\n";
	}
}

SoundPlayer::~SoundPlayer() {
	alutExit();
}

void SoundPlayer::play(ArchiveFile& file) {
	std::cout << "File size: " << file.size() << "\n";
	// Convert the file into a buffer
	ScopedArchiveBuffer buffer(file);
	
	// Allocate a new buffer
	alGenBuffers(1, &_buffer); 
	
	// Create an AL sound buffer from the memory
	_buffer = alutCreateBufferFromFileImage(buffer.buffer, static_cast<ALsizei>(buffer.length));
	
	// Code for decoding possible OGG files goes here
	
	//_buffer = alutCreateBufferFromFile(vfsFile.c_str());
	
	// Assign the buffer to the source and play it
	alSourcei(_source, AL_BUFFER, _buffer);
	alSourcePlay(_source);
	alutSleep(10);
	
	// Free the buffer
	alDeleteBuffers(1, &_buffer);
}

} // namespace sound
