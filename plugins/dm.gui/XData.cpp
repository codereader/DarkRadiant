#include "XData.h"

namespace readable
{
	namespace
	{
		const int MAX_PAGE_COUNT = 20;
		const std::string DEFAULT_TWOSIDED_LAYOUT = "guis/readables/books/book_calig_mac_humaine.gui";
		const std::string DEFAULT_ONESIDED_LAYOUT = "guis/readables/sheets/sheet_paper_hand_nancy.gui";
		const std::string DEFAULT_SNDPAGETURN = "readable_page_turn";
	}

//XData implementations:

	XDataPtrList XData::importXDataFromFile(const std::string& FileName)
	{
		/* ToDO:
			1) Proper error reporting. */

		XDataPtrList ReturnVector;
		boost::filesystem::path Path(FileName);

		//Check if file exists and its extension.
		if (Path.extension() != ".xd")
		{
			reportError("[XDataManager::importXData] FileExtension is not .xd: " + FileName + "\n");
		}
		if (!boost::filesystem::exists(Path))
		{
			reportError("[XDataManager::importXData] Specified file does not exist: " + FileName + "\n");
		}
		
		//Open File and check for sucess.
		boost::filesystem::ifstream file(Path, std::ios_base::in);
		if (file.is_open() == false)
		{
			reportError("[XDataManager::importXData] Failed to open file: " + FileName + "\n");
		}

		StringList ErrorList;

		parser::BasicDefTokeniser<std::istream> tok(file);

		while (tok.hasMoreTokens())
		{
			XDataParse parsed = parseXDataDef(tok);
			if (parsed.xData)
				ReturnVector.push_back(parsed.xData);
			if (parsed.error_msg.size()>0)
				for (unsigned int n=0; n<parsed.error_msg.size(); n++)
					ErrorList.push_back(parsed.error_msg[n]);
		}

		//temporary
		for (unsigned int n = 0; n<ErrorList.size(); n++)
			std::cerr << ErrorList[n];

		if (ErrorList.size() > 0)
			std::cerr << "[XDataManager::importXData] Import finished with " << ErrorList.size() << " errors/warnings. " << ReturnVector.size() << " XData-definitions imported." << std::endl;

		file.close();
		return ReturnVector;
	} // XDataManager::importXData

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
			15)Check if guiPage statements are available for every page.														->done*/

		std::string name = tok.nextToken();
		
		XDataParse NewXData;
		StringList guiPageError;
		int maxPageCount = 0;
		int maxGuiNumber = 0;
		std::string guiPageDef = "";

		int numPages = 0;
		std::string sndPageTurn = DEFAULT_SNDPAGETURN;
		StringList guiPage;
		guiPage.resize(MAX_PAGE_COUNT,"");
		try
		{
			tok.assertNextToken("{");		//throws when syntax error
		}
		catch (...)
		{
			NewXData.error_msg.push_back("[XDataManager::importXData] Syntax error in definition: " + name + ". '{' expected. Jumping to next XData definition...\n");
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
				try
				{
					tok.assertNextToken("{");		//throws when syntax error
				}
				catch (...)
				{
					NewXData.error_msg.push_back("[XDataManager::importXData] Syntax error in definition: " + name + ", " + token +" statement. '{' expected. Jumping to next XData definition...\n");
					jumpOutOfBrackets(tok,1);
					NewXData.xData.reset();
					return NewXData;
				}

				//Acquire PageIndex
				int PageIndex;
				char number = token.c_str()[4];
				try
				{
					PageIndex = boost::lexical_cast<int>(number)-1;	//can throw...
				}
				catch (...)
				{
					NewXData.error_msg.push_back("[XDataManager::importXData] Error in definition: " + name + ", " + token + " statement. '" + number + "' is not a number. Jumping to next XData definition...\n");;
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
						NewXData.error_msg.push_back("[XDataManager::importXData] Error in definition: " + name + ", " + token + " statement.\n\tnumPages does not match the actual amount of pages defined in this XData-Def. Raising numPages to " + number +"...\n");
					}
					else
					{
						NewXData.error_msg.push_back("[XDataManager::importXData] Error in definition: " + name + ", " + token + " statement.\n\tnumPages does not match the actual amount of pages defined in this XData-Def. However, this page does not contain any text and is discarded...\n");
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
				try
				{
					guiNumber = boost::lexical_cast<int>(number)-1;
				}
				catch (...)
				{
					NewXData.error_msg.push_back("[XDataManager::importXData] Error in definition: " + name + ", gui_page statement. '" + number + "' is not a number. Jumping to next XData definition...\n");
					jumpOutOfBrackets(tok,1);
					NewXData.xData.reset();
					return NewXData;	
				}
				if (maxGuiNumber < guiNumber)
					maxGuiNumber = guiNumber;
				guiPageDef = tok.nextToken();
				guiPage[ guiNumber ] = guiPageDef;		//could throw if guiNumber >= MAX_PAGE_COUNT
				if (guiNumber >= numPages)
					guiPageError.push_back("[XDataManager::importXData] Error in definition: " + name + ". More guiPage statements, than pages. Discarding statement for Page " + number + ".\n");
			}
			else if (token == "num_pages")
			{
				tok.skipTokens(1);
				std::string number = tok.nextToken();
				try
				{					
					numPages = boost::lexical_cast<int>(number);	//throws
				}
				catch(...)
				{
					NewXData.error_msg.push_back("[XDataManager::importXData] Error in definition: " + name + ", num_pages statement. '" + number + "' is not a number. Jumping to next XData definition...\n");
					jumpOutOfBrackets(tok,1);
					NewXData.xData.reset();
					return NewXData;
				}
			}
			else if (token == "import")			//Not yet supported...
			{
				NewXData.error_msg.push_back("[XDataManager::importXData] Error in definition: " + name + ". Found an import-statement, which is not yet supported. Jumping to next definition...\n");
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
		if (maxPageCount > numPages) //corrects a faulty numPages value
		{
			numPages = maxPageCount;
			NewXData.error_msg.push_back("[XDataManager::importXData] Error in definition: " + name
				+ ". The specified numPages statement did not match the amount of pages with content.\n\tnumPages has been raised to " 
				+ boost::lexical_cast<std::string>(numPages) + ".");
		}
		if ( maxGuiNumber+1 > numPages)		//Append missing GUI-errormessages... Until now, it wasn't clear how many guipages are actually discarded.
		{
			int diff = maxGuiNumber + 1 - maxPageCount;
			for (unsigned int n = guiPageError.size()-diff; n<guiPageError.size(); n++)
			{
				NewXData.error_msg.push_back(guiPageError[n]);
			}
		}
		// Check if guiPage-statements for all pages are available.
		if (guiPageDef == "")
		{
			if (NewXData.xData->getPageLayout() == TwoSided)
				guiPageDef = DEFAULT_TWOSIDED_LAYOUT; 
			else
				guiPageDef = DEFAULT_ONESIDED_LAYOUT;
		}
		for (int n=0; n<numPages; n++ )
		{
			if (guiPage[n] == "")
				guiPage[n] = guiPageDef;
		}
		NewXData.xData->_guiPage = guiPage;
		NewXData.xData->_numPages = numPages;
		NewXData.xData->_sndPageTurn = sndPageTurn;


		NewXData.xData->resizeVectors(numPages);

		return NewXData;
	}

	std::string XData::parseText(parser::DefTokeniser& tok)
	{
		std::stringstream out;
		std::string token = tok.nextToken();
		while (token != "}")
		{
			out << token << std::endl;
			token = tok.nextToken();
		}
		return out.str();
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


	std::string XData::generateXDataDef()
	{
		//ToDo: 1) Howto handle '"' in String?
		//		2) Non-shared_ptr allowed in this case?
		//		3) Possibly check if e.g. the vectorsize of TwoSidedXD->_pageLeftTitle is smaller than _numPages.
		//			So that no exceptions are thrown. (Depends on how XData objects are generated. Basically all
		//			vectors should be of the size _numPages)

		std::stringstream xDataDef;
		xDataDef << _name << "\n" << "{" << "\n" << "\tprecache" << "\n" << "\t\"num_pages\"\t: \"" << _numPages << "\"\n";

		std::stringstream ss;
		std::string TempString;

		xDataDef << getContentDef();

		for (int n=1; n<=_numPages; n++)
		{
			xDataDef << "\t\"gui_page" << n << "\"\t: \"" << _guiPage[n-1] << "\"\n";
		}
		xDataDef << "\t\"snd_page_turn\"\t: \"" << _sndPageTurn << "\"\n}\n\n";//*/
		
		return xDataDef.str();		//Does this support enough characters??
	}


	FileStatus XData::xport(const std::string& FileName, const ExporterCommands& cmd)
	{
		boost::filesystem::path Path(FileName);
		
		if (boost::filesystem::exists(Path))
		{
			//Need to check out DefTokeniser and see if I do these things with help of that class or manually.
			switch (cmd)
			{
			case Merge: 
				//Check if definition already exists. If it does not, append the definition to the file.
				//If it does: return DefinitionExists
				break;
			case MergeAndOverwriteExisting: 
				//Find the old definition in the target file and delete it. Append the new definition.
				break;
			case Overwrite: 
				//Warn if the target file contains multiple definitions: return MultipleDefinitions. 
				//else overwrite existing file.
				break;
			case OverwriteMultDef:
				//Replace the file no matter what.
				break;
			default: return FileExists;
			}
		}

		//Write the definition into the file.
		boost::filesystem::ofstream file(Path, std::ios_base::out);		//check if file open was successful?
		file << generateXDataDef();
		file.close();

		return AllOk;
	}


	
	void XData::resizeVectors(const int& TargetSize)
	{
		_guiPage.resize(TargetSize, "");
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
		std::stringstream ss;
		std::string TempString;

		for (int n = 0; n < _numPages; n++)
		{
			//Page Left Title
			xDataDef << "\t\"page" << n+1 << "_left_title\"\t:\n\t{\n";
			if (_pageLeftTitle[n] != "")
			{
				ss << _pageLeftTitle[n];
				while ( std::getline(ss, TempString) )	//replace "\n"
				{
					xDataDef << "\t\t\"" << TempString << "\"\n";
				}
				xDataDef << "\t}\n";
			}
			else
				xDataDef << "\t\t\"\"\n\t}\n";

			//Page Left Body
			xDataDef << "\t\"page" << n+1 << "_left_body\"\t:\n\t{\n";
			if (_pageLeftBody[n] != "")
			{
				ss.clear();
				ss << _pageLeftBody[n];
				while ( std::getline(ss, TempString) )	//replace "\n"
				{
					xDataDef << "\t\t\"" << TempString << "\"\n";
				}
				xDataDef << "\t}\n";
			}
			else
				xDataDef << "\t\t\"\"\n\t}\n";

			//Page Right Title
			xDataDef << "\t\"page" << n+1 << "_right_title\"\t:\n\t{\n";
			if (_pageRightTitle[n] != "")
			{
				ss.clear();
				ss << _pageRightTitle[n];
				while ( std::getline(ss, TempString) )	//replace "\n"
				{
					xDataDef << "\t\t\"" << TempString << "\"\n";
				}
				xDataDef << "\t}\n";
			}
			else
				xDataDef << "\t\t\"\"\n\t}\n";

			//Page Right Body
			xDataDef << "\t\"page" << n+1 << "_right_body\"\t:\n\t{\n";
			if (_pageRightBody[n] != "")
			{
				ss.clear();
				ss << _pageRightBody[n];
				while ( std::getline(ss, TempString) )	//replace "\n"
				{
					xDataDef << "\t\t\"" << TempString << "\"\n";
				}
				xDataDef << "\t}\n";
			}
			else
				xDataDef << "\t\t\"\"\n\t}\n";

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
		std::stringstream ss;
		std::string TempString;

		for (int n = 0; n < _numPages; n++)
		{
			//Title
			xDataDef << "\t\"page" << n+1 << "_title\"\t:\n\t{\n";
			if (_pageTitle[n] != "")
			{
				ss << _pageTitle[n];
				while ( std::getline(ss, TempString) )	//replace "\n"
				{
					xDataDef << "\t\t\"" << TempString << "\"\n";
				}
				xDataDef << "\t}\n";
			}
			else
				xDataDef << "\t\t\"\"\n\t}\n";

			//Body
			xDataDef << "\t\"page" << n+1 << "_body\"\t:\n\t{\n";
			if (_pageBody[n] != "")
			{
				ss.clear();
				ss << _pageBody[n];
				while ( std::getline(ss, TempString) )	//replace "\n"
				{
					xDataDef << "\t\t\"" << TempString << "\"\n";
				}
				xDataDef << "\t}\n";
			}
			else
				xDataDef << "\t\t\"\"\n\t}\n";
		}

		return xDataDef.str();
	}

} // namespace readable