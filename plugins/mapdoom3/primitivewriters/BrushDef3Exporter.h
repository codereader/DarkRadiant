#ifndef BrushDef3Exporter_h__
#define BrushDef3Exporter_h__

#include "ibrush.h"
#include "math/Plane3.h"
#include "math/matrix.h"

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

class BrushDef3Exporter
{
public:

	// Writes a brushDef3 definition from the given brush to the given stream
	static void exportBrush(std::ostream& stream, IBrush& brush)
	{
		if (!brush.hasContributingFaces()) {
			return;
		}

		// Brush decl header
		stream << "{\n";
		stream << "brushDef3\n";
		stream << "{\n";

		// Iterate over each brush face, exporting the tokens from all faces
		for (std::size_t i = 0; i < brush.getNumFaces(); ++i)
		{
			writeFace(stream, brush.getFace(i));
		}

		// Close brush contents and header
		stream << "}\n}\n";
	}

	static void writeFace(std::ostream& stream, const IFace& face)
	{
		// Write the plane equation
		const Plane3& plane = face.getPlane3();

		stream << "( ";
		writeDoubleSafe(plane.normal().x(), stream);
		stream << " ";
		writeDoubleSafe(plane.normal().y(), stream);
		stream << " ";
		writeDoubleSafe(plane.normal().z(), stream);
		stream << " ";
		writeDoubleSafe(-plane.dist(), stream); // negate d
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

		// Write Shader
		const std::string& shaderName = face.getShader();

		if (shaderName.empty()) {
			stream << "\"_default\" ";
		}
		else {
			stream << "\"" << shaderName << "\" ";
		}

		// Export (dummy) contents/flags
		stream << "0 0 0\n";
	}
};

}

#endif // BrushDef3Exporter_h__
