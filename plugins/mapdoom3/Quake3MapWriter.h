#pragma once

#include "imapformat.h"

namespace map
{

/**
 * Standard implementation of a Quake 3 Map file writer
 */
class Quake3MapWriter :
	public IMapWriter
{
protected:
	// The counters for numbering the comments
	std::size_t _entityCount;
	std::size_t _primitiveCount;

public:
	Quake3MapWriter() {}

	virtual void beginWriteMap(std::ostream& stream) {}
	virtual void endWriteMap(std::ostream& stream) {}

	// Entity export methods
	virtual void beginWriteEntity(const Entity& entity, std::ostream& stream) {}
	virtual void endWriteEntity(const Entity& entity, std::ostream& stream) {}

	// Brush export methods
	virtual void beginWriteBrush(const IBrush& brush, std::ostream& stream) {}
	virtual void endWriteBrush(const IBrush& brush, std::ostream& stream) {}

	// Patch export methods
	virtual void beginWritePatch(const IPatch& patch, std::ostream& stream) {}
	virtual void endWritePatch(const IPatch& patch, std::ostream& stream) {}
};

} // namespace
