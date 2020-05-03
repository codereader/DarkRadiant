#pragma once

#include "idatastream.h"
#include <memory>

// Forward decl.
struct z_stream_s;
typedef z_stream_s z_stream;

namespace archive
{

/// \brief A wrapper around an InputStream of data compressed with the zlib deflate algorithm.
///
/// - Uses z_stream to decompress the data stream on the fly.
/// - Uses a buffer to reduce the number of times the wrapped stream must be read.
class DeflatedInputStream :
	public InputStream
{
private:
	InputStream& _istream;
	std::unique_ptr<z_stream> _zipStream;
	unsigned char _buffer[1024];

public:
	DeflatedInputStream(InputStream& istream);

	virtual ~DeflatedInputStream();

	// InputStream implementation
	size_type read(byte_type* buffer, size_type length) override;
};

}
