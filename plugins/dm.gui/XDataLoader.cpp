#include "XDataLoader.h"


namespace readable
{
	const bool XDataLoader::importSingleDef( const std::string& filename, const std::string& definitionName, XDataPtr& target )
	{
		//Initialization:
		_errorList.clear();
		_newXData.reset();

		//Check fileextension:
		if (filename.substr( filename.rfind(".")+1 ) != "xd")
			return reportError("[XDataLoader::import] Fileextension is not .xd: " + filename + "\n");

		// Attempt to open the file in text mode and retrieve DefTokeniser
		ArchiveTextFilePtr file = 
			GlobalFileSystem().openTextFile(XDATA_DIR + filename);
		if (file == NULL)
			return reportError("[XDataLoader::import] Failed to open file: " + filename + "\n");
		std::istream is(&(file->getInputStream()));
		parser::BasicDefTokeniser<std::istream> tok(is);

		//Parse the desired definition:
		while (tok.hasMoreTokens() && !parseXDataDef(tok,definitionName)) {}

		//Summarizing report:
		if (!_newXData)
			return reportError("[XDataLoader::importSingleDef] Failed to load " + definitionName + ".\n");
		else
		{
			_errorList.push_back("[XDataLoader::importSingleDef] " + definitionName + " loaded successfully with " 
				+ boost::lexical_cast<std::string>(_errorList.size()) + " error(s)/warning(s).\n");
			//for (unsigned int n=0; n<_errorList.size()-1; n++)
			//	std::cerr << _errorList[n];
			//Summary output:
			if (_errorList.size() > 1)
				std::cerr << _errorList[_errorList.size()-1];
			else
				globalOutputStream() << _errorList[0];
		}

		target = _newXData;
		return true;
	}

	const bool XDataLoader::import( const std::string& filename, XDataPtrList& target )
	{
		//initialization:
		_errorList.clear();
		target.clear();
		unsigned int ErrorCount = 0;

		//Check fileextension:
		if (filename.substr( filename.rfind(".")+1 ) != "xd")
			return reportError("[XDataLoader::import] Fileextension is not .xd: " + filename + "\n");

		// Attempt to open the file in text mode and retrieve DefTokeniser
		ArchiveTextFilePtr file = 
			GlobalFileSystem().openTextFile(XDATA_DIR + filename);
		if (file == NULL)
			return reportError("[XDataLoader::import] Failed to open file: " + filename + "\n");

		std::istream is(&(file->getInputStream()));
		parser::BasicDefTokeniser<std::istream> tok(is);

		//Parse Loop:
		while (tok.hasMoreTokens())
		{
			if (parseXDataDef(tok))
				target.push_back(_newXData);
			else
				ErrorCount += 1;
		}
		
		//Write summary
		_errorList.push_back(
			"[XDataLoader::import] Import finished with " + boost::lexical_cast<std::string>(_errorList.size()) 
			+ " error(s)/warning(s). " + boost::lexical_cast<std::string>(target.size()) 
			+ " XData-definition(s) successfully imported, but failed to import at least " 
			+ boost::lexical_cast<std::string>(ErrorCount) + " definitions.\n"
			);

		//Summary output:
		//for (unsigned int n = 0; n < _errorList.size()-1; n++)
		//	std::cerr << _errorList[n];
		if (_errorList.size() == 1)	//No errors.
			globalOutputStream() << _errorList[0];
		else
			std::cerr << _errorList[_errorList.size()-1];

		if (target.size() == 0)
			return false;

		return true;
	} // XDataLoader::import

