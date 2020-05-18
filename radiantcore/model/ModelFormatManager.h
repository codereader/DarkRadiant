#pragma once

#include <map>
#include "imodel.h"
#include "NullModelLoader.h"

namespace model
{

class ModelFormatManager :
	public IModelFormatManager
{
private:
	// Map file extension to implementation
	typedef std::map<std::string, IModelExporterPtr> ExporterMap;
	ExporterMap _exporters;

	// Map file extension to implementation
	typedef std::map<std::string, IModelImporterPtr> ImporterMap;
	ImporterMap _importers;

	NullModelLoaderPtr _nullModelLoader;

public:
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;

	void registerImporter(const IModelImporterPtr& importer) override;
	void unregisterImporter(const IModelImporterPtr& importer) override;

	IModelImporterPtr getImporter(const std::string& extension) override;

	void registerExporter(const IModelExporterPtr& exporter) override;
	void unregisterExporter(const IModelExporterPtr& exporter) override;

	IModelExporterPtr getExporter(const std::string& extension) override;

	void foreachImporter(const std::function<void(const IModelImporterPtr&)>& functor) override;
	void foreachExporter(const std::function<void(const IModelExporterPtr&)>& functor) override;

private:
	void postModuleInitialisation();
};

}
