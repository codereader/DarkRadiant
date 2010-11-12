#ifndef XDATA_H
#define XDATA_H

#include <string>
#include <vector>
#include <iostream>
#include "i18n.h"
#include <boost/shared_ptr.hpp>
#include "boost/filesystem/fstream.hpp"

#include "parser/DefTokeniser.h"

namespace XData
{

namespace
{
	// All vectors of XData-objects are initialized with this size so that no sorting is necessary, which
	// would otherwise be necessary when e.g. page2_body was defined before page1_body and a simple
	// vector.push_back(..) was used to store the data instead of a direct access using an Index.
	const std::size_t	MAX_PAGE_COUNT			= 20;

	const char*			DEFAULT_TWOSIDED_GUI	= "guis/readables/books/book_calig_mac_humaine.gui";
	const char*			DEFAULT_ONESIDED_GUI	= "guis/readables/sheets/sheet_paper_hand_nancy.gui";
	const char*			DEFAULT_SNDPAGETURN		= "readable_page_turn";
}

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
enum ExporterCommand
{
	Normal,
	Merge,
	MergeOverwriteExisting,
	Overwrite,
	OverwriteMultDef
};
enum Side
{
	Left,
	Right
};
enum ContentType
{
	Title,
	Body
};
enum PageLayout
{
	TwoSided,
	OneSided
};

class XData;
typedef boost::shared_ptr<XData> XDataPtr;

///////////////////////////// XData:
// XData base-class for exporting, storing and managing XData.
class XData
{
public:
//Methods:
	// Returns the PageLayout of the object: TwoSided or OneSided
	virtual const PageLayout getPageLayout() const = 0;

	// Exports the XData class formated properly into the File specified in
	// Filename (absolute Filepath). If the file already exists this function can overwrite the
	// file or merge. If the file cannot be opened, OpenFailed is returned.
	// 	-Merge: If the definition already exists, DefinitionExists is returned (can also return OpenFailed).
	// 	This definition	can be overwritten using the command MergeOverWriteExisting AFTER using Merge-cmd.
	// 	Otherwise the definition is appended. MergeOverwriteExisting can fail and returns MergeFailed or OpenFailed.
	// 	-Overwrite: Returns DefinitionMismatch if the definition in the target-file does not match
	// 	the name of the current definition or returns MultipleDefinitions. If a DefinitionMatch
	// 	is the case, the file is overwritten. Use the command OverwriteMultDef to overwrite the
	// 	file no matter what. With both commands OpenFailed can be returned.
	// 	If the target-file has Syntax errors, it is overwritten...
	FileStatus xport(const std::string& filename, ExporterCommand cmd);

	// Creates a OneSided XData object of an TwoSided XData object and vice versa without discarding any actual page
	// contents. numPages is adjusted accordingly and default guiPage-statements are applied. The result is stored
	// in target.
	virtual void togglePageLayout(XDataPtr& target) const = 0;

	virtual ~XData() {};

//Getters and Setters for Attributes:

	// The page-contents of the Two- and OneSided XData-objects. cc defines whether Title or Body shall be accessed.
	// The Side-parameter is discarded on OneSidedXData-objects. Throws std::runtime_error when index out of bounds.
	virtual const std::string& getPageContent(ContentType cc, std::size_t pageIndex, Side side) const = 0;
	virtual void setPageContent(ContentType cc, std::size_t pageIndex, Side side, const std::string& content) = 0;

	// The name of the XData-Definition
	const std::string& getName() const { return _name; }
	void setName(const std::string& name) { _name = name; }

	// numPages-statement. Resizes vectors accordingly. Attention: If numPages is lowered, data will obviously be discarded.
	const std::size_t getNumPages() const { return _numPages; }
	void setNumPages(std::size_t numPages)
	{
		_numPages = numPages;
		resizeVectors(numPages);
	}

