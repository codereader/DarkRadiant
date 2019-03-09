#pragma once

#include "Doom3MapWriter.h"
#include "primitivewriters/BrushDef3Exporter.h"
#include "Quake4MapFormat.h"

namespace map
{

// A Q4 map writer is working nearly the same as for D3, with
// the exception of brushDef3 declarations having no contents flags
// and the map file carrying a different version number
class Quake4MapWriter :
	public Doom3MapWriter
{
public:
	virtual void beginWriteMap(std::ostream& stream)
	{
		// Write the version tag
		stream << "Version " << MAP_VERSION_Q4 << std::endl;
	}

	virtual void beginWriteBrush(const IBrush& brush, std::ostream& stream)
	{
		// Primitive count comment
		stream << "// primitive " << _primitiveCount++ << std::endl;

		// Export brushDef3 definition to stream, but without contents flags
		BrushDef3Exporter::exportBrush(stream, brush, false);
	}
};

} // namespace
