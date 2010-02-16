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
 *
 * Note: Don't expect this tokeniser to be particularly fast.
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

		ParseNode(const ArchiveTextFilePtr& archive_,
					const char* delims, 
					const char* keptDelims) :
			archive(archive_),
			inputStream(&archive->getInputStream()),
			tokeniser(inputStream, delims, keptDelims)
		{}
	};
	typedef boost::shared_ptr<ParseNode> ParseNodePtr;
	
	// The stack of child tokenisers
	typedef std::list<ParseNodePtr> NodeList;
	NodeList _nodes;

	NodeList::iterator _curNode;

	// The next token which is not a pre-processor token
	std::string _nextToken;

	// A set of visited files to catch infinite include loops
	typedef std::set<std::string> FileSet;
	FileSet _visitedFiles;

	typedef std::map<std::string, std::string> DefinitionMap;
	DefinitionMap _definitions;

	const char* _delims;
	const char* _keptDelims;

public:

    /** 
     * Construct a CodeTokeniser with the given text file from the VFS.
     */
	CodeTokeniser(const ArchiveTextFilePtr& file,
				  const char* delims = " \t\n\v\r", 
				  const char* keptDelims = "{}()") :
		_delims(delims),
		_keptDelims(keptDelims)
    {
		_nodes.push_back(ParseNodePtr(new ParseNode(file, _delims, _keptDelims)));
		_curNode = _nodes.begin();

		_visitedFiles.insert(file->getName());

		loadNextRealToken();
	}
        
    bool hasMoreTokens()
	{
		return !_nextToken.empty();
    }

    std::string nextToken()
	{
		std::string temp = _nextToken;

		loadNextRealToken();

		return temp;
    }
    
private:
	void loadNextRealToken()
	{
		while (_curNode != _nodes.end())
		{
			if (!(*_curNode)->tokeniser.hasMoreTokens())
			{
				++_curNode;
			}

			if (_curNode == _nodes.end())
			{
				_nextToken.clear();
			}

			_nextToken = (*_curNode)->tokeniser.nextToken();

			if (!_nextToken.empty() && _nextToken[0] == '#')
			{
				// A pre-processor token is ahead
				handlePreprocessorToken();
				continue;
			}

			// Found a non-preprocessor token

			// Check if this is matching a preprocessor definition
			DefinitionMap::const_iterator found = _definitions.find(_nextToken);
			
			if (found != _definitions.end())
			{
				_nextToken = found->second;
			}

			break;
		}
	}

	void handlePreprocessorToken()
	{
		if (_nextToken == "#include")
		{
			std::string includeFile = (*_curNode)->tokeniser.nextToken();

			ArchiveTextFilePtr file = GlobalFileSystem().openTextFile(includeFile);

			if (file != NULL)
			{
				// Catch infinite recursions
				std::pair<FileSet::iterator, bool> result = _visitedFiles.insert(file->getName());

				if (result.second)
				{
					// Push a new parse node and switch
					_curNode = _nodes.insert(
						_curNode, 
						ParseNodePtr(new ParseNode(file, _delims, _keptDelims))
					);
				}
				else
				{
					globalErrorStream() << "Caught infinite loop on parsing #include token: " 
						<< includeFile << " in " << (*_curNode)->archive->getName() << std::endl;
				}
			}
			else
			{
				globalWarningStream() << "Couldn't find include file: " 
					<< includeFile << " in " << (*_curNode)->archive->getName() << std::endl;
			}
		}
		else if (_nextToken == "#define")
		{
			std::string key = (*_curNode)->tokeniser.nextToken();
			std::string value = (*_curNode)->tokeniser.nextToken();

			std::pair<DefinitionMap::iterator, bool> result = _definitions.insert(
				DefinitionMap::value_type(key, value)
			);

			if (!result.second)
			{
				globalWarningStream() << "Redefinition of " << key 
					<< " in " << (*_curNode)->archive->getName() << std::endl;
			}
		}
		else if (_nextToken == "#undef")
		{
			std::string key = (*_curNode)->tokeniser.nextToken();
			_definitions.erase(key);
		}
		else if (_nextToken == "#ifdef")
		{
			std::string key = (*_curNode)->tokeniser.nextToken();
			DefinitionMap::const_iterator found = _definitions.find(key);

			if (found == _definitions.end())
			{
				skipUntilMatchingEndif();
			}
		}
		else if (_nextToken == "#ifndef")
		{
			DefinitionMap::const_iterator found = _definitions.find(
				(*_curNode)->tokeniser.nextToken());

			if (found != _definitions.end())
			{
				skipUntilMatchingEndif();
			}
		}
		else if (_nextToken == "#if")
		{
			(*_curNode)->tokeniser.skipTokens(1);
		}
	}

	void skipUntilMatchingEndif()
	{
		// Not defined, skip everything until matching #endif
		for (std::size_t level = 1; level > 0;)
		{
			if (!(*_curNode)->tokeniser.hasMoreTokens())
			{
				globalWarningStream() << "No matching #endif for #ifdef in "
					<< (*_curNode)->archive->getName() << std::endl;
			}

			std::string token = (*_curNode)->tokeniser.nextToken();

			if (token == "#endif")
			{
				level--;
			}
			else if (token == "#ifdef" || token == "#ifndef" || token == "#if")
			{
				level++;
			}
		}
	}
};

} // namespace parser

#endif /* _CODE_TOKENISER_H_ */