	// guiPage-statement. Methods for accessing a whole StringList or single VectorElements.
	const StringList& getGuiPage() const { return _guiPage; }
	const std::string& getGuiPage(std::size_t index) const
	{
		if (index >= _numPages)
			throw std::runtime_error(_("GUI Page Index out of bounds."));
		return _guiPage[index];
	}
	void setGuiPage(const StringList& guiPage) { _guiPage = guiPage; }
	void setGuiPage(const std::string& guiPage, std::size_t index)
	{
		if (index >= _numPages)
			throw std::runtime_error(_("GUI Page Index out of bounds."));
		_guiPage[index] = guiPage;
	}

	// sndPageTurn-statement.
	const std::string& getSndPageTurn() const{ return _sndPageTurn; }
	void setSndPageTurn(const std::string& sndPageTurn) { _sndPageTurn = sndPageTurn; }

private:
//Methods for export:

	// Generates the XData-Definition for the XData-object.
	const std::string generateXDataDef() const;

	// Returns the length of the current definition. The get-pointer has to be at the beginning of that definition. Returns 0 on Syntax errors.
	const std::size_t getDefLength(const std::string& def) const;

	// Returns the definition-name found in the file or "" if multiple definitions where found.
	// Used by the xport()-method in the overwrite-command for checking for a DefinitionMatch or MultipleDefinitions.
	const std::string getDefinitionNameFromXD(boost::filesystem::ifstream& file) const;

	// Used to jump out of a definition. Can lead to undefined behavior on Syntax-errors.
	void jumpOutOfBrackets(parser::DefTokeniser& tok, int currentDepth) const;

protected:
	// Resizes all vectors to TargetSize.
	virtual void resizeVectors(std::size_t targetSize);

	// Returns the OneSided or TwoSided Page-content-definition of an XData object.
	virtual const std::string getContentDef() const = 0;

	// Generates the XData-compatible definition for Multi-line strings and adds escape-characters before quotes.
	const std::string generateTextDef(const std::string& rawString) const;

	//Attributes:
	std::string	_name;				//name of the XData-Definition
	std::size_t _numPages;			//numPages-statement
	StringList	_guiPage;			//guiPage-statements
	std::string _sndPageTurn;		//sndPageTurn-statement
	std::size_t _definitionStart;	//Marks the start of a definition in an .xd-File. Used by the xport()-method.
};


class OneSidedXData : public XData
{
private:
//page contents
	StringList	_pageTitle;
	StringList	_pageBody;
//end of page contents
	const std::string getContentDef() const;
	void resizeVectors(std::size_t targetSize);
public:

	void setPageContent(ContentType cc, std::size_t pageIndex, Side side, const std::string& content);

	const std::string& getPageContent(ContentType cc, std::size_t pageIndex, Side side) const;

	const PageLayout getPageLayout() const { return OneSided; }

	void togglePageLayout(XDataPtr& target) const;

	OneSidedXData(const std::string& name)
	{
		_name = name;
		setNumPages(MAX_PAGE_COUNT);
	}
	~OneSidedXData()
	{
		_guiPage.clear();
		_pageTitle.clear();
		_pageBody.clear();
	}
};
typedef boost::shared_ptr<OneSidedXData> OneSidedXDataPtr;

class TwoSidedXData : public XData
{
private:
//page contents:
	StringList	_pageLeftTitle;
	StringList	_pageRightTitle;
	StringList	_pageLeftBody;
	StringList	_pageRightBody;
//end of page contents
	const std::string getContentDef() const;
	void resizeVectors(std::size_t targetSize);
public:

	void setPageContent(ContentType cc, std::size_t pageIndex, Side side, const std::string& content);

	const std::string& getPageContent(ContentType cc, std::size_t pageIndex, Side side) const;

	const PageLayout getPageLayout() const { return TwoSided; }

	void togglePageLayout(XDataPtr& target) const;

	TwoSidedXData(const std::string& name)
	{
		_name=name;
		setNumPages(MAX_PAGE_COUNT);
	}
	~TwoSidedXData()
	{
		_pageLeftTitle.clear();
		_pageLeftBody.clear();
		_pageRightBody.clear();
		_pageRightTitle.clear();
		_guiPage.clear();
	}
};
typedef boost::shared_ptr<TwoSidedXData> TwoSidedXDataPtr;

} //namespace XData

#endif /* XDATA_H */
