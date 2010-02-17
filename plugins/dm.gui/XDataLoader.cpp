#include "XDataLoader.h"


namespace readable
{
	XDataPtrList XDataLoader::import(const std::string& FileName)
	{
		/* ToDO:
		1) Proper error reporting. A summary should be displayed in the GUI later.
		2) Maybe check FileExtension again.				->done*/

		XDataPtrList ReturnVector;

		//Check fileextension:
		if (FileName.substr( FileName.rfind(".")+1 ) != "xd")
			reportError("[XDataLoader::import] Fileextension is not .xd: " + FileName + "\n");

		// Attempt to open the file in text mode
		ArchiveTextFilePtr file = 
			GlobalFileSystem().openTextFile(XDATA_DIR + FileName);
		if (file == NULL)
			reportError("[XDataLoader::import] Failed to open file: " + FileName + "\n");

		std::istream is(&(file->getInputStream()));
		parser::BasicDefTokeniser<std::istream> tok(is);

		StringList ErrorList;		
		unsigned int ErrorCount = 0;

		while (tok.hasMoreTokens())
		{
			if (parseXDataDef(tok))
				ReturnVector.push_back(_NewXData.xData);
			else
				ErrorCount += 1;
			if (_NewXData.error_msg.size()>0)
				for (unsigned int n=0; n<_NewXData.error_msg.size(); n++)
					ErrorList.push_back(_NewXData.error_msg[n]);
		}

		//temporary
		for (unsigned int n = 0; n<ErrorList.size(); n++)
			std::cerr << ErrorList[n];

		if (ErrorList.size() > 0)
			std::cerr << "[XDataLoader::import] Import finished with " << ErrorList.size() << " error(s)/warning(s). " << ReturnVector.size() << " XData-definition(s) successfully imported, but failed to import at least " << ErrorCount << " definitions." << std::endl;

		return ReturnVector;
	} // XDataLoader::import

