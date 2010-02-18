#include "XData.h"

namespace readable
{
//XData implementations:
//->export:	
	readable::FileStatus XData::xport( const std::string& filename, ExporterCommand cmd )
	{
		boost::filesystem::path Path(filename);		
		if (boost::filesystem::exists(Path))
		{
			switch (cmd)
			{
			case Merge:
				{
				//Check if definition already exists and return DefinitionExists. If it does not, append the definition to the file.
					boost::filesystem::fstream file(Path, std::ios_base::in | std::ios_base::out | std::ios_base::app);
					if (!file.is_open())
						return OpenFailed;
					std::stringstream ss;
					ss << file.rdbuf();		//is this the quickest way to read a whole file?
					std::string String = ss.str();
					std::size_t DefPos = String.find(_name);
					while (DefPos != std::string::npos)	//A name of a readable could be contained in another readable's name. Check that...
					{
						char before = String.c_str()[DefPos-1];		//what happens if -1 is accessed? 
						char after = String.c_str()[DefPos+_name.length()];
						if ((DefPos == 0 || before == ' ' || before == '\t' || before == '\n') && (DefPos+_name.length() == file.end || after == ' ' || after == '\t' || after == '\n'))	//other delimiters necessary?
						{
							definitionStart = DefPos;
							file.close();
							return DefinitionExists;
						}
						DefPos = String.find(_name,DefPos+_name.length());
					}
					file << generateXDataDef();
					file.close();
					return AllOk;
				}
			case MergeOverwriteExisting: 
				{	
				//Find the old definition in the target file and delete it. Append the new definition.
				//definitionStart has been set in the first iteration of this method.
					boost::filesystem::fstream file(Path, std::ios_base::in);
					if (!file.is_open())
						return OpenFailed;
					file.seekg(definitionStart);
					int DefLength = getDefLength(file);
					if (DefLength == 0)		//If the definitionlength can't be obtained, the merge fails.
						return MergeFailed;
					file.seekg(0);
					std::stringstream ss;
					ss << file.rdbuf();
					file.close();
					std::string OutString = ss.str();							//Does string support enough characters for this operation?
					OutString.erase(definitionStart, DefLength);
					OutString.insert(definitionStart, generateXDataDef());
					file.open(Path, std::ios_base::out | std::ios_base::trunc);
					if (!file.is_open())
						return OpenFailed;
					file << OutString;
					file.close();
					return AllOk;
				}
			case Overwrite: 
				{
				//Warn if the target file contains multiple definitions: return MultipleDefinitions. 
				//Warn if the definition in the target file does not match the current definition: return DefinitionMisMatch
				//else overwrite existing file.
					std::string DefName;
					boost::filesystem::ifstream file(Path, std::ios_base::in);
					if (!file.is_open())
						return OpenFailed;
					try	{ DefName = getDefinitionNameFromXD(file); }
					catch (...) 
					{
						std::cerr << "[XData::xport] Syntax error in file " << filename << ". Overwriting the file..." << std::endl;
						break;
					}
					if (DefName == _name)	//Definition will be overwritten...
						break;
					else if (DefName == "")	//MultipleDefinitions exist
						return MultipleDefinitions;
					return DefinitionMismatch;
				}
			case OverwriteMultDef:
				//Replace the file no matter what.
				break;
			default: return FileExists;
			}
		}

		//Write the definition into the file.
		boost::filesystem::ofstream file(Path, std::ios_base::out | std::ios_base::trunc);
		if (!file.is_open())
			return OpenFailed;
		file << generateXDataDef();
		file.close();

		return AllOk;
	}


	const std::string XData::generateXDataDef() const
	{
		std::stringstream xDataDef;
		xDataDef << _name << "\n" << "{" << "\n" << "\tprecache" << "\n" << "\t\"num_pages\"\t: \"" << _numPages << "\"\n";

		std::stringstream ss;
		std::string TempString;

		xDataDef << getContentDef();

		for (std::size_t n=0; n<_numPages; n++)
		{
			xDataDef << "\t\"gui_page" << n << "\"\t: \"" << _guiPage[n] << "\"\n";
		}
		xDataDef << "\t\"snd_page_turn\"\t: \"" << _sndPageTurn << "\"\n}\n\n\n\n";//*/

		return xDataDef.str();		//Does this support enough characters??
	}

	const int XData::getDefLength(boost::filesystem::fstream& file) const
	{
		char ch;
		int bla;
		if (file.is_open())
		while (!file.eof())
		{
			bla = file.tellg();
			ch = (char)file.get();
			if (ch == '{')
			{
				int BracketCount = 1;
				while (!file.eof() && BracketCount > 0)
				{
					ch = (char)file.get();
					if (ch == '{')
						BracketCount += 1;
					else if (ch == '}')
						BracketCount -= 1;
				}
				return ((int)file.tellg() - definitionStart);
			}
		}
		return 0;	//no appropriate bracketstructure was found.
	}

	const std::string XData::getDefinitionNameFromXD(boost::filesystem::ifstream& file) const
	{
		std::string ReturnString;
		parser::BasicDefTokeniser<std::istream> tok(file);
		bool FirstDef = true;
		while (tok.hasMoreTokens())
		{
			if (FirstDef)
				ReturnString = tok.nextToken();
			else
				return "";
			FirstDef = false;
			tok.assertNextToken("{");
			jumpOutOfBrackets(tok, 1);
		}
		return ReturnString;
	}

