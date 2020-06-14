#pragma once

#include "icommandsystem.h"
#include "inode.h"

#include <string>

namespace selection
{

namespace algorithm
{

/**
 * Applies the key/value combination to the currently selected entities.
 * It's safe to set a "classname" key through this method.
 * This doesn't open an UndoableCommand session, take care of this in the 
 * client code.
 *
 * Throws a cmd::ExecutionFailure in case the keyvalue cannot be applied.
 */
void setEntityKeyValue(const std::string& key, const std::string& value);

// Command adaptor wrapping the setEntityKeyvalue function above
void setEntityKeyValueOnSelection(const cmd::ArgumentList& args);

/**
 * greebo: "Binds" the selected entites together by setting the "bind"
 * spawnarg on both entities. Two entities must be highlighted for this
 * command to function correctly.
 */
void bindEntities(const cmd::ArgumentList& args);

/**
 * greebo: Sets up the target spawnarg of the selected entities such that
 * the first selected entity is targetting the second.
 */
void connectSelectedEntities(const cmd::ArgumentList& args);

} // namespace algorithm
} // namespace selection
