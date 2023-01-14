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
void bindEntities();

/**
 * greebo: Sets up the target spawnarg of the selected entities such that
 * the first selected entity is targetting the second.
 */
void connectSelectedEntities();

/**
 * greebo: (De-)selects all entities that reference the given model path
 * in their "model" spawnarg.
 */
void selectItemsByModel(const std::string& model);
void deselectItemsByModel(const std::string& model);

// Command target to (de-)select items by model
void selectItemsByModelCmd(const cmd::ArgumentList& args);
void deselectItemsByModelCmd(const cmd::ArgumentList& args);

// Command target: PlacePlayerStart <origin>
void placePlayerStart(const cmd::ArgumentList& args);

} // namespace algorithm
} // namespace selection
