#pragma once

#include "iarchive.h"
#include <fmt/format.h>
#include <vector>

#ifdef __APPLE__
#include <OpenAL/al.h>
#else
#include <AL/al.h>
#endif

#include "stream/ScopedArchiveBuffer.h"
#include "OggFileStream.h"

namespace sound
{

/**
 * greebo: Loader class creating an AL buffer from an OGG file.
 */
class OggFileLoader
{
public:
    /**
     * greebo: Loads an OGG file from the given stream into OpenAL,
     * returns the openAL buffer handle.
     *
     * @throws: std::runtime_error if an error occurs.
     */
    static ALuint LoadFromFile(ArchiveFile& file)
    {
        // Initialise the wrapper class
        OggFileStream stream(file);

        // This is an OGG Vorbis file, decode it
        vorbis_info* vorbisInfo;
        OggVorbis_File oggFile;

        // Setup the callbacks and point them to the helper class
        ov_callbacks callbacks;
        callbacks.read_func = OggFileStream::oggReadFunc;
        callbacks.seek_func = OggFileStream::oggSeekFunc;
        callbacks.close_func = OggFileStream::oggCloseFunc;
        callbacks.tell_func = OggFileStream::oggTellFunc;

        // Open the OGG data stream using the custom callbacks
        int res = ov_open_callbacks(static_cast<void*>(&stream), &oggFile, nullptr, 0, callbacks);

        if (res != 0)
        {
            throw std::runtime_error(fmt::format("Error opening OGG file (error code: {0}", res));
        }
        
        // Open successful

        // Get some information about the OGG file
        vorbisInfo = ov_info(&oggFile, -1);

        // Check the number of channels
        ALenum format = (vorbisInfo->channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

        // Get the sample Rate
        ALsizei freq = static_cast<ALsizei>(vorbisInfo->rate);

        long bytes;
        char smallBuffer[4096];

        std::vector<char> largeBuffer;
        largeBuffer.reserve(file.size() * 2);

        do
        {
            int bitStream;
            // Read a chunk of decoded data from the vorbis file
            bytes = ov_read(&oggFile, smallBuffer, sizeof(smallBuffer), 0, 2, 1, &bitStream);

            if (bytes == OV_HOLE)
            {
                rError() << "Error decoding OGG: OV_HOLE.\n";
            }
            else if (bytes == OV_EBADLINK)
            {
                rError() << "Error decoding OGG: OV_EBADLINK.\n";
            }
            else
            {
                // Stuff this into the variable-sized buffer
                largeBuffer.insert(largeBuffer.end(), smallBuffer, smallBuffer + bytes);
            }
        } 
        while (bytes > 0);

        ALuint bufferNum = 0;
        // Allocate a new buffer
        alGenBuffers(1, &bufferNum);

        // Upload sound data to buffer
        alBufferData(bufferNum,
            format,
            largeBuffer.data(),
            static_cast<ALsizei>(largeBuffer.size()),
            freq);

        // Clean up the OGG routines
        ov_clear(&oggFile);

        return bufferNum;
    }
};

}
