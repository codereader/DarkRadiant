#ifndef XDATALOADER_H
#define XDATALOADER_H

#include "XData.h"

#include "parser/DefTokeniser.h"
#include <map>
#include "ifilesystem.h"

namespace XData
{

namespace
{
	const std::string XDATA_DIR = "xdata/";
	const std::string XDATA_EXT = "xd";
}


typedef std::set<std::string> StringSet;
typedef std::map<std::string, XDataPtr> XDataMap;
typedef std::map<std::string, std::vector<std::string> > StringVectorMap;
typedef std::map<std::string, std::string > StringMap;
typedef std::vector<std::pair<std::string, std::string> > StringPairList;


///////////////////////////// XDataSelector:
// Class for importing XData from files and retrieving fileinfos: duplicated definitions,
// List of all definitions with their corresponding filename.
class XDataLoader :
	public VirtualFileSystem::Visitor
{
	// Notes:
	//	-Importer cannot cope with multiple definitions in a single file currently.
public:
	// Imports a MultiMap of XData-Pointers sorted by name from the specified File (just the name, not the path).
	// Returns false if import failed. The import-breaking error-message is the last element of _errorList, which can
	// be retrieved by calling getImportSummary() and also stores other errors and warnings.
	//	-target: Key Value = definitionName, Mapped Value = XData object.
	const bool import(const std::string& filename, XDataMap& target);

	// Imports a single Definition from the specified file (filename) if defined or all duplicated definitions if only
	// the definitionName is specified. Returns false if import failed. The import-breaking error-message is the last
	// element of _errorList, which can be retrieved by calling getImportSummary() and also stores other errors and warnings.
	//	-target: Key Value = filepath INCLUDING ModPath(!!), Mapped Value = XData object.
	const bool importDef(const std::string& definitionName, XDataMap& target, const std::string& filename = "");

	virtual ~XDataLoader()
	{
		_defMap.clear();
		_duplicatedDefs.clear();
		_fileSet.clear();
		_errorList.clear();
		_guiPageError.clear();
		_guiPage.clear();
	}

//Getters:
	// Returns StringVector with errors and warnings of the last import process
	// as well as a brief summary in the last element of the vector.
	const StringList& getImportSummary() const
	{
		return _errorList;
	}

	// Returns Map of duplicated definitions. (Data might be outdated, maybe use retrieveXdInfo() before)
	// Key Value = DefinitionNames, Mapped Value = StringVector of corresponding filenames.
	const StringVectorMap& getDuplicateDefinitions() const
	{
		if (_duplicatedDefs.empty())
		{
			throw std::runtime_error("No Data available. Call retrieveXdInfo() before.");
		}

		return _duplicatedDefs;
	}

	// Returns StringSet of all .xd-files in the VFS. (Data might be outdated, maybe use retrieveXdInfo() before)
	const StringSet& getXdFilenameList() const
	{
		if (_fileSet.empty())
			throw std::runtime_error("No Data available. Call retrieveXdInfo() before.");
		return _fileSet;
	}

	// Returns a map of all Definitions and their corresponding filenames found in the VFS. The filenames are stored
	// in a vector in case a definition exists multiple times. (Data might be outdated, maybe use retrieveXdInfo() before)
	const StringVectorMap& getDefinitionList() const
	{
		if (_defMap.empty())
			throw std::runtime_error("No Data available. Call retrieveXdInfo() before.");
		return _defMap;
	}

	// Retrieves all XData-related information found in the VFS.
	void retrieveXdInfo();

//FileVisitor-related:
	// Functor operator: Adds all definitions found in the target file to the _defMap.
	void visit(const std::string& filename);

private:
	// Issues the ErrorMessage to the cerr console and appends it to the _errorList. Returns always false, so that it can be used after a return statement.
	const bool reportError(const std::string& ErrorMessage)
	{
		std::cerr << ErrorMessage;
		_errorList.push_back(ErrorMessage);
		return false;
	}

	// Additionally to what the upper version of reportError(..) does, this method also tries to jump to the next definition by calling jumpOutOfBrackets.
	const bool reportError(parser::DefTokeniser& tok, const std::string& ErrorMessage, std::size_t currentDepth = 1)
	{
		reportError(ErrorMessage);
		jumpOutOfBrackets(tok, currentDepth);
		return false;
	}

	// Opens the file in which sourceDef is contained, while handling duplicate definitions.
	//	-statements: Key value = Statements in sourceDef, Mapped value = Statement in the calling definition to whom the imported content shall be parsed.
	//	-defName: Name of the definition that has induced the recursiveImport. This is just used for error reporting.
	//	-importContent: Vector of stringpairs. First = Name of the destination Statement in the calling definition, Second = Imported Content.
	const bool recursiveImport(const std::string& sourceDef, const StringMap& statements, const std::string& defName, StringPairList& importContent);

	// Parses the contents of the import-statement.
	const bool getImportParameters(parser::DefTokeniser& tok, StringMap& statements, std::string& sourceDef, const std::string& defName);

	// Checks where the content following in the tokenizer has to be stored. DefName is the name of the
	// definition for whom content is parsed and is just used for error-messages. If tok is NULL, the string content is stored. This
	// is only induced by the import-directive, which on the contrary must have tok!= NULL.
	const bool storeContent(const std::string& statement, parser::DefTokeniser* tok, const std::string& defName, const std::string& content = "");

	// Parses a single definition from a stream into an XData object and generates warning and error messages. If definitionName
	// is defined, only matching definitions will be parsed and return false otherwise.
	const bool parseXDataDef(parser::DefTokeniser& tok, const std::string& definitionName = "");

	// Skips the ":" and parses the following SingleLine or MultiLine content into What.
	const bool readLines(parser::DefTokeniser& tok, std::string& what) const;

	// Used to jump out of a definition. Can lead to undefined behavior on Syntax-errors.
	void jumpOutOfBrackets(parser::DefTokeniser& tok, std::size_t currentDepth = 1) const;

//General Member variables:
	StringList			_errorList;
	StringVectorMap		_defMap;
	StringSet			_fileSet;
	StringVectorMap		_duplicatedDefs;

//Helper-variables for import:
	XDataPtr			_newXData;
	std::string			_name;
	StringList			_guiPageError;
	std::size_t			_maxPageCount;
	std::size_t			_maxGuiNumber;
	std::string			_guiPageDef;
	std::size_t			_numPages;
	std::string			_sndPageTurn;
	StringList			_guiPage;
}; // XDataLoader class
typedef boost::shared_ptr<XDataLoader> XDataLoaderPtr;

} // namespace XData

#endif /* XDATALOADER_H */
