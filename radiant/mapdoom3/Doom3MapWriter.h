#pragma once

#include "imapformat.h"

namespace map
{

/**
 * Standard implementation of a Doom 3 Map file writer (Map Version 2)
 *
 * Creates a plaintext file with brushDef3/patchDef2/patchDef3 primitives.
 */
class Doom3MapWriter :
	public IMapWriter
{
protected:
	// The counters for numbering the comments
	std::size_t _entityCount;
	std::size_t _primitiveCount;

public:
	Doom3MapWriter();

	virtual void beginWriteMap(std::ostream& stream);
	virtual void endWriteMap(std::ostream& stream);

	// Entity export methods
	virtual void beginWriteEntity(const Entity& entity, std::ostream& stream);
	virtual void endWriteEntity(const Entity& entity, std::ostream& stream);

	// Brush export methods
	virtual void beginWriteBrush(const IBrush& brush, std::ostream& stream);
	virtual void endWriteBrush(const IBrush& brush, std::ostream& stream);

	// Patch export methods
	virtual void beginWritePatch(const IPatch& patch, std::ostream& stream);
	virtual void endWritePatch(const IPatch& patch, std::ostream& stream);

protected:
	void writeEntityKeyValues(const Entity& entity, std::ostream& stream);
};

} // namespace
