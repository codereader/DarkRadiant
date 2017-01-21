#pragma once

#include <map>
#include "imodel.h"

namespace model
{

class ModelFormatManager :
	public IModelFormatManager
{
private:
	// Map file extension to implementation
	typedef std::map<std::string, IModelExporterPtr> ExporterMap;
	ExporterMap _exporters;

public:
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;

	void registerExporter(const IModelExporterPtr& exporter) override;
	void unregisterExporter(const IModelExporterPtr& exporter) override;
};

}
