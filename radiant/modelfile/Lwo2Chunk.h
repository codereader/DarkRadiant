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

	// Adds a Chunk or Subchunk to this one, according to the type
	Lwo2Chunk::Ptr addChunk(const std::string& identifier_, Type type);

	// Adds a Chunk (size type U4) to this one
	Lwo2Chunk::Ptr addChunk(const std::string& identifier_);

	// Adds a Chunk (size type U2) to this one
	Lwo2Chunk::Ptr addSubChunk(const std::string& identifier_);

	void flushBuffer();

	void writeToStream(std::ostream& output);
};

}
