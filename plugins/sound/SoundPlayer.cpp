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
	_source(0),
	_timer(200, checkBuffer, this)
{
	// Disable the timer, to make sure
	_timer.disable();
	
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
	_timer.disable();
	alutExit();
}

gboolean SoundPlayer::checkBuffer(gpointer data) {
	// Cast the passed pointer onto self
	SoundPlayer* self = reinterpret_cast<SoundPlayer*>(data);
	
	// Check for active source and buffer
	if (self->_source != 0 && self->_buffer != 0) {
		ALint state;
		// Query the state of the source
		alGetSourcei(self->_source, AL_SOURCE_STATE, &state);
		if (state == AL_STOPPED) {
			std::cout << "Playback has stopped.\n";
			// Erase the buffer
			self->clearBuffer();
			
			// Disable the timer to stop calling this function
			self->_timer.disable();
			return false;
		}
		else {
			std::cout << "Still playing.\n";
		}
	}
	
	// Return true, so that the timer gets called again
	return true;
}

void SoundPlayer::clearBuffer() {
	// Check if there is an active buffer
	if (_buffer != 0) {
		// Free the buffer, this stops the playback automatically
		alDeleteBuffers(1, &_buffer);
		_buffer = 0;
		_timer.disable();
	}
}

void SoundPlayer::play(ArchiveFile& file) {
	// Stop any previous playback operations, that might be still active 
	clearBuffer();
	
	std::cout << "File size: " << file.size() << "\n";
	// Convert the file into a buffer
	ScopedArchiveBuffer buffer(file);
	
	// Allocate a new buffer
	alGenBuffers(1, &_buffer); 
	
	// Create an AL sound buffer from the memory
	_buffer = alutCreateBufferFromFileImage(buffer.buffer, static_cast<ALsizei>(buffer.length));
	
	// Code for decoding possible OGG files goes here
		
	// Assign the buffer to the source and play it
	alSourcei(_source, AL_BUFFER, _buffer);
	alSourcePlay(_source);
	
	// Enable the periodic buffer check, this destructs the buffer
	// as soon as the playback has finished
	_timer.enable();
}

} // namespace sound
