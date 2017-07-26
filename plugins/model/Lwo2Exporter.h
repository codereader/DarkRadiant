#pragma once


#include "imodel.h"
#include "render.h"

namespace model
{

class Lwo2Exporter :
	public IModelExporter
{
private:
	struct Surface
	{
		std::string materialName;

		// The vertices of this surface
		std::vector<ArbitraryMeshVertex> vertices;

		// The indices connecting the vertices to triangles
		IndexBuffer indices;
	};

	std::vector<Surface> _surfaces;

public:
	Lwo2Exporter();

	IModelExporterPtr clone() override;

	Format getFileFormat() const override
	{
		return Format::Binary;
	}

	// Returns the uppercase file extension this exporter is suitable for
	const std::string& getExtension() const override;

	// Adds the given Surface to the exporter's queue
	void addSurface(const IModelSurface& surface) override;

	// Export the model file to the given stream
	void exportToStream(std::ostream& stream)  override;
};

}

