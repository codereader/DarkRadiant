#pragma once

#include "ibrush.h"
#include "math/Plane3.h"
#include "math/Matrix4.h"
#include "shaderlib.h"

#include "string/predicate.h"
#include "ExportUtil.h"
#include "../Quake3Utils.h"

namespace map
{

class LegacyBrushDefExporter
{
public:

	// Writes an old Q3-style brush definition from the given brush to the given stream
	static void exportBrush(std::ostream& stream, const IBrushNodePtr& brushNode)
	{
		const IBrush& brush = brushNode->getIBrush();

		// Curly braces surround the brush contents
		stream << "{" << std::endl;

		// Iterate over each brush face, exporting the tokens from all faces
		for (std::size_t i = 0; i < brush.getNumFaces(); ++i)
		{
			writeFace(stream, brush.getFace(i), brush.getDetailFlag());
		}

		// Close brush contents
		stream << "}" << std::endl;
	}

    /*
    {
    ( 16 192 96 ) ( 16 128 96 ) ( 0 192 96 ) shared_vega/trim03a 0 0 0 0.125 0.125 134217728 0 0
    ( 0 208 96 ) ( 0 208 80 ) ( 16 192 96 ) common/caulk 0 0 0 0.5 0.5 134217728 0 0
    ( 0 208 80 ) ( 0 208 96 ) ( 0 112 96 ) shared_vega/trim03a 64 0 90 0.125 0.125 134217728 0 0
    ( 0 112 80 ) ( 16 128 80 ) ( 0 192 80 ) common/caulk 0 0 0 0.5 0.5 134217728 0 0
    ( 16 128 96 ) ( 16 128 80 ) ( 0 112 96 ) shared_vega/trim03a 64 0 90 0.125 0.125 134217728 0 0
    ( 16 128 80 ) ( 16 128 96 ) ( 16 192 80 ) common/caulk 0 0 0 0.5 0.5 134217728 0 0
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

        // ( 16 192 96 ) ( 16 128 96 ) ( 0 192 96 ) shared_vega/trim03a 0 0 0 0.125 0.125 134217728 0 0
        // ( Point 1 ) ( Point 2 ) ( Point 3 ) path/to/material shiftS shiftT rotate scaleS scaleT DetailFlag 0 0

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

        // Write Shader (without quotes)
        const std::string& shaderName = face.getShader();

        if (shaderName.empty())
        {
            stream << "_default ";
        }
        else
        {
            if (string::starts_with(shaderName, GlobalTexturePrefix_get()))
            {
                // Q3 has an implicit "textures/" not written to the map, cut it off
                stream << shader_get_textureName(shaderName.c_str()) << " ";
            }
            else
            {
                stream << shaderName << " ";
            }
        }

        auto transform = getMatrix4FromTextureMatrix(face.getProjectionMatrix());

        // Bake the ComputeAxisBase calculations done in idTech4 into the texdef matrix
        // before converting it back using the GtkRadiant algorithms
        auto axisBase = getBasisTransformForNormal(face.getPlane3().normal());
        transform.multiplyBy(axisBase);

        float imageWidth = 128;
        float imageHeight = 128;

        auto texture = GlobalMaterialManager().getMaterial(face.getShader())->getEditorImage();

        if (texture)
        {
            imageWidth = static_cast<float>(texture->getWidth());
            imageHeight = static_cast<float>(texture->getHeight());
        }

        if (imageWidth == 0 || imageHeight == 0)
        {
            rError() << "LegacyBrushDefParser: Failed to load image: " << face.getShader() << std::endl;
        }

        // Convert the matrix back to Q3 texdef
        auto shiftScaleRotate = quake3::calculateTexDefFromTransform(face, transform, imageWidth, imageHeight);

        // Write Texture Shift/Scale/Rotation
        writeDoubleSafe(shiftScaleRotate.shift[0], stream);
        stream << " ";
        writeDoubleSafe(shiftScaleRotate.shift[1], stream);
        stream << " ";
        writeDoubleSafe(shiftScaleRotate.rotate, stream);
        stream << " ";
        writeDoubleSafe(shiftScaleRotate.scale[0], stream);
        stream << " ";
        writeDoubleSafe(shiftScaleRotate.scale[1], stream);
        stream << " ";

		// Export contents flags and the two zeroes at the end
		stream << detailFlag << " 0 0";
		
		stream << std::endl;
	}
};

}
