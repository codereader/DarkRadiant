#include "XData.h"
#include "ifilesystem.h"
#include "iarchive.h"
#include "parser/DefTokeniser.h"
#include <map>
#include "generic/callback.h"

//#include "reportError.h"

#include "debugging/ScopedDebugTimer.h"


namespace readable
{
	namespace
	{
		const std::string	DEFAULT_TWOSIDED_LAYOUT = "guis/readables/books/book_calig_mac_humaine.gui";
		const std::string	DEFAULT_ONESIDED_LAYOUT = "guis/readables/sheets/sheet_paper_hand_nancy.gui";
		const std::string	DEFAULT_SNDPAGETURN		= "readable_page_turn";

		const std::string	XDATA_DIR				= "xdata/";
		const std::string	XDATA_EXT				= "xd";
	}

	typedef std::map<std::string, std::string> StringMap;
	typedef std::set<std::string> StringSet;
	typedef std::vector<XDataPtr> XDataPtrList;
	typedef std::map<std::string, std::vector<std::string> > DuplicatedDefsMap;

	///////////////////////////// XDataLoader
	// Class for importing XData from files.
	class XDataLoader
	{
	/* ToDo:
		1) Maybe add detection of \n in xdata files as a error/warning. 
			(Basically not necessary because everything is exported correctly and it would decrease performance.) 
		2) Multiple Stage Import-directive support. Probably need a recursive method for this.
		3) Replace XDataPtrList with a XDataPtrSet with custom Allocator and weak-sorter. 
		4) Possibly have import methods return bool and pass a target reference.	->done
		5) Replace DefMap with a multiMap, to allow editing of all readables but still warn about duplicates. 
		6) Add local reportError-Method and change the cerr output of import methods.	->done*/

	public:
		/* Imports a list of XData objects from the File specified by Filename (just the name, not the path).
		Returns false if import failed. The import-breaking error-message is the last element of _errorList, which can
		be retrieved by calling getImportSummary() and also stores other errors and warnings. */
		const bool import(const std::string& filename, XDataPtrList& target);

		/* Imports a single Definition from the specified file. Returns false if import failed. The import-breaking 
		error-message is the last element of _errorList, which can be retrieved by calling getImportSummary() 
		and also stores other errors and warnings. */
		const bool importSingleDef(const std::string& filename, const std::string& definitionName, XDataPtr& target);
	
	//Getters:
		/* Returns StringVector with errors and warnings of the last import process 
		as well as a brief summary in the last element of the vector. */
		const StringList& getImportSummary() const { return _errorList; }

		/* Returns Map of duplicated definitions. (Data might be outdated, maybe use retrieveXdInfo() before)
		Key Value = DefinitionNames, Mapped Value = StringVector of corresponding filenames.*/
		const DuplicatedDefsMap& getDuplicateDefinitions() const
		{ 
			if (_duplicatedDefs.empty())
				throw std::runtime_error("No Data available. Call retrieveXdInfo() before.");
			return _duplicatedDefs;
		}

		/* Returns StringSet of all .xd-files in the VFS. (Data might be outdated, maybe use retrieveXdInfo() before)*/
		const StringSet& getXdFilenameList() const 
		{ 
			if (_fileSet.empty())
				throw std::runtime_error("No Data available. Call retrieveXdInfo() before.");
			return _fileSet;
		}

		/* Returns sorted StringVector of all Definitions found. (Data might be outdated, maybe use retrieveXdInfo() before)*/
		const StringList& getDefinitionList() const
		{
			if (_definitionList.empty())
				throw std::runtime_error("No Data available. Call retrieveXdInfo() before.");
			return _definitionList;
		}

		/* Retrieves all XData-related information found in the VFS. */
		void retrieveXdInfo();

	//FileVisitor-related:
		/* Functor operator: Adds all definitions found in the target file to the _defMap. */
		void operator() (const std::string& filename);

		/* Required typedef for the Callback to work. */
		typedef const std::string& first_argument_type;

	private:
		/* Issues the ErrorMessage to the cerr console and appends it to the _errorList. Returns always false, so that it can be used after a return statement. */
		inline const bool reportError(const std::string& ErrorMessage)
		{
			std::cerr << ErrorMessage;
			_errorList.push_back(ErrorMessage);
			return false;
		}

		/* Additionally to what the upper version of reportError(..) does, this method also tries to jump to the next definition by calling jumpOutOfBrackets. */
		inline const bool reportError(parser::DefTokeniser& tok, const std::string& ErrorMessage, std::size_t currentDepth = 1)
		{ 
			reportError(ErrorMessage);
			jumpOutOfBrackets(tok, currentDepth);
			return false; 
		}

		/* Checks where the content following in the tokenizer has to be stored. DefName is the name of the
		definition for whom content is parsed and is just used for error-messages. */
		const bool storeContent(const std::string& statement, parser::DefTokeniser& tok, const std::string& defName);

		/* Parses a single definition from a stream into an XData object and generates warning and error messages. */
		const bool parseXDataDef(parser::DefTokeniser& tok, const std::string& definitionName = "");

		/* Skips the ":" and parses the following SingleLine or MultiLine content into What.*/
		const bool readLines(parser::DefTokeniser& tok, std::string& what) const;

		/* Used to jump out of a definition. Can lead to undefined behavior on Syntax-errors. */
		void jumpOutOfBrackets(parser::DefTokeniser& tok, std::size_t currentDepth = 1) const;

	//General Member variables:
		StringList			_errorList;
		StringMap			_defMap;
		StringList			_definitionList;
		StringSet			_fileSet;
		DuplicatedDefsMap	_duplicatedDefs;

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
	};
}
