#pragma once

#include <vector>

#ifdef __APPLE__
#include <OpenAL/al.h>
#else
#include <AL/al.h>
#endif

#include <vorbis/vorbisfile.h>
#include <fmt/format.h>

#include "iarchive.h"
#include "stream/ScopedArchiveBuffer.h"
#include "OggFileStream.h"

namespace sound
{

/**
 * greebo: Loader class creating an AL buffer from an OGG file.
 */
class OggFileLoader
{
private:
    class FileWrapper
    {
    private:
        OggVorbis_File _oggFile;
        OggFileStream _stream;
        int _openResult;

    public:
        FileWrapper(ArchiveFile& file) :
            _stream(file)
        {
            // Setup the callbacks and point them to the helper class
            ov_callbacks callbacks;
            callbacks.read_func = OggFileStream::oggReadFunc;
            callbacks.seek_func = OggFileStream::oggSeekFunc;
            callbacks.close_func = OggFileStream::oggCloseFunc;
            callbacks.tell_func = OggFileStream::oggTellFunc;

            // Open the OGG data stream using the custom callbacks
            _openResult = ov_open_callbacks(static_cast<void*>(&_stream), &_oggFile, nullptr, 0, callbacks);
        }

        // Return the pointer to the handle structure needed to call the ov_* functions
        OggVorbis_File* getHandle()
        {
            if (_openResult != 0)
            {
                throw std::runtime_error(fmt::format("Error opening OGG file (error code: {0}", _openResult));
            }

            return &_oggFile;
        }

        ~FileWrapper()
        {
            // Clean up the OGG routines
            ov_clear(&_oggFile);
        }
    };
public:
    /**
     * greebo: Determines the OGG file length in seconds.
     * @throws: std::runtime_error if an error occurs.
     */
    static float GetDuration(ArchiveFile& vfsFile)
    {
        FileWrapper file(vfsFile);

        return static_cast<float>(ov_time_total(file.getHandle(), -1));
    }

    /**
     * greebo: Loads an OGG file from the given stream into OpenAL,
     * returns the openAL buffer handle.
     *
     * @throws: std::runtime_error if an error occurs.
     */
    static ALuint LoadFromFile(ArchiveFile& vfsFile)
    {
        FileWrapper file(vfsFile);

        // Get some information about the OGG file
        vorbis_info* vorbisInfo = ov_info(file.getHandle(), -1);

        // Check the number of channels
        ALenum format = (vorbisInfo->channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

        // Get the sample Rate
        ALsizei freq = static_cast<ALsizei>(vorbisInfo->rate);

        long bytes;
        char smallBuffer[4096];

        std::vector<char> largeBuffer;
        largeBuffer.reserve(vfsFile.size() * 2);

        do
        {
            int bitStream;
            // Read a chunk of decoded data from the vorbis file
            bytes = ov_read(file.getHandle(), smallBuffer, sizeof(smallBuffer), 0, 2, 1, &bitStream);

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

        return bufferNum;
    }
};

}
