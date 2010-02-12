#include "XDataManager.h"

namespace readable
{
	inline void XDataManager::trimLeadingSpaces(std::string& String)
	{
		int i;
		for (i = 0; String.c_str()[i] == ' ' || String.c_str()[i] == '\t' || String.c_str()[i] == '\n'; i++) { }	//_lineCount needs to be updated.
		String.erase(0, i-1);
	}

	inline std::string XDataManager::getLineFormatted(boost::filesystem::ifstream* FileStream, char Delimiter)
	{
		std::string ReadString;
		try
		{
			std::getline(*FileStream, ReadString, Delimiter);		//Check if this function can really throw.
		}
		catch (...)
		{
			reportError("[XDataManager::ImportXData] Failed to read line.");
		}
		trimLeadingSpaces(ReadString);
		_lineCount += 1;
		return ReadString;
	}

	inline std::string XDataManager::getLineFormatted(boost::filesystem::ifstream* FileStream)
	{
		return getLineFormatted(FileStream, '\n');
	}

	int XDataManager::_lineCount = 0;

	XDataPtrList XDataManager::importXData(const std::string& FileName)
	{
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

		std::string LineString;
		_lineCount = 0;
		//Start importing file...
		while (!file.eof())
		{
			if ( (LineString = getLineFormatted(&file)) != "")	//Wrong if comments are possible inside xdata...
			{
				XDataPtr NewXData(new XData(LineString));
			//First Bracket pair
				LineString = getLineFormatted(&file, '{');	// '{' is discarded
				if (LineString != "")	//Syntax error
				{
					reportError("[XDataManager::importXData] Syntax-error in " + FileName + " at Line: " + boost::lexical_cast<std::string>(_lineCount) + "\n");
				}
				file.ignore(1000,'\n'); //Goto next line (DIRTY!!!!!)

				// Do some decoding...

				//Replace with defTokeniser

						//std::string GotoNextSymbol(..) function possibly better...
						//std::string GetNextWord(...) . Sollte eventuell auch ":" ignorieren. Multiple delimiters ("\n", " " und "\t")
						//in body: nach abschlieﬂenden quotes ein ignore bis \n


				LineString = getLineFormatted(&file, '{');	// '{' is discarded
				if (LineString != "")	//Syntax error
				{
					reportError("[XDataManager::importXData] Syntax-error in " + FileName + " at Line: " + boost::lexical_cast<std::string>(_lineCount) + "\n");
				}
				file.ignore(1000,'\n');
			//End of first Bracket pair.
				ReturnVector.push_back(NewXData);
			}
		}

		return ReturnVector;
	} // XDataManager::importXData


