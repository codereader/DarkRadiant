#include "SoundPlayer.h"

#include <vorbis/vorbisfile.h>
#include <iostream>
#include <vector>
#include <boost/algorithm/string/case_conv.hpp>

#include "stream/textstream.h"
#include "stream/textfilestream.h"
#include "archivelib.h"
#include "os/path.h"
#include "imagelib.h" // for ScopedArchiveBuffer

#include "OggFileStream.h"

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
			// Erase the buffer
			self->clearBuffer();
			
			// Disable the timer to stop calling this function
			self->_timer.disable();
			return false;
		}
	}
	
	// Return true, so that the timer gets called again
	return true;
}

void SoundPlayer::clearBuffer() {
	// Check if there is an active buffer
	if (_source != 0 && _buffer != 0) {
		// Stop playing
		alSourceStop(_source);
		// Free the buffer, this stops the playback automatically
		alDeleteBuffers(1, &_buffer);
		_buffer = 0;
		_timer.disable();
	}
}

void SoundPlayer::play(ArchiveFile& file) {
	// Stop any previous playback operations, that might be still active 
	clearBuffer();
	
	// Convert the file into a buffer, self-destructs at end of scope
	ScopedArchiveBuffer buffer(file);
	
	// Retrieve the extension
	std::string ext = os::getExtension(file.getName());
	
	if (boost::algorithm::to_lower_copy(ext) == "ogg") {
		// This is an OGG Vorbis file, decode it
		vorbis_info* vorbisInfo;
  		OggVorbis_File oggFile;
  		
  		// Initialise the wrapper class
  		OggFileStream stream(buffer);
  		
  		// Setup the callbacks and point them to the helper class
  		ov_callbacks callbacks;
  		callbacks.read_func = OggFileStream::oggReadFunc;
  		callbacks.seek_func = OggFileStream::oggSeekFunc;
  		callbacks.close_func = OggFileStream::oggCloseFunc;
  		callbacks.tell_func = OggFileStream::oggTellFunc;
  		
  		// Open the OGG data stream using the custom callbacks
  		int res = ov_open_callbacks(static_cast<void*>(&stream), &oggFile, 
									NULL, 0, callbacks);
  		
  		if (res == 0) {
  			// Open successful
  			
  			// Get some information about the OGG file
			vorbisInfo = ov_info(&oggFile, -1);

			// Check the number of channels
			ALenum format = (vorbisInfo->channels == 1) ? AL_FORMAT_MONO16 
														: AL_FORMAT_STEREO16;
			
			// Get the sample Rate			
			ALsizei freq = (vorbisInfo->rate);
			std::cout << "Sample rate is " << freq << "\n";
			
			long bytes;
			char smallBuffer[16384];
			std::vector<char> largeBuffer;
			do {
				int bitStream;
				// Read a chunk of decoded data from the vorbis file
				bytes = ov_read(&oggFile, smallBuffer, 16384, 0, 2, 1, &bitStream);
				// Stuff this into the variable-sized buffer
				largeBuffer.insert(largeBuffer.end(), smallBuffer, smallBuffer + bytes);
			} while (bytes > 0);
			
			// Allocate a new buffer
			alGenBuffers(1, &_buffer);
			
			// Upload sound data to buffer
			alBufferData(_buffer, 
						 format, 
						 &largeBuffer[0], 
						 static_cast<ALsizei>(largeBuffer.size()), 
						 freq);

			// Clean up the OGG routines
  			ov_clear(&oggFile);
  		}
  		else {
  			std::cout << "Error on opening.\n";
  		}
	}
	else {
		// Must be a wave file
		// Create an AL sound buffer directly from the buffer in memory
		_buffer = alutCreateBufferFromFileImage(
			buffer.buffer,	// The pointer to the buffer data 
			static_cast<ALsizei>(buffer.length) // The length
		);
	}
	
	if (_buffer != 0) {
		// Assign the buffer to the source and play it
		alSourcei(_source, AL_BUFFER, _buffer);
		alSourcePlay(_source);
		
		// Enable the periodic buffer check, this destructs the buffer
		// as soon as the playback has finished
		_timer.enable();	
	}
}

} // namespace sound
