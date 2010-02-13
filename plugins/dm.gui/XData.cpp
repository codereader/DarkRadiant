#include "XData.h"

namespace readable
{
	namespace
	{
		const int MAX_PAGE_COUNT = 20;
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
				for (int n=0; n<parsed.error_msg.size(); n++)
					ErrorList.push_back(parsed.error_msg[n]);
		}

		//temporary
		for (int n = 0; n<ErrorList.size(); n++)
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
				The default resize to MAX_PAGE_COUNT is also uncool.
			3) Add try/catch blocks.	done
			4) Possibly throw syntax-exception if numPages hasn't been defined or reconstruct from the content-vectors.
			5) Try/catch blocks around direct vector access to prevent vector subscript out of range	done
			6) Collect general syntax errors in a struct for a return-value... Also in subclass. Add function that scrolls out to the next definition.	done
			7) Fix detection of exceeding content-dimensions.	done
			8) When guiPages is set, the other vectors should be resized as well.
			9) Pages might be discarded during resize.*/

		std::string name = tok.nextToken();
		
		XDataParse NewXData;

		std::string ErrorString;

		int numPages = MAX_PAGE_COUNT;
		std::string sndPageTurn = "";
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
					if (content.length() > 2) //numPages is raised automatically, if a page with content is detected...
					{
						numPages = PageIndex+1;
						NewXData.error_msg.push_back("[XDataManager::importXData] Error in definition: " + name + ", " + token + " statement.\n\tnumPages does not match the actual amount of pages defined in this XData-Def. Raising numPages to " + number +"...\n");
						//resize vectors:
						guiPage.resize(numPages,"");
						if (NewXData.xData)
							NewXData.xData->resizeVectors(numPages);
					}
					else
						NewXData.error_msg.push_back("[XDataManager::importXData] Error in definition: " + name + ", " + token + " statement.\n\tnumPages does not match the actual amount of pages defined in this XData-Def. However, this page does not contain any text and is discarded...\n");
				}

				//Create the XData object
				if (!NewXData.xData)
				{
					if (token.substr(6,4) == "left" || token.substr(6,5) == "right") //TwoSided
					{
						NewXData.xData = XDataPtr(new TwoSidedXData(name));
						NewXData.xData->resizeVectors(numPages);
					}
					else //OneSided
					{
						NewXData.xData = XDataPtr(new OneSidedXData(name));
						NewXData.xData->resizeVectors(numPages);
					}
				}

				//Write the content into the XData object
				if (PageIndex < numPages)
				{	
					SideChooser side;
					if (token.find("left",6) != std::string::npos)	//SideChooser is discarded on OneSidedXData...
						side = Left;
					else
						side = Right;
					if (token.find("body",6) != std::string::npos)
						NewXData.xData->setContent(Body, PageIndex, side, content);
					else
						NewXData.xData->setContent(Title, PageIndex, side, content);

					/*
					if (twosided)
					{
					if (token.substr(6,4) == "left")
					{
					if(token.find("body",11) != std::string::npos)
					{
					NewXData_two->_pageLeftBody[PageIndex]= content;
					}
					else	//title
					{
					NewXData_two->_pageLeftTitle[PageIndex]= content;
					}
					}
					else	//right side
					{
					if (token.find("body",11) != std::string::npos)
					{
					NewXData_two->_pageRightBody[PageIndex]= content;
					}
					else	//title
					{
					NewXData_two->_pageRightTitle[PageIndex]= content;
					}	
					}
					}
					else	//onesided
					{
					if (token.substr(6,4) == "body")
					{
					NewXData_one->_pageBody[PageIndex]= content;
					}
					else	//title
					{
					NewXData_one->_pageTitle[PageIndex]= content;
					}
					}//*/
				}
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
				if (guiNumber < guiPage.size())			//Maybe the guiPage should be stored anyway, because we don't know yet, whether numPages will be raised by pagedefinitions...
					guiPage[ guiNumber ] = tok.nextToken();
				else
					NewXData.error_msg.push_back("[XDataManager::importXData] Error in definition: " + name + ". More guiPage statements, than pages. Discarding statement for Page " + number + ".\n");
			}
			else if (token == "num_pages")	//Also resize the other vectors here... Maybe check if a page with a higher number has already been added and warn accordingly.
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
				//resize vectors:
				guiPage.resize(numPages, "");
				if (NewXData.xData)
					NewXData.xData->resizeVectors(numPages);
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
		NewXData.xData->_guiPage = guiPage;
		NewXData.xData->_numPages = numPages;
		NewXData.xData->_sndPageTurn = sndPageTurn;

		//Throw syntax exception if numPages wasn't definined?
		
		/*
		if (twosided)	//resize, because numPages is not necessarily defined in the beginning.
		{
			if (NewXData_two->_pageLeftBody.size() != numPages)		//check if this really works.
			{
				NewXData_two->_pageLeftBody.resize(numPages,"");
				NewXData_two->_pageRightBody.resize(numPages,"");
				NewXData_two->_pageLeftTitle.resize(numPages,"");
				NewXData_two->_pageRightTitle.resize(numPages,"");
			}			
		}
		else
		{
			if (NewXData_one->_pageBody.size() != numPages)
			{
				NewXData_one->_pageBody.resize(numPages,"");
				NewXData_one->_pageTitle.resize(numPages,"");
			}
		}//*/

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
				//boost::filesystem::ofstream file(Path, std::ios_base::out);
				//file << generateXDataDef(Data);			//Necessary: Check if writing was successful and throw exception otherwise.
				return AllOk;
				break;
			default: break; //return FileExists;
			}
		}

		//Write the definition into the file.
		boost::filesystem::ofstream file(Path, std::ios_base::out);
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