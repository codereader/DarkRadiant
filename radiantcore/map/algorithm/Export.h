#pragma once

#include "ModelExportOptions.h"
#include "icommandsystem.h"

namespace map
{

namespace algorithm
{

/**
 * Exports the selection as model using the given options.
 * Will throw a std::runtime_error on failure.
 */
void exportSelectedAsModel(const model::ModelExportOptions& options);

/**
 * Command target taking a lot of arguments and sorting them into a
 * ModelExportOptions structure to pass it on to the above overload.
 */
void exportSelectedAsModelCmd(const cmd::ArgumentList& args);

}

}
