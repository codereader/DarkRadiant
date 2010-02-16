#include "XData.h"

namespace readable
{
	namespace
	{
		const int			MAX_PAGE_COUNT			= 20;
		const std::string	DEFAULT_TWOSIDED_LAYOUT = "guis/readables/books/book_calig_mac_humaine.gui";
		const std::string	DEFAULT_ONESIDED_LAYOUT = "guis/readables/sheets/sheet_paper_hand_nancy.gui";
		const std::string	DEFAULT_SNDPAGETURN		= "readable_page_turn";
	}

//XData implementations:
//->import:
	XDataPtrList XData::importXDataFromFile(const std::string& FileName)
	{
		/* ToDO:
			1) Proper error reporting. A summary should be displayed in the GUI later.*/

		XDataPtrList ReturnVector;
		boost::filesystem::path Path(FileName);

		//Check if file exists and its extension.
		if (Path.extension() != ".xd")
			reportError("[XData::importXDataFromFile] FileExtension is not .xd: " + FileName + "\n");
		if (!boost::filesystem::exists(Path))
			reportError("[XData::importXDataFromFile] Specified file does not exist: " + FileName + "\n");
		
		//Open File and check for sucess.
		boost::filesystem::ifstream file(Path, std::ios_base::in);
		if (file.is_open() == false)
			reportError("[XData::importXDataFromFile] Failed to open file: " + FileName + "\n");

		StringList ErrorList;
		parser::BasicDefTokeniser<std::istream> tok(file);
		unsigned int ErrorCount = 0;

		while (tok.hasMoreTokens())
		{
			XDataParse parsed = parseXDataDef(tok);
			if (parsed.xData)
				ReturnVector.push_back(parsed.xData);
			else
				ErrorCount += 1;
			if (parsed.error_msg.size()>0)
				for (unsigned int n=0; n<parsed.error_msg.size(); n++)
					ErrorList.push_back(parsed.error_msg[n]);
		}

		//temporary
		for (unsigned int n = 0; n<ErrorList.size(); n++)
			std::cerr << ErrorList[n];

		if (ErrorList.size() > 0)
			std::cerr << "[XData::importXDataFromFile] Import finished with " << ErrorList.size() << " error(s)/warning(s). " << ReturnVector.size() << " XData-definition(s) successfully imported, but failed to import at least " << ErrorCount << " definitions." << std::endl;

		file.close();
		return ReturnVector;
	} // XData::importXDataFromFile

