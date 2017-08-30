#include "DeflatedInputStream.h"

#include <zlib.h>

namespace archive
{

DeflatedInputStream::DeflatedInputStream(InputStream& istream) :
	_istream(istream),
	_zipStream(new z_stream)
{
	_zipStream->zalloc = 0;
	_zipStream->zfree = 0;
	_zipStream->opaque = 0;
	_zipStream->avail_in = 0;

	inflateInit2(_zipStream.get(), -MAX_WBITS);
}

DeflatedInputStream::~DeflatedInputStream()
{
	inflateEnd(_zipStream.get());
}

DeflatedInputStream::size_type DeflatedInputStream::read(byte_type* buffer, size_type length)
{
	// Tell inflate() to load the data directly to the given buffer
	_zipStream->next_out = buffer;
	_zipStream->avail_out = static_cast<uInt>(length);

	while (_zipStream->avail_out != 0)
	{
		if (_zipStream->avail_in == 0)
		{
			// Load some data from the wrapped buffer and point z_stream to it
			_zipStream->next_in = _buffer;
			_zipStream->avail_in = static_cast<uInt>(_istream.read(_buffer, sizeof(_buffer)));
		}

		if (inflate(_zipStream.get(), Z_SYNC_FLUSH) != Z_OK)
		{
			break;
		}
	}

	return length - _zipStream->avail_out;
}

}
