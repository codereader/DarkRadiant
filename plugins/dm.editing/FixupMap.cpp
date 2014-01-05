#include "FixupMap.h"

#include <fstream>
#include <iostream>
#include <vector>
#include "i18n.h"
#include "iscenegraph.h"
#include "iundo.h"
#include "imainframe.h"
#include "ieclass.h"

#include "gtkutil/dialog/MessageBox.h"
#include "os/file.h"
#include "string/string.h"
#include "parser/Tokeniser.h"

#include "ShaderReplacer.h"
#include "SpawnargReplacer.h"
#include "DeprecatedEclassCollector.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

FixupMap::FixupMap(const std::string& filename) :
	_filename(filename),
	_progress(GlobalMainFrame().getTopLevelWindow(), _("Fixup in progress"))
{}

FixupMap::Result FixupMap::perform()
{
	UndoableCommand cmd("performFixup");

	// Load contents
	loadFixupFile();

	// Load deprecated entities
	loadDeprecatedEntities();

	// Instantiate a line tokeniser
	parser::BasicStringTokeniser tokeniser(_contents, "\n\r");

	_curLineNumber = 0;
	std::size_t parsedSize = 0;

	while (tokeniser.hasMoreTokens())
	{
		_curLineNumber++;

		std::string line = tokeniser.nextToken();

		performFixup(line);

		// Approximate the progress
		parsedSize += line.size();

		try
		{
			double fraction = static_cast<double>(parsedSize) / _contents.size();
			_progress.setTextAndFraction(
				(boost::format(_("Processing line %d...")) % _curLineNumber).str(),
				fraction);
		}
		catch (gtkutil::ModalProgressDialog::OperationAbortedException& ex)
		{
			gtkutil::MessageBox box(_("Fixup cancelled"), ex.what(), ui::IDialog::MESSAGE_ERROR);
			box.run();
			return _result;
		}
	}

	_progress.setTextAndFraction(_("Completed"), 1.0);

	return _result;
}

void FixupMap::performFixup(const std::string& line)
{
	// Skip empty lines and comments
	if (line.empty() ||
		boost::algorithm::starts_with(line, "#") ||
		boost::algorithm::starts_with(line, "//"))
	{
		return;
	}

	boost::regex expr("^" + MATERIAL_PREFIX + "(.*)\\s=>\\s(.*)$");
	boost::smatch matches;

	if (boost::regex_match(line, matches, expr))
	{
		// Fixup a specific shader
		std::string oldShader = matches[1];
		std::string newShader = matches[2];

		replaceShader(oldShader, newShader);
		return;
	}

	expr = boost::regex("^" + ENTITYDEF_PREFIX + "(.*)\\s=>\\s(.*)$");

	if (boost::regex_match(line, matches, expr))
	{
		// Fixup a specific entitydef
		std::string oldDef = matches[1];
		std::string newDef = matches[2];

		// Search all spawnargs
		replaceSpawnarg(oldDef, newDef);
		return;
	}

	// No specific prefix, this can be everything: spawnarg or texture
	expr = boost::regex("^(.*)\\s=>\\s(.*)$");

	if (boost::regex_match(line, matches, expr))
	{
		std::string oldStr = matches[1];
		std::string newStr = matches[2];

		// First, try to use these strings as shader replacement
		replaceShader(oldStr, newStr);

		// Second, traverse all entities and fix them up
		replaceSpawnarg(oldStr, newStr);
		return;
	}
}

void FixupMap::replaceSpawnarg(const std::string& oldVal, const std::string& newVal)
{
	SpawnargReplacer replacer(oldVal, newVal);
	GlobalSceneGraph().root()->traverseChildren(replacer);

	replacer.processEntities();

	_result.replacedModels += replacer.getModelCount();
	_result.replacedEntities += replacer.getEclassCount();
	_result.replacedMisc += replacer.getOtherCount();
}

void FixupMap::replaceShader(const std::string& oldShader, const std::string& newShader)
{
	// Instantiate a walker
	ShaderReplacer replacer(oldShader, newShader);
	GlobalSceneGraph().root()->traverseChildren(replacer);

	_result.replacedShaders += replacer.getReplaceCount();
}

void FixupMap::loadFixupFile()
{
	// Sanity-check the file
	if (!os::fileOrDirExists(_filename) || !file_readable(_filename.c_str()))
	{
		gtkutil::MessageBox box(_("File not readable"),
			_("The specified file doesn't exist."), ui::IDialog::MESSAGE_ERROR);
		box.run();
		return;
	}

	// Load the file's contents
	std::ifstream input;
	input.open(_filename.c_str(), std::ios::in|std::ios::ate);

	if (!input)
	{
		gtkutil::MessageBox box(_("File not readable"),
			_("The specified file can't be opened."), ui::IDialog::MESSAGE_ERROR);
		box.run();
		return;
	}

	std::vector<char> buffer;
	buffer.resize(input.tellg());

	input.seekg(0, std::ios::beg);
	input.read(&buffer.front(), buffer.size());
	input.close();

	_contents = &buffer.front();
}

void FixupMap::loadDeprecatedEntities()
{
	// Traverse all eclasses and collect replacements
	DeprecatedEclassCollector collector;
	GlobalEntityClassManager().forEachEntityClass(collector);

	_contents += "\n";
	_contents += collector.getFixupCode();
}
