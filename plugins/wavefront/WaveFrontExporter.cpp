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
		return;
	}
}

} // namespace exporter
