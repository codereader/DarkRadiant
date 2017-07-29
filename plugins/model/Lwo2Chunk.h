#pragma once

#include <string>
#include <memory>
#include <vector>
#include <sstream>

namespace model
{

// Represents a Chunk in the LWO2 file, accepting content
// through the public stream member. 
// A Chunk can have contents and/or 1 or more subchunks. 
class Lwo2Chunk
{
public:
	typedef std::shared_ptr<Lwo2Chunk> Ptr;

	enum class Type
	{
		Chunk,
		SubChunk
	};

private:
	Type _chunkType;

	// The number of bytes used for the size info of this chunk
	unsigned int _sizeDescriptorByteCount;

public:
	std::string identifier; // the 4-byte ID

	// Child chunks
	std::vector<Lwo2Chunk::Ptr> subChunks;

	// Stream binary data into here
	std::stringstream stream;

	Lwo2Chunk(const std::string& identifier_, Type type);

	// Returns the size of this Chunk's content
	// excluding this Chunk's ID (4 bytes) and Size info (4 bytes)
	unsigned int getContentSize() const;

	// Adds the specified empty Chunk and returns its reference
	Lwo2Chunk::Ptr addChunk(const std::string& identifier_, Type type);

	void flushBuffer();

	void writeToStream(std::ostream& output);
};

}