	const bool XDataLoader::parseXDataDef(parser::DefTokeniser& tok, const std::string& definitionName)
	{
		_name = tok.nextToken();

		//Check Syntax:
		try	{ tok.assertNextToken("{"); }
		catch (...)
		{
			while (tok.nextToken() != "{") {}
			return reportError(tok, "[XDataLoader::import] Syntaxerror in definition: " + _name + ". '{' expected. Undefined behavior!\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
		}

		//Check if every definition shall be parsed or only a specific one:
		if (!definitionName.empty() && _name != definitionName)
		{
			jumpOutOfBrackets(tok);
			return false;
		}

		//Initialization:
		_newXData.reset();		
		_guiPageError.clear();
		_maxPageCount = 0;
		_maxGuiNumber = 0;
		_guiPageDef = "";
		_numPages = 0;
		_sndPageTurn = "";
		_guiPage.clear();
		_guiPage.resize(MAX_PAGE_COUNT,"");		//see MAX_PAGE_COUNT declaration in XData.h for explanation.

		//Parse-Loop:
		while (tok.hasMoreTokens())
		{
			std::string token = tok.nextToken();
			if (token == "}")
				break;
			if (!storeContent(token, tok, _name))
				return false;
		}

		//cleaning up:
		if ( _maxGuiNumber+1 > _numPages)		//Append missing GUI-errormessages... Until now, it wasn't clear how many guipages are actually discarded.
		{
			int diff = _maxGuiNumber + 1 - _maxPageCount;
			for (unsigned int n = _guiPageError.size()-diff; n<_guiPageError.size(); n++)
			{
				reportError(_guiPageError[n]);
			}
		}

		// Check if _guiPage-statements for all pages are available.
		if (_guiPageDef == "")
		{
			reportError("[XDataLoader::import] Warning for definition: " + _name
				+ ". _guiPage-statement(s) missing. Setting default value...\n");
			if (_newXData->getPageLayout() == TwoSided)
				_guiPageDef = DEFAULT_TWOSIDED_LAYOUT; 
			else
				_guiPageDef = DEFAULT_ONESIDED_LAYOUT;
		}
		for (std::size_t n=0; n<_numPages; n++ )
		{
			if (_guiPage[n] == "")
				_guiPage[n] = _guiPageDef;
		}

		//Store values:
		_newXData->setGuiPage(_guiPage);
		_newXData->setNumPages(_numPages);

		//Use default sndPageTurn if the statement was missing:
		if (_sndPageTurn == "")
		{
			_newXData->setSndPageTurn(DEFAULT_SNDPAGETURN);
			reportError("[XDataLoader::import] Warning for definition: " + _name
				+ ". _sndPageTurn-statement missing. Setting default value...\n");
		}
		else
			_newXData->setSndPageTurn(_sndPageTurn);		

		return true;
	}

	const bool XDataLoader::storeContent(const std::string& statement, parser::DefTokeniser& tok, const std::string& defName)
	{
		//page-statement:
		if (statement.substr(0,4) == "page")
		{
			//Now that it is known, whether we are dealing with a Two- or OneSidedXDataObject, create the XData object.
			if (!_newXData)
			{
				if (statement.find("left",6) != std::string::npos || statement.find("right",6) != std::string::npos) //TwoSided
					_newXData = XDataPtr(new TwoSidedXData(_name));
				else
					_newXData = XDataPtr(new OneSidedXData(_name));
			}

			//Acquire PageIndex
			std::size_t PageIndex;
			char number = statement.c_str()[4];
			try { PageIndex = boost::lexical_cast<int>(number)-1; }
			catch (...)
			{
				return reportError(tok, "[XDataLoader::import] Error in definition: " + defName + ", " + statement + " statement. '" + number + "' is not a number.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
			}			

			//Read content
			std::string content;
			if (!readLines(tok, content))
				return reportError(tok, "[XDataLoader::import] Error in definition: " + defName + ". Failed to read content of " + statement + " statement.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");

			//Check PageIndex-Range
			if (PageIndex >= _numPages )
			{
				if (content.length() > 1) //_numPages is raised automatically, if a page with content is detected...		//unclean
				{
					_numPages = PageIndex+1;
					reportError("[XDataLoader::import] Warning for definition: " + defName + ", " + statement + " statement.\n\tnumPages not (yet) specified or too low. Raising _numPages to " + number +"...\n");
				}
				else
				{
					reportError("[XDataLoader::import] Warning for definition: " + defName + ", " + statement + " statement.\n\tnumPages not (yet) specified or too low. However, this page does not contain any text and is discarded...\n");
					return true;
				}
			}

			//Refresh the _maxPageCount variable if this page has content.
			if (content.length() > 1)		//unclean
				if (_maxPageCount < PageIndex+1)
					_maxPageCount = PageIndex+1;

			//Write the content into the XData object
			Side side;
			if (statement.find("left",6) != std::string::npos)	//Side is discarded on OneSidedXData...
				side = Left;
			else
				side = Right;
			if (statement.find("body",6) != std::string::npos)
				_newXData->setPageContent(Body, PageIndex, side, content);			//could throw if PageIndex >= MAX_PAGE_COUNT
			else
				_newXData->setPageContent(Title, PageIndex, side, content);

		}
		//gui_page statement
		else if (statement.substr(0,8) == "gui_page")
		{
			//Acquire gui_page number:
			std::string number = statement.substr(statement.length()-1,1);
			std::size_t guiNumber;
			try	{ guiNumber = boost::lexical_cast<int>(number)-1; }
			catch (...)
			{
				return reportError(tok, "[XDataLoader::import] Error in definition: " + defName + ", gui_page statement. '" + number + "' is not a number.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
			}
			if (_maxGuiNumber < guiNumber)
				_maxGuiNumber = guiNumber;

			//Get the GuiPageDef:
			if (!readLines(tok, _guiPageDef))
				return reportError(tok, "[XDataLoader::import] Error in definition: " + defName + ". Failed to read content of " + statement + " statement.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");	
			_guiPage[ guiNumber ] = _guiPageDef;		//could throw if guiNumber >= MAX_PAGE_COUNT

			//Append error-message if _numPages is exceeded.
			if (guiNumber >= _numPages)
				_guiPageError.push_back("[XDataLoader::import] Warning for definition: " + defName + ". More _guiPage statements, than pages. Discarding statement for Page " + number + ".\n");
		}
		//num_pages statement
		else if (statement == "num_pages")
		{
			//Get num_pages:
			std::string number;
			if (!readLines(tok, number))
				return reportError(tok, "[XDataLoader::import] Error in definition: " + defName + ". Failed to read content of " + statement + " statement.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
			try { _numPages = boost::lexical_cast<int>(number); }
			catch(...)
			{
				return reportError(tok, "[XDataLoader::import] Error in definition: " + defName + ", num_pages statement. '" + number + "' is not a number.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
			}

			//Correct a faulty _numPages value
			if (_maxPageCount > _numPages)
			{
				_numPages = _maxPageCount;
				reportError("[XDataLoader::import] Warning for definition: " + defName
					+ ". The specified _numPages statement did not match the amount of pages with content.\n\tnumPages is set to " 
					+ boost::lexical_cast<std::string>(_numPages) + ".\n");
			}
		}
		//snd_page_turn statement
		else if (statement == "snd_page_turn")
		{
			if (!readLines(tok, _sndPageTurn))
				return reportError(tok, "[XDataLoader::import] Error in definition: " + defName + ". Failed to read content of " + statement + " statement.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
		}
		//import statement
		else if (statement == "import")
		{
			//Enter content brackets
			std::string token;
			try { tok.assertNextToken("{");	}
			catch(...)
			{
				return reportError(tok, "[XDataLoader::import] Syntax error in definition: " + defName + ", import-statement. '{' expected. Undefined behavior!\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
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
				return reportError(tok, "[XDataLoader::import] Error in definition: " + defName + ". Failed to read content of " + statement + " statement. Probably Syntax-error.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
			}

			//Get Name of Source-Definition
			try { tok.assertNextToken("from"); }
			catch (...)
			{
				return reportError(tok, "[XDataLoader::import] Syntax error in definition: " + defName + ", import-statement. 'from' expected. Undefined behavior!\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
			}
			std::string SourceDef;
			try	{ SourceDef = tok.nextToken(); }
			catch (...)
			{
				return reportError(tok, "[XDataLoader::import] Error in definition: " + defName + ". Failed to read content of " + statement + " statement.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
			}

			//Check where the Definition is stored in the _defMap
			StringMap::iterator it = _defMap.find(SourceDef);
			if (it == _defMap.end())		//If the definition couldn't be found, refresh the _defMap and try again.
			{
				retrieveXdInfo();
				it = _defMap.find(SourceDef);
			}
			if (it == _defMap.end())
				return reportError(tok, "[XData::import] Error in definition: " + defName + ". Found an import-statement, but not the corresponding definition.\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");

			//Open the file
			ArchiveTextFilePtr file = 
				GlobalFileSystem().openTextFile(XDATA_DIR + it->second);
			if (file == NULL)
				return reportError(tok, "[XData::import] Error in definition: " + defName + ". Found an import-statement, but failed to open the corresponding file: " + it->second + ".\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");

			//Find the Definition in the File:
			std::stringstream ss;
			ss << &(file->getInputStream());
			ss << ss.str().substr( ss.str().find(SourceDef) );
			std::istream is(ss.rdbuf());
			//Import-Loop: Find the SourceStatement in the file and pass the DefTokeniser to storeContent
			for (std::size_t n = 0; n < SourceStatement.size(); n++)
			{
				is.seekg(0);
				parser::BasicDefTokeniser<std::istream> ImpTok(is);
				//Move into brackets of the definition and count entering and leaving brackets afterwards, so that the end of the definition can be identified.
				try
				{	
					while (ImpTok.nextToken() != "{") {}
					int BracketDepth = 1;
					std::string BracketToken = ImpTok.nextToken();
					while ( BracketToken != SourceStatement[n])
					{
						if (BracketToken == "{")
							BracketDepth += 1;
						else if (BracketToken == "}")
							BracketDepth -= 1;
						if (BracketDepth == 0)	 //Sourcestatement is not in the current definition
							return reportError(tok, "[XData::import] Error in definition: " + defName + ". Found an import-statement, but couldn't find the desired statement: " + SourceStatement[n] +", in definition " + SourceDef + ", in file " + it->second + ".\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
						BracketToken = ImpTok.nextToken();
					}
				}
				catch (...)
				{
					return reportError(tok, "[XDataLoader::import] Error in definition: " + defName + ". Found an import-statement, but couldn't read the desired statement: " + SourceStatement[n] +", in definition " + SourceDef + ", in file " + it->second + ".\n");
				}
				if (!storeContent(DestStatement[n],ImpTok,SourceDef))
					return reportError(tok, "[XData::import] Error in definition: " + defName + ". Found an import-statement, but failed to parse the corresponding statement: " + SourceStatement[n] +", in definition " + SourceDef + ", in file " + it->second + ".\n\tTrying to Jump to next XData definition. Might lead to furthers errors.\n");
			}
		}

		return true;
	}

	inline const bool XDataLoader::readLines(parser::DefTokeniser& tok, std::string& what) const
	{
		std::stringstream out;
		std::string token;

		//Check Syntax
		try
		{
			tok.skipTokens(1);			//Skip ":"
			what = tok.nextToken();
			if (what != "{" )	//Parsing SingleLine Content
				return true;

			//Parse MultiLine Content.
			token = tok.nextToken();
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
				token = tok.nextToken();
			}
		}
		catch (...) { return false; }
		what = out.str();
		return true;
	}


	void XDataLoader::jumpOutOfBrackets(parser::DefTokeniser& tok, std::size_t currentDepth) const
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


		
	void XDataLoader::retrieveXdInfo()
	{
		_defMap.clear();
		_definitionList.clear();
		_fileSet.clear();
		_duplicatedDefs.clear();
		ScopedDebugTimer timer("XData definitions parsed: ");
		GlobalFileSystem().forEachFile(
			XDATA_DIR, 
			XDATA_EXT,
			makeCallback1(*this),
			99);

		//Generate the sorted vector-list of definitions and files:		
		for (StringMap::iterator it = _defMap.begin(); it != _defMap.end(); it++)
		{
			_definitionList.push_back(it->first);
		}
		return;
	}

	void XDataLoader::operator() (const std::string& filename)
	{
		// Attempt to open the file in text mode
		ArchiveTextFilePtr file = 
			GlobalFileSystem().openTextFile(XDATA_DIR + filename);

		if (file != NULL) {		
			// File is open, so add the file to the _fileSet-set and parse the tokens
			_fileSet.insert(filename);
			try 
			{
				std::istream is(&(file->getInputStream()));
				parser::BasicDefTokeniser<std::istream> tok(is);
				//grab all names from stream:
				while (tok.hasMoreTokens())
				{
					std::string tempstring = tok.nextToken();
					tok.assertNextToken("{");
					std::pair<StringMap::iterator,bool> ret = _defMap.insert(StringMap::value_type(tempstring,filename));
					if (!ret.second)	//Definition already exists.
					{
						std::cerr << "[XDataLoader] The definition " << tempstring << " of the file " << filename << " already exists. It was defined in " << ret.first->second << ".\n";
						std::pair<DuplicatedDefsMap::iterator,bool> duplRet = _duplicatedDefs.insert(DuplicatedDefsMap::value_type(tempstring,std::vector<std::string>(1,ret.first->second)));
						duplRet.first->second.push_back(filename);
					}
					jumpOutOfBrackets(tok);
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
