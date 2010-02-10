#include <string>
#include <vector>
#include "boost/filesystem/fstream.hpp"
#include "boost/lexical_cast.hpp"
#include <boost/shared_ptr.hpp>

#include "itextstream.h"

#include "reportError.h"


namespace readable
{
	typedef std::vector<std::string> StringList;

	///////////////////////////// XData:
	// XData Containers.
	class XData
	{
	public:
		bool _twoSided;
		std::string _name;
		int _numPages;
		StringList _guiPage;
		std::string _sndPageTurn;

		XData(std::string name) : _name(name) { }
		XData(bool twoSided, std::string name) : _twoSided(twoSided), _name(name) { }
	};
	typedef boost::shared_ptr<XData> XDataPtr;
	typedef std::vector<XDataPtr> XDataPtrList;

	class OneSidedXData : public XData
	{
	public:
		StringList _pageTitle;
		StringList _pageBody;

		OneSidedXData(std::string name) : XData(false, name) { }
	};

	class TwoSidedXData : public XData
	{
	public:
		StringList _pageLeftTitle;
		StringList _pageRightTitle;
		StringList _pageLeftBody;
		StringList _pageRightBody;

		TwoSidedXData(std::string name) : XData(true, name) { }
	};

	enum FileStatus
	{
		FileExists,
		DefinitionExists,
		MultipleDefinitions,
		AllOk
	};



	///////////////////////////// XDataManager:
	// Small static class that imports and exports XData.
	class XDataManager
	{
	private:
		static void trimLeadingSpaces(std::string& String);
		static std::string getLineFormatted(boost::filesystem::ifstream* FileStream);
		static std::string getLineFormatted(boost::filesystem::ifstream* FileStream, char Delimiter);
		static std::string gotoNextSymbol(boost::filesystem::ifstream* FileStream);	//to be implemented
		static std::string getNextWord(boost::filesystem::ifstream* FileStream); //to be implemented
		//static void deQuote(std::string& String); //to be implemented. Possibly unnecessary?
		static int _lineCount;
	public:
		/* Imports a list of XData objects from the File specified by Filename. Throws
		runtime_error exceptions on filesystemerrors, syntax errors and general exceptions.*/
		static XDataPtrList importXData(std::string FileName);

		/* Exports the XData class formated properly into the File specified in 
		Filename. If the file already exists this function can overwrite the file or merge.
		If the definition contained in XData already exists, it can be merged into the file,
		replacing the old one. Throws corresponding runtime_error exceptions in these cases
		and if the file to be overwritten contains more than one definitions.*/
		static FileStatus exportXData(std::string FileName, XData Data, bool merge, bool overwrite);
	};


} //namespace readable