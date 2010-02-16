#ifndef _DECL_FILE_INPUT_STREAM_H_
#define _DECL_FILE_INPUT_STREAM_H_

#include "itextstream.h"
#include "iarchive.h"
#include <stdio.h>
#include <string>
#include <stack>
#include <boost/regex.hpp>

/**
 * greebo: An inputstream wrapping around an archive text file,
 * properly resolving #include statements found while streaming the contents.
 * This type of stream can be used to parse Doom 3 declaration files
 * like .gui and .script files which can make use of #include directives.
 */
class DeclFileInputStream : 
	public TextInputStream
{
protected:
	struct InputNode
	{
		ArchiveTextFilePtr archive;		// The owning archive
		TextInputStream* stream;		// The input stream
		std::string buffer;				// The immediate buffer

		InputNode(const ArchiveTextFilePtr& archive_) :
			archive(archive_),
			stream(&archive->getInputStream())
		{}
	};

	// The list of active input nodes.
	typedef std::list<InputNode> NodeList; 
	NodeList _nodes;

	NodeList::iterator _curNode;

	// Temporary buffer, with 1 extra byte for the NULL
	char _temp[BUFFER_SIZE+1];
	
	// The include string ('#include' by default)
	std::string _include;
	std::size_t _includeLength;

	boost::regex _includeRegex;

	char* _clientBufStart;
	char* _clientBufPos;

public:
	DeclFileInputStream(const ArchiveTextFilePtr& source, 
			const std::string& includeStr = "#include") :
		_include(includeStr),
		_includeLength(_include.length()),
		_includeRegex("^#include \"(.*)\"$")
	{
		// We start with the source file
		_nodes.push_back(InputNode(source));
		_curNode = _nodes.begin();
	}

	std::size_t read(char* buffer, std::size_t length)
	{
		char* bufferPos = buffer;
		
		while (_curNode != _nodes.end())
		{
			std::size_t bytesNeeded = length - (bufferPos - buffer);

			// Check if our buffer has enough bytes, 
			// attempt to fill it from the current stream, even it might have
			// too few bytes remaining
			ensureCurBufferHasBytes(bytesNeeded);

			// Process the buffer (recursively processing #included files)
			processBuffer(_curNode, bytesNeeded);

			// Copy the buffer from the _curNode to the client's buffer
			_curNode->buffer.copy(bufferPos, _curNode->buffer.length());

			// Increase the client pointer for the next iteration
			bufferPos += _curNode->buffer.length();

			// Erase the copied bytes from the buffer
			_curNode->buffer.clear();

			if (static_cast<std::size_t>(bufferPos - buffer) >= length) 
			{
				break;
			}

			// Not enough bytes read, increase iterator
			++_curNode;
		}

		return bufferPos - buffer;
	}

private:
	// Attempts to read <length> bytes from <buf>'s stream, appending the result
	// to <buf>'s buffer string. This is using _temp as temporary buffer
	// Returns the number of bytes read
	std::size_t readBytesFromBuffer(NodeList::iterator buf, std::size_t length)
	{
		std::size_t bytesNeeded = length;

		// Read in the amount of missing bytes, in packets of size BUFFER_SIZE max.
		while (bytesNeeded > 0)
		{
			// Take care of _temp overruns
			std::size_t bytesRead = buf->stream->read(
				_temp, 
				bytesNeeded > BUFFER_SIZE ? BUFFER_SIZE : bytesNeeded
			);
			assert(bytesNeeded >= bytesRead);

			if (bytesRead == 0) break; // No bytes read, stream exhausted

			// NULL-terminate the string
			_temp[bytesRead] = '\0';

			// Transfer _temp's contents to the end of the node's string buffer
			buf->buffer += _temp;
			
			bytesNeeded -= bytesRead;
		}

		return length - bytesNeeded;
	}

	void processBuffer(NodeList::iterator buf, std::size_t length)
	{
		if (length == 0) return;

		// Check buffer for #include statements, searching until 
		// the first one is found or the end of the buffer is reached
		for (std::size_t pos = 0; pos != std::string::npos; ++pos)
		{
			pos = buf->buffer.find('#', pos);

			if (pos > length) break; // Beyond scope

			if (pos == std::string::npos) break; // End of Buffer reached

			// We've found a '#', check if this can be an include
			if (pos > buf->buffer.length() - _includeLength)
			{
				// The #include could be cut off at the end, read a few more bytes
				readBytesFromBuffer(buf, _includeLength + 512);
			}

			// Check for "#include"
			if (buf->buffer.substr(pos, _includeLength) == _include)
			{
				// #include found, read what it says
				std::size_t lineEnd = buf->buffer.find('\n', pos+1);
				std::string includeLine = buf->buffer.substr(pos, lineEnd - pos);

				// Create a new node for the contents after the #include
				NodeList::iterator next = buf;
				next++;

				NodeList::iterator backBuf = _nodes.insert(next, InputNode(buf->archive));

				// Move everything after the #include to the new buffer
				backBuf->buffer = buf->buffer.substr(lineEnd+1);

				// Remove everything from the current buffer, including the "#include" line
				buf->buffer.erase(pos);

				// Now check the #included file, and create a new node between buf and backBuf
				boost::smatch matches;

				if (boost::regex_match(includeLine, matches, _includeRegex))
				{
					std::string includedFilePath = matches[1];

					// Acquire a new archive file
					ArchiveTextFilePtr file = GlobalFileSystem().openTextFile(includedFilePath);

					if (file != NULL)
					{
						// Create a new node for the new file
						NodeList::iterator next = buf;
						next++;
						NodeList::iterator newBuf = _nodes.insert(next, InputNode(file));

						// Recursively process until enough bytes are processed
						std::size_t bytesRemaining = length - buf->buffer.length();
						processBuffer(newBuf, bytesRemaining);
					}
					else
					{
						globalWarningStream() << "Cannot find #included file: " << includedFilePath << std::endl;
					}
				}

				// We've found one #include file, node list has been expanded
				// so give control back to the caller
				break; 
			}
		}
	}

	void ensureCurBufferHasBytes(std::size_t length)
	{
		if (_curNode->buffer.length() < length)
		{
			readBytesFromBuffer(_curNode, length);
		}
	}
};

#endif /* _DECL_FILE_INPUT_STREAM_H_ */
