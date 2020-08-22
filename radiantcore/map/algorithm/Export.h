#pragma once

#include <string>
#include "icommandsystem.h"

namespace map
{

namespace algorithm
{

struct ModelExportOptions
{
	std::string outputFilename;		// full export path
	std::string outputFormat;		// model exporter extension
	bool skipCaulk;					// whether to skip caulk
	bool centerObjects;				// whether to center objects
	bool replaceSelectionWithModel;	// delete the selection and put the exported model in its place
	bool useEntityOrigin;			// use entity origin as model origin (only applicable if a single entity is selected)
	bool exportLightsAsObjects;		// will export lights as small octahedrons
};

/**
 * Exports the selection as model using the given options.
 * Will throw a std::runtime_error on failure.
 */
void exportSelectedAsModel(const ModelExportOptions& options);

/**
 * Command target taking a lot of arguments and sorting them into a
 * ModelExportOptions structure to pass it on to the above overload.
 */
void exportSelectedAsModelCmd(const cmd::ArgumentList& args);

}

}