	const std::string XData::generateTextDef(const std::string& rawString) const
	{
		/* ToDo:
			1) Possibly check if Quotecount per line is even and warn otherwise, because uneven quotecounts seem to be discarded.*/
		std::stringstream ss;
		std::stringstream xDataDef;
		std::string TempString;
		xDataDef << "\t{\n";
		if (rawString != "")
		{
			ss << rawString;
			while ( std::getline(ss, TempString) )	//replace "\n" and add an escape-character before quotes.
			{
				std::size_t QuotePos = TempString.find("\"",0);
				while ( QuotePos != std::string::npos )
				{
					TempString.insert(QuotePos, "\\");
					QuotePos = TempString.find("\"",QuotePos+2);
				}
				xDataDef << "\t\t\"" << TempString << "\"\n";
			}
			xDataDef << "\t}\n";
		}
		else
			xDataDef << "\t\t\"\"\n\t}\n";
		return xDataDef.str();
	}
//->general:
	void XData::resizeVectors(std::size_t targetSize)
	{
		_guiPage.resize(targetSize, "");
	}
	
	void XData::jumpOutOfBrackets(parser::DefTokeniser& tok, int currentDepth) const
	{
		while ( tok.hasMoreTokens() && currentDepth > 0)
		{
			std::string token = tok.nextToken();
			if (token == "{")
				currentDepth += 1;
			else if (token == "}")
				currentDepth -= 1;
		}
	}

//TwoSidedXData implementations:

	void TwoSidedXData::resizeVectors(std::size_t targetSize)
	{
		XData::resizeVectors(targetSize);
		_pageLeftBody.resize(targetSize, "");
		_pageLeftTitle.resize(targetSize, "");
		_pageRightBody.resize(targetSize, "");
		_pageRightTitle.resize(targetSize, "");
	}

	void TwoSidedXData::setPageContent(ContentType cc, std::size_t pageIndex, Side side, const std::string& content)
	{
		if (pageIndex >= _numPages)
			throw std::runtime_error("Page Index out of bounds.");
		switch (cc)
		{
		case Title:
			switch (side)
			{
			case Left:
				_pageLeftTitle[pageIndex] = content;
				break;
			default:
				_pageRightTitle[pageIndex] = content;
			}
			break;
		default:
			switch (side)
			{
			case Left:
				_pageLeftBody[pageIndex] = content;
				break;
			default:
				_pageRightBody[pageIndex] = content;
			}
		}
	}

	const std::string& TwoSidedXData::getPageContent(ContentType cc, std::size_t pageIndex, Side side) const
	{
		if (pageIndex >= _numPages)
			throw std::runtime_error("Page Index out of bounds.");
		switch (cc)
		{
		case Title:
			switch (side)
			{
			case Left:
				return _pageLeftTitle[pageIndex];
			default:
				return _pageRightTitle[pageIndex];
			}
		default:
			switch (side)
			{
			case Left:
				return _pageLeftBody[pageIndex];
			default:
				return _pageRightBody[pageIndex];
			}
		}
	}

	const std::string TwoSidedXData::getContentDef() const
	{
		std::stringstream xDataDef;
		for (std::size_t n = 0; n < _numPages; n++)
		{
			//Page Left Title
			xDataDef << "\t\"page" << n+1 << "_left_title\"\t:\n";
			xDataDef << generateTextDef(_pageLeftTitle[n]);

			//Page Left Body
			xDataDef << "\t\"page" << n+1 << "_left_body\"\t:\n";
			xDataDef << generateTextDef(_pageLeftBody[n]);

			//Page Right Title
			xDataDef << "\t\"page" << n+1 << "_right_title\"\t:\n";
			xDataDef << generateTextDef(_pageRightTitle[n]);

			//Page Right Body
			xDataDef << "\t\"page" << n+1 << "_right_body\"\t:\n";
			xDataDef << generateTextDef(_pageRightBody[n]);
		}
		return xDataDef.str();
	}

//OneSidedXData implementations:

	void OneSidedXData::resizeVectors(std::size_t targetSize)
	{
		XData::resizeVectors(targetSize);
		_pageBody.resize(targetSize, "");
		_pageTitle.resize(targetSize, "");
	}

	void OneSidedXData::setPageContent(ContentType cc, std::size_t pageIndex, Side side, const std::string& content)
	{
		if (pageIndex >= _numPages)
			throw std::runtime_error("Page Index out of bounds.");
		switch (cc)
		{
		case Title:
			_pageTitle[pageIndex] = content;
			break;
		case Body:
		default:
			_pageBody[pageIndex] = content;
		}
	}

	const std::string& OneSidedXData::getPageContent(ContentType cc, std::size_t pageIndex, Side side) const
	{
		if (pageIndex >= _numPages)
			throw std::runtime_error("Page Index out of bounds.");
		switch (cc)
		{
		case Title:
			return _pageTitle[pageIndex];
		default:
			return _pageBody[pageIndex];
		}
	}

	const std::string OneSidedXData::getContentDef() const
	{
		std::stringstream xDataDef;

		for (std::size_t n = 0; n < _numPages; n++)
		{
			//Title
			xDataDef << "\t\"page" << n+1 << "_title\"\t:\n";
			xDataDef << generateTextDef(_pageTitle[n]);

			//Body
			xDataDef << "\t\"page" << n+1 << "_body\"\t:\n";
			xDataDef << generateTextDef(_pageBody[n]);
		}

		return xDataDef.str();
	}

} // namespace readable