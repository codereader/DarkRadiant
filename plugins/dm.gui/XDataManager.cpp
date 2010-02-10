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

	XDataPtrList XDataManager::importXData(std::string FileName)
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

				//std::string GotoNextSymbol(..) function possibly better...
				//std::string GetNextWord(...) . Sollte eventuell auch ":" ignorieren. Multiple delimiters ("\n", " " und "\t")
				//in body nach abschlieﬂenden quotes ein ignore bis \n


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

	FileStatus XDataManager::exportXData(std::string FileName, XData Data, bool merge, bool overwrite)
	{
		return AllOk;
	}

} // namespace readable