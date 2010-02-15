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
	struct InputFile
	{
		TextInputStream* stream;		// The input stream
		ArchiveTextFilePtr archive;		// The owning archive
		std::string front;				// The immediate buffer, for serving to the client
		std::string back;				// The "back-burner", saved for later serving
	};

	// The stack of active streams.
	typedef std::stack<InputFile> StreamStack; 
	StreamStack _streams;

	static const std::size_t CHUNK_SIZE = BUFFER_SIZE >> 1;

	// The include string ('#include' by default)
	std::string _include;
	std::size_t _includeLength;

	boost::regex _includeRegex;

public:
	DeclFileInputStream(const ArchiveTextFilePtr& source, 
			const std::string& includeStr = "#include") :
		_include(includeStr),
		_includeLength(_include.length()),
		_includeRegex("^#include \"(.*)\"$")
	{
		// We start with the source file
		_streams.push(InputFile());
		_streams.top().archive = source;
		_streams.top().stream = &source->getInputStream();
	}

	std::size_t read(char* buffer, std::size_t length)
	{
		return _streams.top().stream->read(buffer, length);
	}
};

#endif /* _DECL_FILE_INPUT_STREAM_H_ */