	bool XDataLoader::parseXDataDef(parser::DefTokeniser& tok)
	{
		/* TODO:
		1) Support for import-directive								->done
		2) This is pretty ugly code. There's gotta be a better way to do this than with two separate shared pointers. 
		The default resize to MAX_PAGE_COUNT is also uncool.															->done
		3) Add try/catch blocks.																							->done
		4) Possibly throw syntax-exception if _numPages hasn't been defined or reconstruct from the content-vectors or 
		set it automatically depending on the _maxPageCount variable.													->done
		5) Try/catch blocks around direct vector access to prevent vector subscript out of range							->done
		6) Collect general syntax errors in a struct for a return-value... Also in subclass. Add function 
		that scrolls out to the next definition.																		->done
		7) Fix detection of exceeding content-dimensions.																	->done
		8) When guiPages is set, the other vectors should be resized as well.												->done
		9) Pages might be discarded during resize. Maintain a _maxPageCount variable... Page-creation conditions possibly 
		have to be revisited. _maxPageCount should only be raised for pages that have content.							->done
		10)If GuiPageCount is exceeded, the gui page should be stored anyway, because _numPages might be raised later. 
		Resize the vector in the end and store it in the XData object. Errormessage should also be handled differently then.	->done
		11)Have to check the new virtual methods for possible exceptions.				->done
		12)Only resize in the end depending on _numPages and _maxPageCount, to prevent dataloss.								->done
		13)ErrorMessage when _numPages has been raised																		->done
		14)Maybe add a default _guiPage layout for OneSided and TwoSided Objects if no _guiPage layout has been defined.		->done
		15)Check if _guiPage statements are available for every page.														->done
		16)nextToken() can throw if no tokens are available. Think of something to catch that.								->done
		17)Check how the DefTokeniser reacts on escaped quotes.																->done
		18)Warnings for missing statements.																					->done*/

		//Initialization:
		_name = tok.nextToken();		
		_NewXData.error_msg.clear();
		_NewXData.xData.reset();		
		_guiPageError.clear();
		_maxPageCount = 0;
		_maxGuiNumber = 0;
		_guiPageDef = "";
		_numPages = 0;
		_sndPageTurn = "";
		_guiPage.clear();
		_guiPage.resize(MAX_PAGE_COUNT,"");		//see MAX_PAGE_COUNT declaration in header-file for explanation.

		//Start Parsing:
		try	{ tok.assertNextToken("{"); }		//throws when syntax error
		catch (...)
		{
			while (tok.nextToken() != "{") {}
			_NewXData.error_msg.push_back("[XDataLoader::import] Syntaxerror in definition: " + _name + ". '{' expected. Undefined behavior!\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
			jumpOutOfBrackets(tok,1);
			return false;
		}

		//Parse-Loop:
		while (tok.hasMoreTokens())
		{
			std::string token = tok.nextToken();
			if (token == "}")
				break;
			if (!StoreContent(token, tok, _name))
				return false;
		}

		//cleaning up:
		if ( _maxGuiNumber+1 > _numPages)		//Append missing GUI-errormessages... Until now, it wasn't clear how many guipages are actually discarded.
		{
			int diff = _maxGuiNumber + 1 - _maxPageCount;
			for (unsigned int n = _guiPageError.size()-diff; n<_guiPageError.size(); n++)
			{
				_NewXData.error_msg.push_back(_guiPageError[n]);
			}
		}
		// Check if _guiPage-statements for all pages are available.
		if (_guiPageDef == "")
		{
			_NewXData.error_msg.push_back("[XDataLoader::import] Warning for definition: " + _name
				+ ". _guiPage-statement(s) missing. Setting default value...\n");
			if (_NewXData.xData->getPageLayout() == TwoSided)
				_guiPageDef = DEFAULT_TWOSIDED_LAYOUT; 
			else
				_guiPageDef = DEFAULT_ONESIDED_LAYOUT;
		}
		for (int n=0; n<_numPages; n++ )
		{
			if (_guiPage[n] == "")
				_guiPage[n] = _guiPageDef;
		}

		//Store values:
		_NewXData.xData->setGuiPage(_guiPage);
		_NewXData.xData->setNumPages(_numPages);
		_NewXData.xData->resizeVectors(_numPages);

		//Use default sndPageTurn if the statement was missing:
		if (_sndPageTurn == "")
		{
			_NewXData.xData->setSndPageTurn(DEFAULT_SNDPAGETURN);
			_NewXData.error_msg.push_back("[XDataLoader::import] Warning for definition: " + _name
				+ ". _sndPageTurn-statement missing. Setting default value...\n");
		}
		else
			_NewXData.xData->setSndPageTurn(_sndPageTurn);		

		return true;
	}

	bool XDataLoader::StoreContent(const std::string& Where, parser::DefTokeniser& tok, const std::string& DefName)
	{
		//page-statement:
		if (Where.substr(0,4) == "page")
		{
			//Now that it is known, whether we are dealing with a Two- or OneSidedXDataObject, create the XData object.
			if (!_NewXData.xData)
			{
				if (Where.find("left",6) != std::string::npos || Where.find("right",6) != std::string::npos) //TwoSided
				{
					_NewXData.xData = XDataPtr(new TwoSidedXData(_name));
					_NewXData.xData->resizeVectors(MAX_PAGE_COUNT);
				}
				else //OneSided
				{
					_NewXData.xData = XDataPtr(new OneSidedXData(_name));
					_NewXData.xData->resizeVectors(MAX_PAGE_COUNT);
				}
			}

			//Acquire PageIndex
			int PageIndex;
			char number = Where.c_str()[4];
			try { PageIndex = boost::lexical_cast<int>(number)-1; }	//can throw...
			catch (...)
			{
				_NewXData.error_msg.push_back("[XDataLoader::import] Error in definition: " + DefName + ", " + Where + " statement. '" + number + "' is not a number.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");;
				jumpOutOfBrackets(tok,2);
				return false;
			}			

			//Read content
			std::string content;
			if (!ReadLines(tok, content))
			{
				_NewXData.error_msg.push_back("[XDataLoader::import] Error in definition: " + DefName + ". Failed to read content of " + Where + " statement.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
				jumpOutOfBrackets(tok,1);
				return false;
			}

			//Check PageIndex-Range
			if (PageIndex >= _numPages )
			{
				if (content.length() > 1) //_numPages is raised automatically, if a page with content is detected...		//unclean
				{
					_numPages = PageIndex+1;
					_NewXData.error_msg.push_back("[XDataLoader::import] Warning for definition: " + DefName + ", " + Where + " statement.\n\tnumPages not (yet) specified or too low. Raising _numPages to " + number +"...\n");
				}
				else
				{
					_NewXData.error_msg.push_back("[XDataLoader::import] Warning for definition: " + DefName + ", " + Where + " statement.\n\tnumPages not (yet) specified or too low. However, this page does not contain any text and is discarded...\n");
					return true;
				}
			}

			//Refresh the _maxPageCount variable if this page has content.
			if (content.length() > 1)		//unclean
				if (_maxPageCount < PageIndex+1)
					_maxPageCount = PageIndex+1;

			//Write the content into the XData object
			SideChooser side;
			if (Where.find("left",6) != std::string::npos)	//SideChooser is discarded on OneSidedXData...
				side = Left;
			else
				side = Right;
			if (Where.find("body",6) != std::string::npos)
				_NewXData.xData->setPageContent(Body, PageIndex, side, content);			//could throw if PageIndex >= MAX_PAGE_COUNT
			else
				_NewXData.xData->setPageContent(Title, PageIndex, side, content);

		}
		//gui_page statement
		else if (Where.substr(0,8) == "gui_page")
		{
			//Acquire gui_page number:
			std::string number = Where.substr(Where.length()-1,1);
			int guiNumber;
			try	{ guiNumber = boost::lexical_cast<int>(number)-1; }
			catch (...)
			{
				_NewXData.error_msg.push_back("[XDataLoader::import] Error in definition: " + DefName + ", gui_page statement. '" + number + "' is not a number.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
				jumpOutOfBrackets(tok,1);
				return false;	
			}
			if (_maxGuiNumber < guiNumber)
				_maxGuiNumber = guiNumber;

			//Get the GuiPageDef:
			if (!ReadLines(tok, _guiPageDef))
			{
				_NewXData.error_msg.push_back("[XDataLoader::import] Error in definition: " + DefName + ". Failed to read content of " + Where + " statement.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
				jumpOutOfBrackets(tok,1);
				return false;	
			}
			_guiPage[ guiNumber ] = _guiPageDef;		//could throw if guiNumber >= MAX_PAGE_COUNT

			//Append error-message if _numPages is exceeded.
			if (guiNumber >= _numPages)
				_guiPageError.push_back("[XDataLoader::import] Warning for definition: " + DefName + ". More _guiPage statements, than pages. Discarding statement for Page " + number + ".\n");
		}
		//num_pages statement
		else if (Where == "num_pages")
		{
			//Get num_pages:
			std::string number;
			if (!ReadLines(tok, number))
			{
				_NewXData.error_msg.push_back("[XDataLoader::import] Error in definition: " + DefName + ". Failed to read content of " + Where + " statement.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
				jumpOutOfBrackets(tok,1);
				return false;
			}
			try { _numPages = boost::lexical_cast<int>(number); }	//throws
			catch(...)
			{
				_NewXData.error_msg.push_back("[XDataLoader::import] Error in definition: " + DefName + ", num_pages statement. '" + number + "' is not a number.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
				jumpOutOfBrackets(tok,1);
				return false;
			}

			//Correct a faulty _numPages value
			if (_maxPageCount > _numPages)
			{
				_numPages = _maxPageCount;
				_NewXData.error_msg.push_back("[XDataLoader::import] Warning for definition: " + DefName
					+ ". The specified _numPages statement did not match the amount of pages with content.\n\tnumPages is set to " 
					+ boost::lexical_cast<std::string>(_numPages) + ".\n");
			}
		}
		//snd_page_turn statement
		else if (Where == "snd_page_turn")
		{
			if (!ReadLines(tok, _sndPageTurn))
			{
				_NewXData.error_msg.push_back("[XDataLoader::import] Error in definition: " + DefName + ". Failed to read content of " + Where + " statement.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
				jumpOutOfBrackets(tok,1);
				return false;
			}
		}
		//import statement
		else if (Where == "import")
		{
			//Enter content brackets
			std::string token;
			try { tok.assertNextToken("{");	}	//can throw
			catch(...)
			{
				_NewXData.error_msg.push_back("[XDataLoader::import] Syntax error in definition: " + DefName + ", import-statement. '{' expected. Undefined behavior!\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
				jumpOutOfBrackets(tok,1);
				return false;
			}

			//Get Source- and DestStatements
			StringList SourceStatement;
			StringList DestStatement;
			try
			{
				token = tok.nextToken();
				while (token != "}")
				{
					SourceStatement.push_back(token);
					tok.skipTokens(1);	//Skip "->"
					DestStatement.push_back(tok.nextToken());
					token = tok.nextToken();
				}
			}
			catch (...)
			{
				_NewXData.error_msg.push_back("[XDataLoader::import] Error in definition: " + DefName + ". Failed to read content of " + Where + " statement. Probably Syntax-error.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
				jumpOutOfBrackets(tok,1);
				return false;
			}

			//Get Name of Source-Definition
			try { tok.assertNextToken("from"); }
			catch (...)
			{
				_NewXData.error_msg.push_back("[XDataLoader::import] Syntax error in definition: " + DefName + ", import-statement. 'from' expected. Undefined behavior!\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
				jumpOutOfBrackets(tok,1);
				return false;
			}
			std::string SourceDef;
			try	{ SourceDef = tok.nextToken(); }
			catch (...)
			{
				_NewXData.error_msg.push_back("[XDataLoader::import] Error in definition: " + DefName + ". Failed to read content of " + Where + " statement.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
				jumpOutOfBrackets(tok,1);
				return false;
			}

			//Check where the Definition is stored in the DefMap
			StringMap::iterator it = DefMap.find(SourceDef);			//gotta test if this works when DefMap is empty.
			if (it == DefMap.end())		//If the definition couldn't be found, refresh the DefMap and try again.
			{
				grabAllDefinitions();
				it = DefMap.find(SourceDef);
			}
			if (it == DefMap.end())
			{
				_NewXData.error_msg.push_back("[XData::import] Error in definition: " + DefName + ". Found an import-statement, but not the corresponding definition.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
				jumpOutOfBrackets(tok,1);
				return false;
			}

			//Open the file
			ArchiveTextFilePtr file = 
				GlobalFileSystem().openTextFile(XDATA_DIR + it->second);
			if (file == NULL)
			{
				_NewXData.error_msg.push_back("[XData::import] Error in definition: " + DefName + ". Found an import-statement, but failed to open the corresponding file: " + it->second + ".\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
				jumpOutOfBrackets(tok,1);
				return false;
			}
			//Find the Definition in the File:
			std::stringstream ss;
			ss << &(file->getInputStream());
			ss << ss.str().substr( ss.str().find(SourceDef) );
			std::istream is(ss.rdbuf());
			//Import-Loop: Find the SourceStatement in the file and pass the DefTokeniser to StoreContent
			for (int n = 0; n < SourceStatement.size(); n++)
			{
				is.seekg(0);
				parser::BasicDefTokeniser<std::istream> ImpTok(is);
				//Move into brackets of the definition and count entering and leaving brackets afterwards, so that the end of the definition can be identified.
				try
				{	
					while (ImpTok.nextToken() != "{") {}
					int BracketDepth = 1;
					std::string BracketToken = ImpTok.nextToken(); //can throw
					while ( BracketToken != SourceStatement[n])
					{
						if (BracketToken == "{")
							BracketDepth += 1;
						else if (BracketToken == "}")
							BracketDepth -= 1;
						if (BracketDepth == 0)	 //Sourcestatement is not in the current definition
						{
							_NewXData.error_msg.push_back("[XData::import] Error in definition: " + DefName + ". Found an import-statement, but couldn't find the desired statement: " + SourceStatement[n] +", in definition " + SourceDef + ", in file " + it->second + ".\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
							jumpOutOfBrackets(tok,1);
							return false;
						}
						BracketToken = ImpTok.nextToken();
					}
				}
				catch (...)
				{
					_NewXData.error_msg.push_back("[XDataLoader::import] Error in definition: " + DefName + ". Found an import-statement, but couldn't read the desired statement: " + SourceStatement[n] +", in definition " + SourceDef + ", in file " + it->second + ".\n");
					jumpOutOfBrackets(tok,1);
					return false;
				}
				if (!StoreContent(DestStatement[n],ImpTok,SourceDef))
				{
					_NewXData.error_msg.push_back("[XData::import] Error in definition: " + DefName + ". Found an import-statement, but failed to parse the corresponding statement: " + SourceStatement[n] +", in definition " + SourceDef + ", in file " + it->second + ".\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
					jumpOutOfBrackets(tok,1);
					return false;
				}
			}
		}

		return true;
	}

	inline bool XDataLoader::ReadLines(parser::DefTokeniser& tok, std::string& What)
	{
		/* ToDo:
			1) Check how parsing slash-separated linebreaks work.			->done*/

		std::stringstream out;
		std::string token;

		//Check Syntax
		try
		{
			tok.skipTokens(1);			//Skip ":"
			What = tok.nextToken();			//can throw
			if (What != "{" )	//Parsing SingleLine Content
				return true;

			//Parse MultiLine Content.
			token = tok.nextToken();		//can throw
			while (token != "}")
			{
				if (token == "/")		//newlines can also be signalized by slashed. Ignore them
				{
					token = tok.nextToken();
					continue;
				}
				else if (token.c_str()[token.length()-1] == '\\')	//support for escaped quotes in texts...
					token = token.substr(0,token.length()-1) + "\"";
				else
					token = token + "\n";
				out << token;	
				token = tok.nextToken();		//can throw
			}
		}
		catch (...) { return false; }
		What = out.str();
		return true;
	}


	void XDataLoader::jumpOutOfBrackets(parser::DefTokeniser& tok, int CurrentDepth)	//not tested.
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


		
	void XDataLoader::grabAllDefinitions()
	{
		ScopedDebugTimer timer("XData definitions parsed: ");
		GlobalFileSystem().forEachFile(
			XDATA_DIR, 
			XDATA_EXT,
			makeCallback1(*this),
			99);
	}

	void XDataLoader::operator() (const std::string& filename)
	{
		// Attempt to open the file in text mode
		ArchiveTextFilePtr file = 
			GlobalFileSystem().openTextFile(XDATA_DIR + filename);

		if (file != NULL) {		
			// File is open, so parse the tokens
			try 
			{
				std::istream is(&(file->getInputStream()));
				parser::BasicDefTokeniser<std::istream> tok(is);
				//grab all names from stream:
				while (tok.hasMoreTokens())
				{
					std::string tempstring = tok.nextToken();
					tok.assertNextToken("{");
					std::pair<StringMap::iterator,bool> ret = DefMap.insert(StringMap::value_type(tempstring,filename));		//What happens if the same definition exists multiple times?
					if (!ret.second)
						std::cerr << "[XDataLoader] The definition " << tempstring << " of the file " << filename << " already exists. It was defined in " << ret.first->second << ".\n";
					jumpOutOfBrackets(tok, 1);
				}
			}
			catch (parser::ParseException e) 
			{
				std::cerr << "[XDataLoader] Failed to parse " << filename
					<< ": " << e.what() << std::endl;
			}
		}
		else {
			std::cerr << "[XDataLoader] Unable to open " << filename << std::endl;
		}
	}
}