#ifndef _CODE_TOKENISER_H_
#define _CODE_TOKENISER_H_

#include "iarchive.h"
#include "DefTokeniser.h"
#include <list>

namespace parser {

/**
 * High-level tokeniser taking a specific VFS file as input.
 * It is able to handle preprocessor statements like #include 
 * by maintaining several child tokenisers. This can be used
 * to parse code-like files as Doom 3 Scripts or GUIs.
 */
class CodeTokeniser : 
	public DefTokeniser
{
private:

	struct ParseNode
	{
		ArchiveTextFilePtr archive;
		std::istream inputStream;
		BasicDefTokeniser<std::istream> tokeniser;

		ParseNode(const ArchiveTextFilePtr& archive_) :
			archive(archive_),
			inputStream(&archive->getInputStream()),
			tokeniser(inputStream)
		{}
	};
	typedef boost::shared_ptr<ParseNode> ParseNodePtr;
	
	// The stack of child tokenisers
	typedef std::list<ParseNodePtr> NodeList;
	NodeList _nodes;

	NodeList::iterator _curNode;

public:

    /** 
     * Construct a CodeTokeniser with the given text file from the VFS.
     */
    CodeTokeniser(const ArchiveTextFilePtr& file)
    {
		_nodes.push_back(ParseNodePtr(new ParseNode(file)));
		_curNode = _nodes.begin();
	}
        
    bool hasMoreTokens()
	{
        return (_curNode != _nodes.end() && (*_curNode)->tokeniser.hasMoreTokens());
    }

    std::string nextToken()
	{
		if (hasMoreTokens())
		{
			// Inspect this token (TODO)
            return (*_curNode)->tokeniser.nextToken();
		}
        else
		{
            throw ParseException("CodeTokeniser: no more tokens");
		}
    }
    
};

} // namespace parser

#endif /* _CODE_TOKENISER_H_ */
