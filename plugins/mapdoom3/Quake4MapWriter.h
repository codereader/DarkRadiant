#pragma once

#include "Doom3MapWriter.h"
#include "primitivewriters/BrushDef3Exporter.h"

namespace map
{

// A Q4 map writer is working nearly the same as for D3, with
// the exception of brushDef3 declarations having no contents flags.
class Quake4MapWriter :
	public Doom3MapWriter
{
public:
	virtual void beginWriteBrush(const IBrush& brush, std::ostream& stream)
	{
		// Primitive count comment
		stream << "// primitive " << _primitiveCount++ << std::endl;

		// Export brushDef3 definition to stream, but without contents flags
		BrushDef3Exporter::exportBrush(stream, brush, false);
	}
};

} // namespace
