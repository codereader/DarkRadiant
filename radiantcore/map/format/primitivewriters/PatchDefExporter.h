#pragma once

#include "shaderlib.h"
#include "ipatch.h"

#include "string/predicate.h"
#include "ExportUtil.h"

namespace map
{

class PatchDefExporter
{
public:

	// Writes a patchDef2/3 definition from the given patch to the given stream
	static void exportPatch(std::ostream& stream, const IPatchNodePtr& patchNode)
	{
		const IPatch& patch = patchNode->getPatch();

		if (patch.subdivisionsFixed())
		{
			exportPatchDef3(stream, patch);
		}
		else
		{
			exportPatchDef2(stream, patch);
		}
	}

	// Export a patchDef2 declaration, Q3-style
	static void exportQ3PatchDef2(std::ostream& stream, const IPatchNodePtr& patchNode)
	{
		const IPatch& patch = patchNode->getPatch();

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

		assert(patch.subdivisionsFixed());

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
			if (string::starts_with(shaderName, GlobalTexturePrefix_get()))
			{
				// Q3-style patchDef2 doesn't write the "textures/" prefix to the map, cut it off
				stream << shader_get_textureName(shaderName.c_str());
			}
			else
			{
				stream << shaderName;
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
				writeDoubleSafe(patch.ctrlAt(r,c).vertex[0], stream);
				stream << " ";
                writeDoubleSafe(patch.ctrlAt(r,c).vertex[1], stream);
				stream << " ";
                writeDoubleSafe(patch.ctrlAt(r,c).vertex[2], stream);
				stream << " ";
                writeDoubleSafe(patch.ctrlAt(r,c).texcoord[0], stream);
				stream << " ";
                writeDoubleSafe(patch.ctrlAt(r,c).texcoord[1], stream);
				stream << " ) ";
			}

			stream << ")\n";
		}

		stream << ")\n";
	}
};

}
