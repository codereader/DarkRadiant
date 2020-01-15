#pragma once

#include "Doom3MapWriter.h"
#include "mapdoom3/primitivewriters/BrushDefExporter.h"
#include "mapdoom3/primitivewriters/PatchDefExporter.h"
#include "Quake3MapFormat.h"

namespace map
{

// A Q3 map writer is working nearly the same as for D3, with
// brushDef primitives instead of brushDef3 and
// patchDef2 only. No version string is written at the top of the file
class Quake3MapWriter :
	public Doom3MapWriter
{
public:
	virtual void beginWriteMap(std::ostream& stream) override
	{
		// Write an empty line at the beginning of the file
		stream << std::endl;
	}

	virtual void beginWriteBrush(const IBrushNodePtr& brush, std::ostream& stream) override
	{
		// Primitive count comment
		stream << "// brush " << _primitiveCount++ << std::endl;

		// Export brushDef definition to stream
		BrushDefExporter::exportBrush(stream, brush);
	}

	virtual void beginWritePatch(const IPatchNodePtr& patch, std::ostream& stream) override
	{
		// Primitive count comment, not a typo, patches also seem to have "brush" in their comments
		stream << "// brush " << _primitiveCount++ << std::endl;

		// Export patchDef2 to stream (patchDef3 is not supported)
		PatchDefExporter::exportQ3PatchDef2(stream, patch);
	}
};

} // namespace
