#pragma once

#include "icommandsystem.h"
#include "inode.h"

#include <string>

namespace selection
{

namespace algorithm
{

/**
 * greebo: Changes the classname of the currently selected entities.
 */
void setEntityClassname(const std::string& classname);

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


/** 
 * Exception thrown when the incorrect number of brushes is selected when
 * creating an entity.
 */
class EntityCreationException : public std::runtime_error
{
public:
	EntityCreationException(const std::string& what)
	: std::runtime_error(what) {}
};

/**
 * Create an instance of the given entity at the given position, and return
 * the Node containing the new entity. If the incorrect number of brushes
 * is selected, an EntityCreationException will be thrown.
 *
 * @returns
 * A scene::INodePtr containing the new entity.
 */
scene::INodePtr createEntityFromSelection(const std::string& name, const Vector3& origin);

} // namespace algorithm
} // namespace selection