	XDataParse XData::parseXDataDef(parser::DefTokeniser& tok)
	{
		/* TODO:
			1) Support for import-directive
			2) This is pretty ugly code. There's gotta be a better way to do this than with two separate shared pointers. 
				The default resize to MAX_PAGE_COUNT is also uncool.															->done
			3) Add try/catch blocks.																							->done
			4) Possibly throw syntax-exception if numPages hasn't been defined or reconstruct from the content-vectors or 
				set it automatically depending on the maxPageCount variable.													->done
			5) Try/catch blocks around direct vector access to prevent vector subscript out of range							->done
			6) Collect general syntax errors in a struct for a return-value... Also in subclass. Add function 
				that scrolls out to the next definition.																		->done
			7) Fix detection of exceeding content-dimensions.																	->done
			8) When guiPages is set, the other vectors should be resized as well.												->done
			9) Pages might be discarded during resize. Maintain a maxPageCount variable... Page-creation conditions possibly 
				have to be revisited. maxPageCount should only be raised for pages that have content.							->done
			10)If GuiPageCount is exceeded, the gui page should be stored anyway, because numPages might be raised later. 
				Resize the vector in the end and store it in the XData object. Errormessage should also be handled differently then.	->done
			11)Have to check the new virtual methods for possible exceptions.
			12)Only resize in the end depending on numPages and maxPageCount, to prevent dataloss.								->done
			13)ErrorMessage when numPages has been raised																		->done
			14)Maybe add a default guiPage layout for OneSided and TwoSided Objects if no guiPage layout has been defined.		->done
			15)Check if guiPage statements are available for every page.														->done
			16)nextToken() can throw if no tokens are available. Think of something to catch that.
			17)Check how the DefTokeniser reacts on escaped quotes.																->done
			18)Warnings for missing statements.																					->done*/

		std::string name = tok.nextToken();
		
		XDataParse NewXData;
		StringList guiPageError;
		int maxPageCount = 0;
		int maxGuiNumber = 0;
		std::string guiPageDef = "";

		int numPages = 0;
		std::string sndPageTurn = "";
		StringList guiPage;
		guiPage.resize(MAX_PAGE_COUNT,"");
		try	{ tok.assertNextToken("{"); }		//throws when syntax error
		catch (...)
		{
			while (tok.nextToken() != "{") {}
			NewXData.error_msg.push_back("[XData::importXDataFromFile] Syntaxerror in definition: " + name + ". '{' expected. Undefined behavior!\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
			jumpOutOfBrackets(tok,1);
			return NewXData;
		}

		std::string token = tok.nextToken();
		while (token != "}")
		{
			if (token.substr(0,4) == "page")
			{
				//Now that it is known, whether we are dealing with a Two- or OneSidedXDataObject, create the XData object.
				if (!NewXData.xData)
				{
					if (token.find("left",6) != std::string::npos || token.find("right",6) != std::string::npos) //TwoSided
					{
						NewXData.xData = XDataPtr(new TwoSidedXData(name));
						NewXData.xData->resizeVectors(MAX_PAGE_COUNT);
					}
					else //OneSided
					{
						NewXData.xData = XDataPtr(new OneSidedXData(name));
						NewXData.xData->resizeVectors(MAX_PAGE_COUNT);
					}
				}

				//Check Syntax
				tok.skipTokens(1);
				try { tok.assertNextToken("{"); }		//throws when syntax error
				catch (...)
				{
					NewXData.error_msg.push_back("[XData::importXDataFromFile] Syntax error in definition: " + name + ", " + token +" statement. '{' expected. Undefined behavior!\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
					jumpOutOfBrackets(tok,1);
					NewXData.xData.reset();
					return NewXData;
				}

				//Acquire PageIndex
				int PageIndex;
				char number = token.c_str()[4];
				try { PageIndex = boost::lexical_cast<int>(number)-1; }	//can throw...
				catch (...)
				{
					NewXData.error_msg.push_back("[XData::importXDataFromFile] Error in definition: " + name + ", " + token + " statement. '" + number + "' is not a number.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");;
					jumpOutOfBrackets(tok,2);
					NewXData.xData.reset();
					return NewXData;
				}

				//Check PageIndex-Range
				std::string content = parseText(tok);
				if (PageIndex >= numPages )
				{
					if (content.length() > 1) //numPages is raised automatically, if a page with content is detected...		//unclean
					{
						numPages = PageIndex+1;
						NewXData.error_msg.push_back("[XData::importXDataFromFile] Warning for definition: " + name + ", " + token + " statement.\n\tnumPages not (yet) specified or too low. Raising numPages to " + number +"...\n");
					}
					else
					{
						NewXData.error_msg.push_back("[XData::importXDataFromFile] Warning for definition: " + name + ", " + token + " statement.\n\tnumPages not (yet) specified or too low. However, this page does not contain any text and is discarded...\n");
						token = tok.nextToken();
						continue;
					}
				}

				//Refresh the maxPageCount variable if this page has content.
				if (content.length() > 1)		//unclean
					if (maxPageCount < PageIndex+1)
						maxPageCount = PageIndex+1;

				//Write the content into the XData object
				SideChooser side;
				if (token.find("left",6) != std::string::npos)	//SideChooser is discarded on OneSidedXData...
					side = Left;
				else
					side = Right;
				if (token.find("body",6) != std::string::npos)
					NewXData.xData->setContent(Body, PageIndex, side, content);
				else
					NewXData.xData->setContent(Title, PageIndex, side, content);
			} //end: page
			else if (token.substr(0,8) == "gui_page")
			{
				tok.skipTokens(1);
				std::string number = token.substr(token.length()-1,1);
				int guiNumber;
				try	{ guiNumber = boost::lexical_cast<int>(number)-1; }
				catch (...)
				{
					NewXData.error_msg.push_back("[XData::importXDataFromFile] Error in definition: " + name + ", gui_page statement. '" + number + "' is not a number.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
					jumpOutOfBrackets(tok,1);
					NewXData.xData.reset();
					return NewXData;	
				}
				if (maxGuiNumber < guiNumber)
					maxGuiNumber = guiNumber;
				guiPageDef = tok.nextToken();
				guiPage[ guiNumber ] = guiPageDef;		//could throw if guiNumber >= MAX_PAGE_COUNT
				if (guiNumber >= numPages)
					guiPageError.push_back("[XData::importXDataFromFile] Warning for definition: " + name + ". More guiPage statements, than pages. Discarding statement for Page " + number + ".\n");
			}
			else if (token == "num_pages")
			{
				tok.skipTokens(1);
				std::string number = tok.nextToken();
				try { numPages = boost::lexical_cast<int>(number); }	//throws
				catch(...)
				{
					NewXData.error_msg.push_back("[XData::importXDataFromFile] Error in definition: " + name + ", num_pages statement. '" + number + "' is not a number.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
					jumpOutOfBrackets(tok,1);
					NewXData.xData.reset();
					return NewXData;
				}
				if (maxPageCount > numPages) //corrects a faulty numPages value
				{
					numPages = maxPageCount;
					NewXData.error_msg.push_back("[XData::importXDataFromFile] Warning for definition: " + name
						+ ". The specified numPages statement did not match the amount of pages with content.\n\tnumPages is set to " 
						+ boost::lexical_cast<std::string>(numPages) + ".\n");
				}
			}
			else if (token == "import")			//Not yet supported...
			{
				//importDirective(tok, NewXData, name);	//!!!!!!!!!!!!!!!!!!

				NewXData.error_msg.push_back("[XData::importXDataFromFile] Error in definition: " + name + ". Found an import-statement, which is not yet supported.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
				jumpOutOfBrackets(tok,1);
				NewXData.xData.reset();
				return NewXData;
			}
			else if (token == "snd_page_turn")
			{
				tok.skipTokens(1);
				sndPageTurn = tok.nextToken();
			}
			
			token = tok.nextToken();
		}

		//cleaning up:
		if ( maxGuiNumber+1 > numPages)		//Append missing GUI-errormessages... Until now, it wasn't clear how many guipages are actually discarded.
		{
			int diff = maxGuiNumber + 1 - maxPageCount;
			for (unsigned int n = guiPageError.size()-diff; n<guiPageError.size(); n++)
			{
				NewXData.error_msg.push_back(guiPageError[n]);
			}
		}//*/
		// Check if guiPage-statements for all pages are available.
		if (guiPageDef == "")
		{
			NewXData.error_msg.push_back("[XData::importXDataFromFile] Warning for definition: " + name
				+ ". guiPage-statement(s) missing. Setting default value...\n");
			if (NewXData.xData->getPageLayout() == TwoSided)
				guiPageDef = DEFAULT_TWOSIDED_LAYOUT; 
			else
				guiPageDef = DEFAULT_ONESIDED_LAYOUT;
		}
		for (int n=0; n<numPages; n++ )
		{
			if (guiPage[n] == "")
				guiPage[n] = guiPageDef;
		}//*/
		NewXData.xData->_guiPage = guiPage;
		NewXData.xData->_numPages = numPages;
		if (sndPageTurn == "")
		{
			NewXData.xData->_sndPageTurn = DEFAULT_SNDPAGETURN;
			NewXData.error_msg.push_back("[XData::importXDataFromFile] Warning for definition: " + name
				+ ". sndPageTurn-statement missing. Setting default value...\n");
		}
		else
			NewXData.xData->_sndPageTurn = sndPageTurn;

		NewXData.xData->resizeVectors(numPages);

		return NewXData;
	}

	std::string XData::parseText(parser::DefTokeniser& tok)
	{
		std::stringstream out;
		std::string token = tok.nextToken();
		while (tok.hasMoreTokens() && token != "}")
		{
			if (token.c_str()[token.length()-1] == '\\')	//support for escaped quotes in texts...
				token = token.substr(0,token.length()-1) + "\"";
			else
				token = token + "\n";
			out << token;	
			token = tok.nextToken();
		}
		return out.str();
	}


	void XData::importDirective(parser::DefTokeniser& tok, XDataParse& NewXData, const std::string name)
	{
		//WIP
		std::string token;
		try { tok.assertNextToken("{");	}	//can throw
		catch(...)
		{
			NewXData.error_msg.push_back("[XData::importXDataFromFile] Syntax error in definition: " + name + ", import-statement. '{' expected. Undefined behavior!\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
			jumpOutOfBrackets(tok,1);
			NewXData.xData.reset();
			//return NewXData;		//replace with throwing??
		}
		StringList SourceStatement;
		StringList DestStatement;
		token = tok.nextToken();
		while (token != "}")
		{
			SourceStatement.push_back(token);
			tok.skipTokens(1);	//Should be "->"
			DestStatement.push_back(tok.nextToken());
			token = tok.nextToken();
		}
		try { tok.assertNextToken("from"); }
		catch (...)
		{
			NewXData.error_msg.push_back("[XData::importXDataFromFile] Syntax error in definition: " + name + ", import-statement. 'from' expected. Undefined behavior!\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
			jumpOutOfBrackets(tok,1);
			NewXData.xData.reset();
			//return NewXData;		//replace with throwing??
		}
		std::string SourceDef = tok.nextToken();

		StringMap::iterator it = getDefMap().find(SourceDef);
		if (it != getDefMap().end())
		{
			std::string FileName = it->second;

			/*	-VFS: open file...
				-Search definition and cut it to a stringstream. (No DefTokeniser)
				-Use the DefTokeniser to identify the statements and grab their contents. Store them in the Source-vector or a temporary string.
				-Identify where the elements should go and store them.*/
		}
		else
		{
			//Definition has not been found: throw/return something
		}
	}

	StringMap& XData::getDefMap()
	{
		static StringMap XDataDefMap = grabAllDefinitions();
		return XDataDefMap;
	}

	StringMap XData::grabAllDefinitions()
	{
		//WIP
		//Traverse all xd files, parse all Definition names as the key value and their file as the mapped value
		StringMap temp;
		return temp;
	}


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
					int DefPos = String.find(_name,0);
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
		//boost::filesystem::ifstream file(Path, std::ios_base::in);
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

	void TwoSidedXData::setContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side, const std::string& content)
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

	std::string TwoSidedXData::getContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side)
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

	void OneSidedXData::setContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side, const std::string& content)
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

	std::string OneSidedXData::getContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side)
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