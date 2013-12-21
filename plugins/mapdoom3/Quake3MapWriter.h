#pragma once

#include "Doom3MapWriter.h"
#include "primitivewriters/BrushDefExporter.h"
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
	virtual void beginWriteMap(std::ostream& stream)
	{
		// Write an empty line at the beginning of the file
		stream << std::endl;
	}

	virtual void beginWriteBrush(const IBrush& brush, std::ostream& stream)
	{
		// Primitive count comment
		stream << "// brush " << _primitiveCount++ << std::endl;

		// Export brushDef definition to stream, including contents flags
		BrushDefExporter::exportBrush(stream, brush, true);
	}
};

} // namespace
