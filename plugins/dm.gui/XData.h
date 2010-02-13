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
	class XData;
	typedef boost::shared_ptr<XData> XDataPtr;
	typedef std::vector<XDataPtr> XDataPtrList;
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

	///////////////////////////// XData:
	// XData base-class for importing, exporting and managing XData.
	class XData
	{
	private:
		static XDataParse parseXDataDef(parser::DefTokeniser& tok);
		static std::string parseText(parser::DefTokeniser& tok);
		static void jumpOutOfBrackets(parser::DefTokeniser& tok, int CurrentDepth);
		std::string generateXDataDef();
		
	
	protected:
		virtual std::string getContentDef() = 0;	//can throw
	//Attributes:
		std::string _name;
		int _numPages;
		StringList _guiPage;
		std::string _sndPageTurn;

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
		If the definition contained in XData already exists, it can be merged into the file,
		replacing the old one. Throws corresponding runtime_error exceptions in these cases
		and if the file to be overwritten contains more than one definitions.*/
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