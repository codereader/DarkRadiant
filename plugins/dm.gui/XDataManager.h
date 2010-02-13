#include <string>
#include <vector>
#include "boost/filesystem/fstream.hpp"
#include "boost/lexical_cast.hpp"
#include <boost/shared_ptr.hpp>

#include "itextstream.h"

#include "parser/DefTokeniser.h"
#include "reportError.h"
#include <stdio.h>


namespace readable
{
	typedef std::vector<std::string> StringList;

	///////////////////////////// XData:
	// XData base-class. (Container)
	class XData
	{
	public:
		std::string _name;
		int _numPages;
		StringList _guiPage;
		std::string _sndPageTurn;

		virtual ~XData() {};
	};
	typedef boost::shared_ptr<XData> XDataPtr;
	typedef std::vector<XDataPtr> XDataPtrList;

	class OneSidedXData : public XData
	{
	public:
		StringList _pageTitle;
		StringList _pageBody;

		OneSidedXData(std::string name) { _name=name; }
		~OneSidedXData() {}
	};
	typedef boost::shared_ptr<OneSidedXData> OneSidedXDataPtr;

	class TwoSidedXData : public XData
	{
	public:
		StringList _pageLeftTitle;
		StringList _pageRightTitle;
		StringList _pageLeftBody;
		StringList _pageRightBody;

		TwoSidedXData(std::string name) { _name=name; }
		~TwoSidedXData() {}
	};
	typedef boost::shared_ptr<TwoSidedXData> TwoSidedXDataPtr;

	enum FileStatus
	{
		FileExists,
		DefinitionExists,
		MultipleDefinitions,
		AllOk
	};

	enum ExporterCommands
	{
		Normal,
		Merge,
		MergeAndOverwriteExisting,
		Overwrite,
		OverwriteMultDef		
	};

	struct XDataParse
	{
		XDataPtr xData;
		StringList error_msg;
	};

	///////////////////////////// XDataManager:
	// Small static class that imports and exports XData.
	class XDataManager
	{
	private:
		static std::string generateXDataDef(const XData& Data);
		static XDataParse parseXDataDef(parser::DefTokeniser& tok);
		static std::string parseText(parser::DefTokeniser& tok);
		static void jumpOutOfBrackets(parser::DefTokeniser& tok, int CurrentDepth);
	public:
		/* Imports a list of XData objects from the File specified by Filename. Throws
		runtime_error exceptions on filesystemerrors, syntax errors and general exceptions.*/
		static XDataPtrList importXData(const std::string& FileName);

		/* Exports the XData class formated properly into the File specified in 
		Filename. If the file already exists this function can overwrite the file or merge.
		If the definition contained in XData already exists, it can be merged into the file,
		replacing the old one. Throws corresponding runtime_error exceptions in these cases
		and if the file to be overwritten contains more than one definitions.*/
		static FileStatus exportXData(const std::string& FileName, const XData& Data, const ExporterCommands& cmd);
	};


} //namespace readable