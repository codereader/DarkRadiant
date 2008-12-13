#ifndef WAV_FILE_LOADER_H_
#define WAV_FILE_LOADER_H_

#include <stdexcept>
#include <AL/al.h>

class InputStream;

namespace sound {

/**
 * greebo: Loader class creating an AL buffer from a WAV file.
 *
 * Modeled after the one used by the Ogre3D people, found it posted
 * somewhere on the net.
 */
class WavFileLoader
{
public:

	/**
	 * greebo: Loads a WAV file from a stream into OpenAL.
	 *
	 * @throws: std::runtime_error if an error occurs.
	 */
	static ALuint LoadFromStream(InputStream& stream) {
		// buffers
		char magic[5];
		magic[4] = '\0';
		typedef StreamBase::byte_type byte;

		byte temp[256];

		int format = 0;
		
		// check magic
		stream.read(reinterpret_cast<byte*>(magic), 4);

		if (std::string(magic) != "RIFF") {
			throw std::runtime_error("No wav file");
		}
		
		// The next 4 bytes are the file size, we can skip this since we get the size from the DataStream
		unsigned int size;
		stream.read(reinterpret_cast<byte*>(&size), 4);

		// check file format
		stream.read(reinterpret_cast<byte*>(magic), 4);
		if (std::string(magic) != "WAVE") {
			throw std::runtime_error("Wrong wav file format");
		}
		
		// check 'fmt ' sub chunk (1)
		stream.read(reinterpret_cast<byte*>(magic), 4);
		if (std::string(magic) != "fmt ") {
			throw std::runtime_error("No 'fmt ' subchunk.");
		}
		
		// read (1)'s size
		unsigned int subChunk1Size(0);
		stream.read(reinterpret_cast<byte*>(&subChunk1Size), 4);

		if (subChunk1Size < 16) {
			throw std::runtime_error("'fmt ' chunk too small.");
		}

		// check PCM audio format
		unsigned short audioFormat(0);
		stream.read(reinterpret_cast<byte*>(&audioFormat), 2);
		
		if (audioFormat != 1) {
			throw std::runtime_error("Audio format is not PCM.");
		}

		// read number of channels
		unsigned short channels(0);
		stream.read(reinterpret_cast<byte*>(&channels), 2);
		
		// read frequency (sample rate)
		unsigned int freq = 0;
		stream.read(reinterpret_cast<byte*>(&freq), 4);

		// skip 6 bytes (Byte rate (4), Block align (2))
		stream.read(temp, 6);

		// read bits per sample
		unsigned short bps = 0;
		stream.read(reinterpret_cast<byte*>(&bps), 2);

		int bufferSize = 0;

		if (channels == 1) {
			if (bps == 8) {
				format = AL_FORMAT_MONO8;
				// Set BufferSize to 250ms (Frequency divided by 4 (quarter of a second))
				bufferSize = freq / 4;
			}
			else {
				format = AL_FORMAT_MONO16;
				// Set BufferSize to 250ms (Frequency * 2 (16bit) divided by 4 (quarter of a second))
				bufferSize = freq >> 1;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				bufferSize -= (bufferSize % 2);
			}
		}
		else {
			if (bps == 8) {
				format = AL_FORMAT_STEREO16;
				// Set BufferSize to 250ms (Frequency * 2 (8bit stereo) divided by 4 (quarter of a second))
				bufferSize = freq >> 1;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				bufferSize -= (bufferSize % 2);
			}
			else {
				format = AL_FORMAT_STEREO16;
				// Set BufferSize to 250ms (Frequency * 4 (16bit stereo) divided by 4 (quarter of a second))
				bufferSize = freq;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				bufferSize -= (bufferSize % 4);
			}
		}

		// check 'data' sub chunk (2)
		stream.read(reinterpret_cast<byte*>(magic), 4);
		if (std::string(magic) != "data" && std::string(magic) != "fact") {
			throw std::runtime_error("No 'data' subchunk.");
		}

		// fact is an option section we don't need to worry about
		if (std::string(magic) == "fact")
		{
			stream.read(temp, 8);

			// Now we should hit the data chunk
			stream.read(reinterpret_cast<byte*>(magic), 4);
			if (std::string(magic) != "data") {
				throw std::runtime_error("No 'data' subchunk.");
			}
		}

		// The next four bytes are the size remaing of the file
		unsigned int remainingSize = 0;
		stream.read(reinterpret_cast<byte*>(&remainingSize), 4);

		ALuint bufferNum = 0;
		alGenBuffers(1, &bufferNum);

		byte* buffer = new byte[remainingSize];
		stream.read(buffer, remainingSize);

		alBufferData(bufferNum, format, buffer, static_cast<ALsizei>(remainingSize), freq);

		delete[] buffer;

		return bufferNum;
	}
};

} // namespace sound

#endif /* WAV_FILE_LOADER_H_ */