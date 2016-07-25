#pragma once

#include "parser/DefTokeniser.h"

namespace map
{

class InfoFile
{
public:
	// Tokens / Constants
	// The version of the map info file
	static const int MAP_INFO_VERSION = 2;

	// InfoFile tokens --------------------------------------------------
	static const char* const HEADER_SEQUENCE;

private:
	// The actual DefTokeniser to split the infoStream into pieces
	parser::BasicDefTokeniser<std::istream> _tok;

	// TRUE if the map info file was found to be valid
	bool _isValid;

public:
	// Pass the input stream to the constructor
	InfoFile(std::istream& infoStream);

	// Parse the entire file
	void parse();

private:
	void parseInfoFileBody();
};

} // namespace map
