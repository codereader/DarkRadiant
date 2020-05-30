#pragma once

#include "imap.h"
#include "imapinfofile.h"
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

	// Import root node
	const scene::IMapRootNodePtr& _root;

	// Index to Node mapping
	const NodeIndexMap& _nodeMap;

public:
	// Pass the input stream to the constructor, plus some info about the map we're dealing with
	InfoFile(std::istream& infoStream, const scene::IMapRootNodePtr& root, const NodeIndexMap& nodeMap);

	// Parse the entire file
	void parse();

private:
	void parseInfoFileHeader();
	void parseInfoFileBody();
};

} // namespace map