	inline std::string XDataManager::generateXDataDef(const XData& Data)
	{
		//ToDo: 1) Howto handle '"' in String?
		//		2) Non-shared_ptr allowed in this case?
		//		3) Possibly check if e.g. the vectorsize of TwoSidedXD->_pageLeftTitle is smaller than _numPages.
		//			So that now exceptions are thrown. (Depends on how XData objects are generated. Basically all
		//			vectors should be of the size _numPages)

		std::stringstream xDataDef;
		xDataDef << Data._name << "\n" << "{" << "\n" << "\tprecache" << "\n" << "\t\"num_pages\"\t: \"" << Data._numPages << "\"\n\n";
		if ( const TwoSidedXData* TwoSidedXD = dynamic_cast<const TwoSidedXData*>(&Data) )
		{
			std::stringstream ss;
			std::string TempString;
			for (int n = 1; n <= TwoSidedXD->_numPages; n++)
			{
				//Left Title:
				xDataDef << "\t\"page" << n << "_left_title\" :\n\t{\n";
				if (TwoSidedXD->_pageLeftTitle[n-1] != "")
				{
					ss.clear();
					ss << TwoSidedXD->_pageLeftTitle[n-1];
					while ( std::getline(ss, TempString) )	//replace "\n"
					{
						xDataDef << "\t\t\"" << TempString << "\"\n";
					}
					xDataDef << "\t}\n";
				}
				else
					xDataDef << "\t\t\"\"\n\t}\n";
				//Left Body:
				xDataDef << "\t\"page" << n << "_left_body\" :\n\t{\n";
				if (TwoSidedXD->_pageLeftBody[n-1] != "")
				{
					ss.clear();		// ????
					ss << TwoSidedXD->_pageLeftBody[n-1];
					while ( std::getline(ss, TempString) )
					{
						xDataDef << "\t\t\"" << TempString << "\"\n";
					}
					xDataDef << "\t}\n";
				}
				else
					xDataDef << "\t\t\"\"\n\t}\n";
				//Right Title:
				xDataDef << "\t\"page" << n << "_right_title\" :\n\t{\n";
				if (TwoSidedXD->_pageRightTitle[n-1] != "")
				{
					ss.clear();
					ss << TwoSidedXD->_pageRightTitle[n-1];
					while ( std::getline(ss, TempString) )
					{
						xDataDef << "\t\t\"" << TempString << "\"\n";
					}
					xDataDef << "\t}\n";
				}
				else
					xDataDef << "\t\t\"\"\n\t}\n";
				//Right Body:
				xDataDef << "\t\"page" << n << "_right_body\" :\n\t{\n";
				if (TwoSidedXD->_pageRightBody[n-1] != "")
				{
					ss.clear();
					ss << TwoSidedXD->_pageRightBody[n-1];
					while ( std::getline(ss, TempString) )
					{
						xDataDef << "\t\t\"" << TempString << "\"\n";
					}
					xDataDef << "\t}\n";
				}
				else
					xDataDef << "\t\t\"\"\n\t}\n";
			}

		}
		else	//OneSided
		{
			const OneSidedXData* OneSidedXD(dynamic_cast<const OneSidedXData*>(&Data));
			std::stringstream ss;
			std::string TempString;
			for (int n = 1; n <= OneSidedXD->_numPages; n++)
			{
				//Title
				xDataDef << "\t\"page" << n << "_title\" :\n\t{\n";
				if (OneSidedXD->_pageTitle[n-1] != "")
				{
					ss.clear();
					ss << OneSidedXD->_pageTitle[n-1];
					while ( std::getline(ss, TempString) )	//replace "\n"
					{
						xDataDef << "\t\t\"" << TempString << "\"\n";
					}
					xDataDef << "\t}\n";
				}
				else
					xDataDef << "\t\t\"\"\n\t}\n";				
				//Body:
				xDataDef << "\t\"page" << n << "_body\" :\n\t{\n";
				if (OneSidedXD->_pageBody[n-1] != "")
				{
					ss.clear();
					ss << OneSidedXD->_pageBody[n-1];
					while ( std::getline(ss, TempString) )
					{
						xDataDef << "\t\t\"" << TempString << "\"\n";
					}
					xDataDef << "\t}\n";
				}
				else
					xDataDef << "\t\t\"\"\n\t}\n";
			}
		}

		for (int n=1; n<=Data._numPages; n++)
		{
			xDataDef << "\t\"gui_page" << n << "\"\t: \"" << Data._guiPage[n-1] << "\"\n";
		}
		xDataDef << "\t\"snd_page_turn\"\t: \"" << Data._sndPageTurn << "\"\n}\n\n";//*/
		
		return xDataDef.str();		//Does this support enough characters??
	}


	FileStatus XDataManager::exportXData(const std::string& FileName, const XData& Data, const ExporterCommands& cmd)
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
				boost::filesystem::ofstream file(Path, std::ios_base::out);
				file << generateXDataDef(Data);			//Necessary: Check if writing was successful and throw exception otherwise.
				return AllOk;
				break;
			default: return FileExists;
			}
		}

		//Write the definition into the file.
		boost::filesystem::ofstream file(Path, std::ios_base::out);
		file << generateXDataDef(Data);

		return AllOk;
	}

} // namespace readable