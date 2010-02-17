#include "XDataLoader.h"


namespace readable
{
	void XDataLoader::StoreContent(const std::string& Where, parser::DefTokeniser& tok)
	{

	}

	XDataPtrList XDataLoader::import(const std::string& FileName)
	{
		/* ToDO:
		1) Proper error reporting. A summary should be displayed in the GUI later.
		2) Maybe check FileExtension again.				->done*/

		XDataPtrList ReturnVector;

		// Attempt to open the file in text mode
		ArchiveTextFilePtr file = 
			GlobalFileSystem().openTextFile(XDATA_DIR + FileName);
		if (file == NULL)
			reportError("[XData::importXDataFromFile] Failed to open file: " + FileName + "\n");

		std::istream is(&(file->getInputStream()));
		parser::BasicDefTokeniser<std::istream> tok(is);

		StringList ErrorList;		
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

		//file.close();
		return ReturnVector;
	} // XData::importXDataFromFile

	XDataParse XDataLoader::parseXDataDef(parser::DefTokeniser& tok)
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
		guiPage.resize(MAX_PAGE_COUNT,"");		//see MAX_PAGE_COUNT declaration in header-file for explanation.
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
					NewXData.xData->setPageContent(Body, PageIndex, side, content);
				else
					NewXData.xData->setPageContent(Title, PageIndex, side, content);
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
		NewXData.xData->setGuiPage(guiPage);
		NewXData.xData->setNumPages(numPages);
		if (sndPageTurn == "")
		{
			NewXData.xData->setSndPageTurn(DEFAULT_SNDPAGETURN);
			NewXData.error_msg.push_back("[XData::importXDataFromFile] Warning for definition: " + name
				+ ". sndPageTurn-statement missing. Setting default value...\n");
		}
		else
			NewXData.xData->setSndPageTurn(sndPageTurn);

		NewXData.xData->resizeVectors(numPages);

		return NewXData;
	}

	std::string XDataLoader::parseText(parser::DefTokeniser& tok)
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


	void XDataLoader::importDirective(parser::DefTokeniser& tok, XDataParse& NewXData, const std::string name)
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

		if (DefMap.empty())
			grabAllDefinitions();

		StringMap::iterator it = DefMap.find(SourceDef);
		if (it != DefMap.end())
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
	
	void XDataLoader::grabAllDefinitions()
	{
		ScopedDebugTimer timer("XData definitions parsed: ");
		GlobalFileSystem().forEachFile(
			XDATA_DIR, 
			XDATA_EXT,
			makeCallback1(*this),
			99);
	}

	void XDataLoader::refreshDefMap()
	{
		grabAllDefinitions();
		std::string tempstring = "Number of Definitions imported: " + boost::lexical_cast<std::string>(DefMap.size()) + ".\n";
		printf(tempstring.c_str());		
	}
	
	// Functor operator: For reading all files.
	void XDataLoader::operator() (const std::string& filename)
	{
		// Attempt to open the file in text mode
		ArchiveTextFilePtr file = 
			GlobalFileSystem().openTextFile(XDATA_DIR + filename);

		if (file != NULL) {		
			// File is open, so parse the tokens
			try {
				std::istream is(&(file->getInputStream()));
				parser::BasicDefTokeniser<std::istream> tok(is);
				//grab all names from stream:
				while (tok.hasMoreTokens())
				{
					std::string tempstring = tok.nextToken();
					tok.assertNextToken("{");
					DefMap.insert(StringMap::value_type(tempstring,filename));		//What happens if the same definition exists multiple times?
					jumpOutOfBrackets(tok, 1);
				}
			}
			catch (parser::ParseException e) {
				std::cerr << "[XDataLoader] Failed to parse " << filename
					<< ": " << e.what() << std::endl;
			}
		}
		else {
			std::cerr << "[XDataLoader] Unable to open " << filename << std::endl;
		}
	}
}