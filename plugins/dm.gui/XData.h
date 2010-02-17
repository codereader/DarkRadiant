#ifndef XDATA_H
#define XDATA_H

#include <string>
#include <vector>
#include "boost/filesystem/fstream.hpp"
#include "boost/lexical_cast.hpp"
#include <boost/shared_ptr.hpp>
#include <iostream>
#include "parser/DefTokeniser.h"

/* ToDo:
	1) Use VFS instead of boost::filesystem for importer. ->done
	2) import-directive support for exporter? */

namespace readable
{
	typedef std::vector<std::string> StringList;

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
	// XData base-class for exporting, storing and managing XData.
	class XData
	{
	public:
	//Methods:
		/* Resizes all vectors to TargetSize. */
		virtual void resizeVectors(const int& TargetSize);

		/* Returns the PageLayout of the object: TwoSided or OneSided */
		virtual PageLayout getPageLayout() = 0;

		/* Exports the XData class formated properly into the File specified in 
		Filename (absolute Filepath). If the file already exists this function can overwrite the
		file or merge. If the file cannot be opened, OpenFailed is returned.
			-Merge: If the definition already exists, DefinitionExists is returned. This definition
			can be overwritten using the command MergeOverWriteExisting. Otherwise the definition
			is appended. MergeOverwriteExisting can fail and returns MergeFailed then.
			-Overwrite: Returns DefinitionMismatch if the definition in the target-file does not match
			the name of the current definition or returns MultipleDefinitions. If a DefinitionMatch 
			is the case, the file is overwritten. Use the command OverwriteMultDef to overwrite the
			file no matter what. If the target-file has Syntax errors, it is overwritten...*/
		FileStatus xport(const std::string& FileName, const ExporterCommands& cmd);

		virtual ~XData() {};

	//Getters and Setters for Attributes:

		/* The page-contents of the Two- and OneSided XData-objects. cc defines whether Title or Body shall be accessed.
		The SideChooser-parameter is discarded on OneSidedXData-objects.*/
		virtual std::string getPageContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side) = 0;	//can throw: vector subscript dimensions exceeded.
		virtual void setPageContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side, const std::string& content) = 0;	//can throw: vector subscript dimensions exceeded.

		/* The name of the XData-Definition */
		std::string getName() { return _name; }
		void setName(const std::string& name) { _name = name; }

		/* numPages-statement */
		int getNumPages() { return _numPages; }
		void setNumPages(const int& numPages) { _numPages = numPages; }

		/* guiPage-statement. Methods for accessing a whole StringList or single VectorElements. */
		StringList getGuiPage() { return _guiPage; }
		std::string getGuiPage(int Index) { return _guiPage[Index]; }	//can throw: vector subscript dimensions exceeded.
		void setGuiPage(const StringList& guiPage) { _guiPage = guiPage; }
		void setGuiPage(const std::string& guiPage, int Index) { _guiPage[Index] = guiPage; }	//can throw: vector subscript dimensions exceeded.

		/* sndPageTurn-statement. */
		std::string getSndPageTurn() { return _sndPageTurn; }
		void setSndPageTurn(const std::string& sndPageTurn) { _sndPageTurn = sndPageTurn; }

	private:
	//Methods for export:

		/* Generates the XData-Definition for the XData-object. */
		std::string generateXDataDef();

		/* Returns the length of the current definition. The get-pointer has to be at the beginning of that definition. Returns 0 on Syntax errors. */
		int getDefLength(boost::filesystem::fstream& file);

		/* Returns the definition-name found in the file or "" if multiple definitions where found. Used by the xport()-method in the overwrite-command for checking for a DefinitionMatch or MultipleDefinitions.*/
		std::string getDefinitionNameFromXD(boost::filesystem::ifstream& file);	//can throw

		/* Used to jump out of a definition. Can lead to undefined behavior on Syntax-errors. */
		void jumpOutOfBrackets(parser::DefTokeniser& tok, int CurrentDepth);

	protected:

		/* Returns the OneSided or TwoSided Page-content-definition of an XData object*/
		virtual std::string getContentDef() = 0;	//can throw

		/* Generates the XData-compatible definition for Multi-line strings and adds escape-characters before quotes. */
		std::string generateTextDef(std::string String);

		//Attributes:
		std::string _name;			//name of the XData-Definition
		int _numPages;				//numPages-statement
		StringList _guiPage;		//guiPage-statements
		std::string _sndPageTurn;	//sndPageTurn-statement
		int definitionStart;		//Marks the start of a definition in an .xd-File. Used by the xport()-method.
	};
	typedef boost::shared_ptr<XData> XDataPtr;


	class OneSidedXData : public XData
	{
	private:
	//page contents
		StringList _pageTitle;
		StringList _pageBody;
	//end of page contents
		std::string getContentDef();
	public:

		void resizeVectors(const int& TargetSize);

		void setPageContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side, const std::string& content);

		std::string getPageContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side);

		PageLayout getPageLayout() { return OneSided; }

		OneSidedXData(std::string name) { _name=name; }
		~OneSidedXData() {}
	};
	typedef boost::shared_ptr<OneSidedXData> OneSidedXDataPtr;

	class TwoSidedXData : public XData
	{
	private:
	//page contents:
		StringList _pageLeftTitle;
		StringList _pageRightTitle;
		StringList _pageLeftBody;
		StringList _pageRightBody;
	//end of page contents
		std::string getContentDef();
	public:

		void resizeVectors(const int& TargetSize);

		void setPageContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side, const std::string& content);

		std::string getPageContent(const ContentChooser& cc, const int& PageIndex, const SideChooser& Side);

		PageLayout getPageLayout() { return TwoSided; }

		TwoSidedXData(std::string name) { _name=name; }
		~TwoSidedXData() {}
	};
	typedef boost::shared_ptr<TwoSidedXData> TwoSidedXDataPtr;

} //namespace readable

#endif XDATA_H