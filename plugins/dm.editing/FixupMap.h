#ifndef _FIXUP_MAP_H_
#define _FIXUP_MAP_H_

#include <string>
#include <map>
#include "gtkutil/ModalProgressDialog.h"

class FixupMap
{
public:
	struct Result
	{
		std::size_t replacedEntities;
		std::size_t replacedShaders;
		std::size_t replacedModels;
		std::size_t replacedMisc;

		// Errors, sorted by line
		typedef std::map<std::size_t, std::string> ErrorMap;
		ErrorMap errors;

		Result() :
			replacedEntities(0),
			replacedShaders(0),
			replacedModels(0),
			replacedMisc(0)
		{}
	};

private:
	// Path to fixup file
	std::string _filename;

	// Fixup contents
	std::string _contents;

	std::size_t _curLineNumber;

	Result _result;

	gtkutil::ModalProgressDialog _progress;

public:
	// Pass the fixup filename to the constructor
	FixupMap(const std::string& filename);

	// Run the fixup process
	Result perform();

private:
	void loadFixupFile();
	void loadDeprecatedEntities();

	void performFixup(const std::string& line);

	void replaceShader(const std::string& oldShader, const std::string& newShader);
	void replaceSpawnarg(const std::string& oldVal, const std::string& newVal);
};

#endif /* _FIXUP_MAP_H_ */
