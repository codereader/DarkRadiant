#pragma once

namespace map
{

namespace algorithm
{

struct ModelExportOptions
{
	std::string outputFilename; // full export path
	std::string outputFormat;	// model exporter extension
	bool skipCaulk;				// whether to skip caulk
	bool centerObjects;			// whether to center objects
};

/**
 * Exports the selection as model using the given options
 */
void exportSelectedAsModel(const ModelExportOptions& options);

/**
 * Command target taking a lot of arguments and sorting them into a
 * ModelExportOptions structure to pass it on to the above overload.
 */
void exportSelectedAsModelCmd(const cmd::ArgumentList& args);

}

}