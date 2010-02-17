#include "XData.h"

namespace readable
{
//XData implementations:
//->export:	
	FileStatus XData::xport(const std::string& FileName, const ExporterCommands& cmd)
	{
		/* ToDo:
			1) Need to handle return 0 of getDefLength().			->done
			2) Test all possibilities.								->done
			3) Do I need to check fstream.open for success, as well as the write process?	->done
			4) Does String support enough characters? 
			5) Handle the exception of getDefinitionNameFromXD().	->done */

		boost::filesystem::path Path(FileName);		
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
					int DefPos = String.find(_name);
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
						std::cerr << "[XData::xport] Syntax error in file " << FileName << ". Overwriting the file..." << std::endl;
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
		boost::filesystem::ofstream file(Path, std::ios_base::out | std::ios_base::trunc);		//check if file open was successful?
		if (!file.is_open())
			return OpenFailed;
		file << generateXDataDef();
		file.close();

		return AllOk;
	}


	std::string XData::generateXDataDef()
	{
		//ToDo: 1) Howto handle '"' in String?				->done
		//		2) Non-shared_ptr allowed in this case?		->removed.
		//		3) Possibly check if e.g. the vectorsize of TwoSidedXD->_pageLeftTitle is smaller than _numPages.
		//			So that no exceptions are thrown. (Depends on how XData objects are generated. Basically all
		//			vectors should be of the size _numPages)	->fixed by resizeVectors-method

		std::stringstream xDataDef;
		xDataDef << _name << "\n" << "{" << "\n" << "\tprecache" << "\n" << "\t\"num_pages\"\t: \"" << _numPages << "\"\n";

		std::stringstream ss;
		std::string TempString;

		xDataDef << getContentDef();

		for (int n=1; n<=_numPages; n++)
		{
			xDataDef << "\t\"gui_page" << n << "\"\t: \"" << _guiPage[n-1] << "\"\n";
		}
		xDataDef << "\t\"snd_page_turn\"\t: \"" << _sndPageTurn << "\"\n}\n\n\n\n";//*/

		return xDataDef.str();		//Does this support enough characters??
	}

	int XData::getDefLength(boost::filesystem::fstream& file)
	{
		/* ToDo:
			1) Have to check if getpointer is raised after get() or before get() with reference to the returnvalue.		->done*/
		char ch;
		int bla;
		if (file.is_open())
		while (!file.eof())
		{
			bla = file.tellg();
			ch = file.get();
			if (ch == '{')
			{
				int BracketCount = 1;
				while (!file.eof() && BracketCount > 0)
				{
					ch = file.get();
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

	std::string XData::getDefinitionNameFromXD(boost::filesystem::ifstream& file)
	{
		/* ToDo:
			1) Better use assertNextToken("{") here and let the caller catch.	->done*/
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

	std::string XData::generateTextDef(std::string String)
	{
		/* ToDo:
			1) Possibly check if Quotecount per line is even and warn otherwise, because uneven quotecounts seem to be discarded.*/
		std::stringstream ss;
		std::stringstream xDataDef;
		std::string TempString;
		xDataDef << "\t{\n";
		if (String != "")
		{
			ss << String;
			while ( std::getline(ss, TempString) )	//replace "\n" and add an escape-character before quotes.
			{
				int QuotePos = -2;
				while ( (QuotePos = TempString.find("\"",QuotePos+2) ) != std::string::npos )
				{
					TempString.insert(QuotePos, "\\");
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
	void XData::resizeVectors(const int& TargetSize)
	{
		_guiPage.resize(TargetSize, "");
	}
	
	void XData::jumpOutOfBrackets(parser::DefTokeniser& tok, int CurrentDepth)	//not tested.
	{
		while ( tok.hasMoreTokens() && CurrentDepth > 0)
		{
			std::string token = tok.nextToken();
			if (token == "{")
				CurrentDepth += 1;
			else if (token == "}")
				CurrentDepth -= 1;
		}
	}

//TwoSidedXData implementations:

	void TwoSidedXData::resizeVectors(const int& TargetSize)
	{
		XData::resizeVectors(TargetSize);
		_pageLeftBody.resize(TargetSize, "");
		_pageLeftTitle.resize(TargetSize, "");
		_pageRightBody.resize(TargetSize, "");
		_pageRightTitle.resize(TargetSize, "");
	}

	void TwoSidedXData::setPageContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side, const std::string& content)
	{
		switch (cc)
		{
		case Title:
			switch (Side)
			{
			case Left:
				_pageLeftTitle[PageIndex] = content;
				break;
			default:
				_pageRightTitle[PageIndex] = content;
			}
			break;
		default:
			switch (Side)
			{
			case Left:
				_pageLeftBody[PageIndex] = content;
				break;
			default:
				_pageRightBody[PageIndex] = content;
			}
		}
	}

	std::string TwoSidedXData::getPageContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side)
	{
		switch (cc)
		{
		case Title:
			switch (Side)
			{
			case Left:
				return _pageLeftTitle[PageIndex];
			default:
				return _pageRightTitle[PageIndex];
			}
		default:
			switch (Side)
			{
			case Left:
				return _pageLeftBody[PageIndex];
			default:
				return _pageRightBody[PageIndex];
			}
		}
	}

	std::string TwoSidedXData::getContentDef()
	{
		std::stringstream xDataDef;

		for (int n = 0; n < _numPages; n++)
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

	void OneSidedXData::resizeVectors(const int& TargetSize)
	{
		XData::resizeVectors(TargetSize);
		_pageBody.resize(TargetSize, "");
		_pageTitle.resize(TargetSize, "");
	}

	void OneSidedXData::setPageContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side, const std::string& content)
	{
		switch (cc)
		{
		case Title:
			_pageTitle[PageIndex] = content;
			break;
		case Body:
		default:
			_pageBody[PageIndex] = content;
		}
	}

	std::string OneSidedXData::getPageContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side)
	{
		switch (cc)
		{
		case Title:
			return _pageTitle[PageIndex];
		default:
			return _pageBody[PageIndex];
		}
	}

	std::string OneSidedXData::getContentDef()
	{
		std::stringstream xDataDef;

		for (int n = 0; n < _numPages; n++)
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