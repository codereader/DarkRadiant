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
#include <boost/shared_ptr.hpp>

// We need the usleep() command. Be sure to include the windows.h
// header after the local textfilestream.h because there are definition conflicts otherwise.
#ifdef WIN32
#include <windows.h>
#define usleep(x) Sleep((x)/1000)
#else
#include <unistd.h>
#endif

#include "OggFileStream.h"
#include "WavFileLoader.h"

namespace sound {

	namespace {
		typedef std::vector<char> DecodeBuffer;
		typedef boost::shared_ptr<DecodeBuffer> DecodeBufferPtr;
	}

// Constructor
SoundPlayer::SoundPlayer() :
	_context(NULL),
	_buffer(0),
	_source(0),
	_timer(200, checkBuffer, this)
{
	// Disable the timer, to make sure
	_timer.disable();

	// Create device
	ALCdevice* device = alcOpenDevice(NULL);

	if (device != NULL) {
		// Create context
		_context = alcCreateContext(device, NULL);

		if (_context != NULL) {
			// Make context current
			if (!alcMakeContextCurrent(_context)) {
				alcDestroyContext(_context);
				alcCloseDevice(device);
				_context = NULL;

				globalErrorStream() << "Could not make ALC context current." << std::endl;
			}

			// Success
			globalOutputStream() << "SoundPlayer: OpenAL context successfully set up." << std::endl;
		}
		else {
			alcCloseDevice(device);
			globalErrorStream() << "Could not create ALC context." << std::endl;
		}
	}
	else {
		globalErrorStream() << "Could not open ALC device." << std::endl;
    }
}

SoundPlayer::~SoundPlayer() {
	clearBuffer();

	// Unset the context
	if (alcMakeContextCurrent(NULL)) {
		// Destroy the context and close device if appropriate
		if (_context != NULL) {
			ALCdevice* device = alcGetContextsDevice(_context);
			alcDestroyContext(_context);
			
			if (alcGetError(device) != ALC_NO_ERROR) {
				globalErrorStream() << "Could not destroy ALC context." << std::endl;
			}

			if (!alcCloseDevice(device)) {
				globalErrorStream() << "Could not close ALC device." << std::endl;
			}
		}
	}
	else {
		globalErrorStream() << "Could not reset ALC context." << std::endl;
    }
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
	if (_source != 0) {
		// Stop playing
		alSourceStop(_source);
		alDeleteSources(1, &_source);
		_source = 0;
				
		if (_buffer != 0) {
			// Free the buffer
			alDeleteBuffers(1, &_buffer);
			_buffer = 0;
		}
	}

	_timer.disable();
}

void SoundPlayer::stop() {
	clearBuffer();
}

void SoundPlayer::play(ArchiveFile& file) {
	// Stop any previous playback operations, that might be still active 
	clearBuffer();
	
	// Retrieve the extension
	std::string ext = os::getExtension(file.getName());
	
	if (boost::algorithm::to_lower_copy(ext) == "ogg") {
		// Convert the file into a buffer, self-destructs at end of scope
		ScopedArchiveBuffer buffer(file);

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
			//std::cout << "Sample rate is " << freq << "\n";
			
			long bytes;
			char smallBuffer[4096];
			DecodeBufferPtr largeBuffer(new DecodeBuffer());
			do {
				int bitStream;
				// Read a chunk of decoded data from the vorbis file
				bytes = ov_read(&oggFile, smallBuffer, sizeof(smallBuffer), 
								0, 2, 1, &bitStream);
				
				if (bytes == OV_HOLE) {
					globalErrorStream() << "SoundPlayer: Error decoding OGG: OV_HOLE.\n"; 
				}
				else if (bytes == OV_EBADLINK) {
					globalErrorStream() << "SoundPlayer: Error decoding OGG: OV_EBADLINK.\n";
				}
				else {
					// Stuff this into the variable-sized buffer
					largeBuffer->insert(largeBuffer->end(), smallBuffer, smallBuffer + bytes);
				}
			} while (bytes > 0);
			
			// Allocate a new buffer
			alGenBuffers(1, &_buffer);
			
			DecodeBuffer& bufferRef = *largeBuffer;
			
			// Upload sound data to buffer
			alBufferData(_buffer, 
						 format, 
						 &bufferRef[0], 
						 static_cast<ALsizei>(bufferRef.size()), 
						 freq);

			// Clean up the OGG routines
  			ov_clear(&oggFile);
  		}
  		else {
  			globalErrorStream() << "SoundPlayer: Error opening OGG file.\n";
  		}
	}
	else {
		// Must be a wave file
		try {
			// Create an AL sound buffer directly from the buffer in memory
			_buffer = WavFileLoader::LoadFromStream(file.getInputStream());
		}
		catch (std::runtime_error e) {
			globalErrorStream() << "SoundPlayer: Error opening WAV file: " << e.what() << std::endl;
			_buffer = 0;
		}
	}
	
	if (_buffer != 0) {
		alGenSources(1, &_source);
		// Assign the buffer to the source and play it
		alSourcei(_source, AL_BUFFER, _buffer);

		// greebo: Wait 10 msec. to fix a problem with buffers not being played
		// maybe the AL needs time to push the data?
		usleep(10000);

		alSourcePlay(_source);
		
		// Enable the periodic buffer check, this destructs the buffer
		// as soon as the playback has finished
		_timer.enable();	
	}
}

} // namespace sound
