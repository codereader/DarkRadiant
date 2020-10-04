#pragma once

#include <stdexcept>
#include "idatastream.h"

#ifdef __APPLE__
#include <OpenAL/al.h>
#else
#include <AL/al.h>
#endif

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
private:
    struct FileInfo
    {
        char magic[5];
        unsigned int size;
        char fileFormat[5];
        unsigned short audioFormat;
        unsigned short channels;
        unsigned int freq;
        unsigned short bps; // bits per sample

        FileInfo()
        {
            magic[4] = '\0';
            fileFormat[4] = '\0';
            audioFormat = 0;
        }

        ALenum getAlFormat() const
        {
            if (channels == 1)
            {
                return bps == 8 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
            }
            else
            {
                return  bps == 8 ? AL_FORMAT_STEREO16 : AL_FORMAT_STEREO16;
            }
        }
    };

    typedef StreamBase::byte_type byte;

public:
    /**
     * greebo: Loads a WAV file from a stream into OpenAL,
     * returns the openAL buffer handle.
     *
     * @throws: std::runtime_error if an error occurs.
     */
    static float GetDuration(InputStream& stream)
    {
        FileInfo info;
        ParseFileInfo(stream, info);

        SkipToRemainingData(stream);

        // The next four bytes are the remaining size of the file
        unsigned int remainingSize = 0;
        stream.read(reinterpret_cast<byte*>(&remainingSize), sizeof(remainingSize));

        // Calculate how many samples we have in the payload, then calculate the duration
        auto numSamples = remainingSize / (info.bps >> 3);
        auto numSamplesPerChannel = numSamples / info.channels;

        return static_cast<float>(numSamplesPerChannel) / info.freq;
    }

	/**
	 * greebo: Loads a WAV file from a stream into OpenAL,
     * returns the openAL buffer handle.
	 *
	 * @throws: std::runtime_error if an error occurs.
	 */
	static ALuint LoadFromStream(InputStream& stream)
    {
        FileInfo info;
        ParseFileInfo(stream, info);

        SkipToRemainingData(stream);

		// The next four bytes are the remaining size of the file
		unsigned int remainingSize = 0;
		stream.read(reinterpret_cast<byte*>(&remainingSize), sizeof(remainingSize));

		ALuint bufferNum = 0;
		alGenBuffers(1, &bufferNum);

        std::vector<byte> data(remainingSize);
        stream.read(data.data(), remainingSize);

		alBufferData(bufferNum, info.getAlFormat(), data.data(), static_cast<ALsizei>(remainingSize), info.freq);

		return bufferNum;
	}

private:
    // Assuming that the FMT chunk has been parsed, this seeks forward to the 
    // beginning of the sound data
    static void SkipToRemainingData(InputStream& stream)
    {
        char buffer[5];
        buffer[4] = '\0';

        // check 'data' sub chunk (2)
        stream.read(reinterpret_cast<byte*>(buffer), 4);

        if (std::string(buffer) != "data" && std::string(buffer) != "fact")
        {
            throw std::runtime_error("No 'data' subchunk.");
        }

        // fact is an option section we don't need to worry about
        if (std::string(buffer) == "fact")
        {
            byte temp[8];
            stream.read(temp, 8);

            // Now we should hit the data chunk
            stream.read(reinterpret_cast<byte*>(buffer), 4);
            if (std::string(buffer) != "data")
            {
                throw std::runtime_error("No 'data' subchunk.");
            }
        }
    }
    
    // Reads the file header, leaves the stream right after the end of the first chunk
    static void ParseFileInfo(InputStream& stream, FileInfo& info)
    {
        // check magic
        stream.read(reinterpret_cast<byte*>(info.magic), 4);

        if (std::string(info.magic) != "RIFF")
        {
            throw std::runtime_error("No wav file");
        }

        // The next 4 bytes are the file size, we can skip this since we get the size from the DataStream
        stream.read(reinterpret_cast<byte*>(&info.size), sizeof(info.size));

        // check file format
        stream.read(reinterpret_cast<byte*>(info.fileFormat), 4);

        if (std::string(info.fileFormat) != "WAVE")
        {
            throw std::runtime_error("Wrong wav file format");
        }

        // check 'fmt ' sub chunk (1)
        char buffer[5];
        buffer[4] = '\0';
        stream.read(reinterpret_cast<byte*>(buffer), 4);
        if (std::string(buffer) != "fmt ") {
            throw std::runtime_error("No 'fmt ' subchunk.");
        }

        // read (1)'s size
        unsigned int subChunk1Size(0);
        stream.read(reinterpret_cast<byte*>(&subChunk1Size), 4);

        if (subChunk1Size < 16)
        {
            throw std::runtime_error("'fmt ' chunk too small.");
        }

        // check PCM audio format
        stream.read(reinterpret_cast<byte*>(&info.audioFormat), sizeof(info.audioFormat));

        if (info.audioFormat != 1)
        {
            throw std::runtime_error("Audio format is not PCM.");
        }

        // read number of channels
        stream.read(reinterpret_cast<byte*>(&info.channels), sizeof(info.channels));

        // read frequency (sample rate)
        stream.read(reinterpret_cast<byte*>(&info.freq), sizeof(info.freq));

        // skip 6 bytes (Byte rate (4), Block align (2))
        byte temp[6];
        stream.read(temp, 6);

        // read bits per sample
        stream.read(reinterpret_cast<byte*>(&info.bps), sizeof(info.bps));
    }
};

} // namespace sound
