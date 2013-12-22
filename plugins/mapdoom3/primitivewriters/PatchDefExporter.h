#pragma once

#include "shaderlib.h"
#include "ipatch.h"

#include <boost/algorithm/string/predicate.hpp>

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
		if (patch.subdivionsFixed())
		{
			exportPatchDef3(stream, patch);
		}
		else
		{
			exportPatchDef2(stream, patch);
		}
	}

	// Export a patchDef2 declaration, Q3-style
	static void exportQ3PatchDef2(std::ostream& stream, const IPatch& patch)
	{
		// Export patch declaration
		stream << "{\n";
		stream << "patchDef2\n";
		stream << "{\n";

		exportQ3Shader(stream, patch);

		// Export patch dimension / parameters
		stream << "( ";
		stream << patch.getWidth() << " ";
		stream << patch.getHeight() << " ";

		// empty contents/flags
		stream << "0 0 0 )\n";

		exportPatchControlMatrix(stream, patch);

		stream << "}\n}\n";
	}

private:
	// Export a patchDef3 declaration (fixed subdivisions)
	static void exportPatchDef3(std::ostream& stream, const IPatch& patch)
	{
		// Export patch declaration
		stream << "{\n";
		stream << "patchDef3\n";
		stream << "{\n";

		exportShader(stream, patch);

		// Export patch dimension / parameters
		stream << "( ";
		stream << patch.getWidth() << " ";
		stream << patch.getHeight() << " ";

		assert(patch.subdivionsFixed());

		Subdivisions divisions = patch.getSubdivisions();
		stream << divisions.x() << " ";
		stream << divisions.y() << " ";

		// empty contents/flags
		stream << "0 0 0 )\n";

		exportPatchControlMatrix(stream, patch);

		stream << "}\n}\n";
	}

	// Export a patchDef2 declaration, D3-style
	static void exportPatchDef2(std::ostream& stream, const IPatch& patch)
	{
		// Export patch declaration
		stream << "{\n";
		stream << "patchDef2\n";
		stream << "{\n";

		exportShader(stream, patch);

		// Export patch dimension / parameters
		stream << "( ";
		stream << patch.getWidth() << " ";
		stream << patch.getHeight() << " ";

		// empty contents/flags
		stream << "0 0 0 )\n";

		exportPatchControlMatrix(stream, patch);

		stream << "}\n}\n";
	}

	static void exportShader(std::ostream& stream, const IPatch& patch)
	{
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
	}

	// Q3 shader declarations are missing their textures/ prefix and don't use quotes
	static void exportQ3Shader(std::ostream& stream, const IPatch& patch)
	{
		// Export shader
		const std::string& shaderName = patch.getShader();

		if (shaderName.empty())
		{
			stream << "_default";
		}
		else
		{
			if (boost::algorithm::starts_with(shaderName, GlobalTexturePrefix_get()))
			{
				// Q3-style patchDef2 has the "textures/" not written to the map, cut it off
				stream << "" << shader_get_textureName(shaderName.c_str()) << " ";
			}
			else
			{
				stream << "" << shaderName << " ";
			}
		}
		stream << "\n";
	}

	static void exportPatchControlMatrix(std::ostream& stream, const IPatch& patch)
	{
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
	}
};

}
