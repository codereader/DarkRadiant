#include "WaveFrontExporter.h"

#include "string/string.h"

namespace exporter
{

WaveFrontExporter::WaveFrontExporter(const std::string& outputPath) :
	_output(outputPath.c_str()),
	_exportedBrushes(0),
	_exportedPatches(0),
	_vertexCount(0)
{}

void WaveFrontExporter::exportSelection()
{
	GlobalSelectionSystem().foreachSelected(*this);
}

void WaveFrontExporter::exportBrush(IBrush& brush)
{
	_output << "\ng " << "Brush" << _exportedBrushes << "\n";

	std::string vertexBuf;
	std::string texCoordBuf;
	std::string faceBuf;

	for (std::size_t faceIdx = 0; faceIdx < brush.getNumFaces(); ++faceIdx)
	{
		IFace& face = brush.getFace(faceIdx);

		// Remember the index of the first vertex
		std::size_t firstVertex = _vertexCount;

		const IWinding& winding = face.getWinding();

		for (std::size_t i = 0; i < winding.size(); ++i)
		{
			// Write coordinates into the export buffers
			vertexBuf += "v " + std::string(winding[i].vertex) + "\n";
			texCoordBuf += "vt " + std::string(winding[i].texcoord) + "\n";

			// Count the exported vertices
			++_vertexCount;
		}

		// Construct the face section
		faceBuf += "\nf";

		for (std::size_t i = firstVertex; i < _vertexCount; ++i)
		{
			faceBuf += " " + sizetToStr(i+1) + "/" + sizetToStr(i+1);
		}
	}

    _output << vertexBuf << "\n";
    _output << texCoordBuf;
    _output << faceBuf << "\n";

    ++_exportedBrushes;
}

void WaveFrontExporter::exportPatch(IPatch& patch)
{
	_output << "\ng " << "Patch" << _exportedPatches << "\n";

	std::string vertexBuf;
	std::string texCoordBuf;
	std::string faceBuf;

	// Get hold of the fully tesselated patch mesh, not just the control vertices
	PatchMesh mesh = patch.getTesselatedPatchMesh();

	// Remember the vertex count offset
	std::size_t firstVertex = _vertexCount;

	for (std::size_t h = 0; h < mesh.height; ++h)
	{
		for (std::size_t w = 0; w < mesh.width; ++w)
		{
			const PatchMesh::Vertex& v = mesh.vertices[mesh.width*h + w];

			// Write coordinates into the export buffers
			vertexBuf += "v " + std::string(v.vertex) + "\n";
			texCoordBuf += "vt " + std::string(v.texcoord) + "\n";

			// Count the exported vertices
			++_vertexCount;

			// Starting from the second row, we're gathering the faces
			if (h > 0 && w > 0)
			{
				// Gather the indices forming a quad
				std::size_t v1 = 1 + firstVertex + h*mesh.width + w;
				std::size_t v2 = 1 + firstVertex + (h-1)*mesh.width + w;
				std::size_t v3 = 1 + firstVertex + (h-1)*mesh.width + (w-1);
				std::size_t v4 = 1 + firstVertex + h*mesh.width + (w-1);

				// Construct the quad
				faceBuf += "f";
				faceBuf += " " + sizetToStr(v1) + "/" + sizetToStr(v1);
				faceBuf += " " + sizetToStr(v4) + "/" + sizetToStr(v4);
				faceBuf += " " + sizetToStr(v3) + "/" + sizetToStr(v3);
				faceBuf += " " + sizetToStr(v2) + "/" + sizetToStr(v2);
				faceBuf += "\n";
			}
		}
	}

	_output << vertexBuf << "\n";
    _output << texCoordBuf << "\n";
    _output << faceBuf << "\n";

    ++_exportedPatches;
}

void WaveFrontExporter::visit(const scene::INodePtr& node) const
{
	IBrushNodePtr brushNode = boost::dynamic_pointer_cast<IBrushNode>(node);

	if (brushNode != NULL)
	{
		// SelectionSystem Visitor is const, what a pain
		const_cast<WaveFrontExporter&>(*this).exportBrush(brushNode->getIBrush());
		return;
	}

	IPatchNodePtr patchNode = boost::dynamic_pointer_cast<IPatchNode>(node);

	if (patchNode != NULL)
	{
		const_cast<WaveFrontExporter&>(*this).exportPatch(patchNode->getPatch());
		return;
	}
}

} // namespace exporter
