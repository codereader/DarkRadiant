#pragma once

#include "imodel.h"
#include "ModelExporterBase.h"

namespace model
{

class AseExporter :
	public ModelExporterBase
{
public:
	AseExporter();

	IModelExporterPtr clone() override;

	Format getFileFormat() const override
	{
		return Format::Text;
	}

	const std::string& getDisplayName() const override;

	// Returns the uppercase file extension this exporter is suitable for
	const std::string& getExtension() const override;

	// Export the model file to the given stream
	void exportToStream(std::ostream& stream)  override;
};

}
