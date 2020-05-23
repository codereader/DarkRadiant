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

	virtual void beginWriteMap(const scene::IMapRootNodePtr& root, std::ostream& stream) override;
	virtual void endWriteMap(const scene::IMapRootNodePtr& root, std::ostream& stream) override;

	// Entity export methods
	virtual void beginWriteEntity(const IEntityNodePtr& entity, std::ostream& stream) override;
	virtual void endWriteEntity(const IEntityNodePtr& entity, std::ostream& stream) override;

	// Brush export methods
	virtual void beginWriteBrush(const IBrushNodePtr& brush, std::ostream& stream) override;
	virtual void endWriteBrush(const IBrushNodePtr& brush, std::ostream& stream) override;

	// Patch export methods
	virtual void beginWritePatch(const IPatchNodePtr& patch, std::ostream& stream) override;
	virtual void endWritePatch(const IPatchNodePtr& patch, std::ostream& stream) override;

protected:
	void writeEntityKeyValues(const IEntityNodePtr& entity, std::ostream& stream);
};

} // namespace
