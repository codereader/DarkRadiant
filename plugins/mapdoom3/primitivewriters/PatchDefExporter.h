#ifndef PatchDefExporter_h__
#define PatchDefExporter_h__

#include "ipatch.h"

namespace map
{

namespace
{
	inline void writePatchDouble(const double d, std::ostream& os)
	{
		if (isValid(d))
		{
			if (d == -0.0)
			{
				os << 0; // convert -0 to 0
			}
			else
			{
				os << d;
			}
		}
		else
		{
			// Is infinity or NaN, write 0
			os << "0";
		}
	}
}

class PatchDefExporter
{
public:

	// Writes a patchDef2/3 definition from the given patch to the given stream
	static void exportPatch(std::ostream& stream, const IPatch& patch)
	{
		// Export patch declaration
		stream << "{\n";
		stream << (patch.subdivionsFixed() ? "patchDef3\n" : "patchDef2\n");
		stream << "{\n";

		// Export shader
		const std::string& shaderName = patch.getShader();

		if (shaderName.empty())
		{
			stream << "\"_default\"";
		}
		else
		{
			stream << "\"" << shaderName << "\"";
		}
		stream << "\n";

		// Export patch dimension / parameters
		stream << "( ";
		stream << patch.getWidth() << " ";
		stream << patch.getHeight() << " ";

		if (patch.subdivionsFixed())
		{
			Subdivisions divisions = patch.getSubdivisions();
			stream << divisions.x() << " ";
			stream << divisions.y() << " ";
		}

		// empty contents/flags
		stream << "0 0 0 )\n";

		// Export the control point matrix
		stream << "(\n";

		for (std::size_t c = 0; c < patch.getWidth(); c++)
		{
			stream << "( ";

			for (std::size_t r = 0; r < patch.getHeight(); r++)
			{
				stream << "( ";
				writePatchDouble(patch.ctrlAt(r,c).vertex[0], stream);
				stream << " ";
				writePatchDouble(patch.ctrlAt(r,c).vertex[1], stream);
				stream << " ";
				writePatchDouble(patch.ctrlAt(r,c).vertex[2], stream);
				stream << " ";
				writePatchDouble(patch.ctrlAt(r,c).texcoord[0], stream);
				stream << " ";
				writePatchDouble(patch.ctrlAt(r,c).texcoord[1], stream);
				stream << " ) ";
			}

			stream << ")\n";
		}

		stream << ")\n";

		stream << "}\n}\n";
	}
};

}

#endif // PatchDefExporter_h__
