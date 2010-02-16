#include <string>
#include <vector>
#include "boost/filesystem/fstream.hpp"
#include "boost/lexical_cast.hpp"
#include <boost/shared_ptr.hpp>

#include "itextstream.h"

#include "parser/DefTokeniser.h"
#include "reportError.h"
#include <stdio.h>
#include <map>

/* ToDo:
	1) Use VFS instead of boost::filesystem for importer. 
	2) import-directive support for exporter. */

namespace readable
{
	typedef std::vector<std::string> StringList;
	class XData;
	typedef boost::shared_ptr<XData> XDataPtr;
	typedef std::vector<XDataPtr> XDataPtrList;
	enum FileStatus
	{
		FileExists,
		DefinitionExists,
		MultipleDefinitions,
		DefinitionMismatch,
		MergeFailed,
		OpenFailed,
		AllOk
	};
	enum ExporterCommands
	{
		Normal,
		Merge,
		MergeOverwriteExisting,
		Overwrite,
		OverwriteMultDef		
	};
	struct XDataParse		//replace with std::map ?
	{
		XDataPtr xData;
		StringList error_msg;
	};
	enum SideChooser
	{
		Left,
		Right
	};
	enum ContentChooser
	{
		Title,
		Body
	};
	enum PageLayout
	{
		TwoSided,
		OneSided
	};
	typedef std::map<std::string, std::string> StringMap;

	///////////////////////////// XData:
	// XData base-class for importing, exporting and managing XData.
	class XData
	{
	private:
	//Methods for import:
		static XDataParse parseXDataDef(parser::DefTokeniser& tok);
		static std::string parseText(parser::DefTokeniser& tok);
		static void jumpOutOfBrackets(parser::DefTokeniser& tok, int CurrentDepth);
		static void importDirective(parser::DefTokeniser& tok, XDataParse& NewXData, const std::string name);
		static StringMap grabAllDefinitions();	//not yet implemented...
		static StringMap& getDefMap();
	//Methods for export:
		std::string generateXDataDef();
		int getDefLength(boost::filesystem::fstream& file);
		int definitionStart;
		std::string getDefinitionNameFromXD(boost::filesystem::ifstream& file);	//can throw
		
	
	protected:
		virtual std::string getContentDef() = 0;	//can throw
		std::string generateTextDef(std::string String);
	//Attributes:
		std::string _name;
		int _numPages;
		StringList _guiPage;
		std::string _sndPageTurn;
		//static StringMap XDataDefMap;

	public:

	//Getters and Setters for Attributes:			Should reference-to-const be used here?
		std::string getName() { return _name; }
		void setName(std::string name) { _name = name; }

		int getNumPages() { return _numPages; }
		void setNumPages(int numPages) { _numPages = numPages; }

		StringList getGuiPage() { return _guiPage; }
		std::string getGuiPage(int Index) { return _guiPage[Index]; }	//can throw: vector subscript dimensions exceeded.
		void setGuiPage(StringList guiPage) { _guiPage = guiPage; }
		void setGuiPage(std::string guiPage, int Index) { _guiPage[Index] = guiPage; }	//can throw: vector subscript dimensions exceeded.

		std::string getSndPageTurn() { return _sndPageTurn; }
		void setSndPageTurn(std::string sndPageTurn) { _sndPageTurn = sndPageTurn; }

	//Methods:
		/* Resizes all vectors to TargetSize. */
		virtual void resizeVectors(const int& TargetSize);

		/* Sets the page-contents of the Two- and OneSided XData-objects. cc defines whether Title or Body shall be accessed.
		The Side-parameter is discarded on OneSidedXData-objects.*/
		virtual void setContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side, const std::string& content) = 0;	//can throw

		/* Returns the page-contents of the Two- and OneSided XData-objects. cc defines whether Title or Body shall be accessed.
		The Side-parameter is discarded on OneSidedXData-objects. */
		virtual std::string getContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side) = 0;	//can throw

		/* Returns the PageLayout of the object: TwoSided or OneSided */
		virtual PageLayout getPageLayout() = 0;

		/* Exports the XData class formated properly into the File specified in 
		Filename. If the file already exists this function can overwrite the file or merge.
		If the file cannot be opened, OpenFailed is returned.
		-Merge: If the definition already exists, DefinitionExists is returned. This definition
			can be overwritten using the command MergeOverWriteExisting. Otherwise the definition
			is appended. MergeOverwriteExisting can fail and returns MergeFailed then.
		-Overwrite: Returns DefinitionMismatch if the definition in the targetfile does not match
			the name of the current definition or returns MultipleDefinitions. If a DefinitionMatch 
			is the case, the file is overwritten. Use the command OverwriteMultDef to overwrite the
			file no matter what. If the targetfile has Syntax errors, it is overwritten...*/
		FileStatus xport(const std::string& FileName, const ExporterCommands& cmd);

		/* Imports a list of XData objects from the File specified by Filename. Throws
		runtime_error exceptions on filesystemerrors, syntax errors and general exceptions.*/
		static XDataPtrList importXDataFromFile(const std::string& FileName);

		virtual ~XData() {};
	};
	

	class OneSidedXData : public XData
	{
	private:
		StringList _pageTitle;
		StringList _pageBody;
		std::string getContentDef();
	public:

		void resizeVectors(const int& TargetSize);	//not yet implemented

		void setContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side, const std::string& content);	//not yet implemented.

		std::string getContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side);	//not yet implemented

		PageLayout getPageLayout() { return OneSided; }

		OneSidedXData(std::string name) { _name=name; }
		~OneSidedXData() {}
	};
	typedef boost::shared_ptr<OneSidedXData> OneSidedXDataPtr;

	class TwoSidedXData : public XData
	{
	private:
		StringList _pageLeftTitle;
		StringList _pageRightTitle;
		StringList _pageLeftBody;
		StringList _pageRightBody;
		std::string getContentDef();
	public:

		void resizeVectors(const int& TargetSize);	//not yet implemented

		void setContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side, const std::string& content);	//not yet implemented.

		std::string getContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side);	//not yet implemented

		PageLayout getPageLayout() { return TwoSided; }

		TwoSidedXData(std::string name) { _name=name; }
		~TwoSidedXData() {}
	};
	typedef boost::shared_ptr<TwoSidedXData> TwoSidedXDataPtr;

} //namespace readable