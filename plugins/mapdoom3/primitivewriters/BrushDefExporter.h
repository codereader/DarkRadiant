#pragma once

#include "ibrush.h"
#include "math/Plane3.h"
#include "math/Matrix4.h"
#include "shaderlib.h"

#include <boost/algorithm/string/predicate.hpp>

namespace map
{

namespace
{
	// Writes a double to the given stream and checks for NaN and infinity
	inline void writeDoubleSafe(const double d, std::ostream& os)
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

class BrushDefExporter
{
public:

	// Writes a Q3-style brushDef definition from the given brush to the given stream
	static void exportBrush(std::ostream& stream, const IBrush& brush)
	{
		// Brush decl header
		stream << "{" << std::endl;
		stream << "brushDef" << std::endl;
		stream << "{" << std::endl;

		// Iterate over each brush face, exporting the tokens from all faces
		for (std::size_t i = 0; i < brush.getNumFaces(); ++i)
		{
			writeFace(stream, brush.getFace(i), brush.getDetailFlag());
		}

		// Close brush contents and header
		stream << "}" << std::endl << "}" << std::endl;
	}

	/* 
	brushDef
	{
	( -64 64 64 ) ( 64 -64 64 ) ( -64 -64 64 ) ( ( 0.015625 0 -0 ) ( -0 0.015625 0 ) ) common/caulk 0 0 0
	( -64 64 64 ) ( 64 64 -64 ) ( 64 64 64 ) ( ( 0.015625 0 0 ) ( 0 0.015625 0 ) ) common/caulk 0 0 0
	( 64 64 64 ) ( 64 -64 -64 ) ( 64 -64 64 ) ( ( 0.015625 0 -0 ) ( -0 0.015625 0 ) ) common/caulk 0 0 0
	( 64 64 -64 ) ( -64 -64 -64 ) ( 64 -64 -64 ) ( ( 0.015625 0 -0 ) ( -0 0.015625 0 ) ) common/caulk 0 0 0
	( 64 -64 -64 ) ( -64 -64 64 ) ( 64 -64 64 ) ( ( 0.015625 0 -0 ) ( -0 0.015625 0 ) ) common/caulk 0 0 0
	( -64 -64 64 ) ( -64 64 -64 ) ( -64 64 64 ) ( ( 0.015625 0 -0 ) ( -0 0.015625 0 ) ) common/caulk 0 0 0
	}
	*/

private:

	static void writeFace(std::ostream& stream, const IFace& face, IBrush::DetailFlag detailFlag)
	{
		// greebo: Don't export faces with degenerate or empty windings (they are "non-contributing")
		const IWinding& winding = face.getWinding();

		if (winding.size() <= 2)
		{
			return;
		}

		// Each face plane is defined by three points

		stream << "( ";
		writeDoubleSafe(winding[2].vertex.x(), stream);
		stream << " ";
		writeDoubleSafe(winding[2].vertex.y(), stream);
		stream << " ";
		writeDoubleSafe(winding[2].vertex.z(), stream);
		stream << " ";
		stream << ") ";

		stream << "( ";
		writeDoubleSafe(winding[0].vertex.x(), stream);
		stream << " ";
		writeDoubleSafe(winding[0].vertex.y(), stream);
		stream << " ";
		writeDoubleSafe(winding[0].vertex.z(), stream);
		stream << " ";
		stream << ") ";

		stream << "( ";
		writeDoubleSafe(winding[1].vertex.x(), stream);
		stream << " ";
		writeDoubleSafe(winding[1].vertex.y(), stream);
		stream << " ";
		writeDoubleSafe(winding[1].vertex.z(), stream);
		stream << " ";
		stream << ") ";

		// Write TexDef
		Matrix4 texdef = face.getTexDefMatrix();
		stream << "( ";

		stream << "( ";
		writeDoubleSafe(texdef.xx(), stream);
		stream << " ";
		writeDoubleSafe(texdef.yx(), stream);
		stream << " ";
		writeDoubleSafe(texdef.tx(), stream);
		stream << " ) ";

		stream << "( ";
		writeDoubleSafe(texdef.xy(), stream);
		stream << " ";
		writeDoubleSafe(texdef.yy(), stream);
		stream << " ";
		writeDoubleSafe(texdef.ty(), stream);
		stream << " ) ";

		stream << ") ";

		// Write Shader (without quotes)
		const std::string& shaderName = face.getShader();

		if (shaderName.empty())
		{
			stream << "_default ";
		}
		else
		{
			if (boost::algorithm::starts_with(shaderName, GlobalTexturePrefix_get()))
			{
				// brushDef has an implicit "textures/" not written to the map, cut it off
				stream << "" << shader_get_textureName(shaderName.c_str()) << " ";
			}
			else
			{
				stream << "" << shaderName << " ";
			}
		}

		// Export (dummy) contents/flags
		stream << detailFlag << " 0 0";
		
		stream << std::endl;
	}
};

}
