#include "ModelFormatManager.h"

#include "itextstream.h"
#include <boost/algorithm/string/case_conv.hpp>

#include "modulesystem/StaticModule.h"

namespace model
{

const std::string& ModelFormatManager::getName() const
{
	static std::string _name(MODULE_MODELFORMATMANAGER);
	return _name;
}

const StringSet& ModelFormatManager::getDependencies() const
{
	static StringSet _dependencies;
	return _dependencies;
}

void ModelFormatManager::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	_nullModelLoader.reset(new NullModelLoader);
}

void ModelFormatManager::registerImporter(const IModelImporterPtr& importer)
{
	assert(importer);

	std::string extension = boost::algorithm::to_upper_copy(importer->getExtension());

	if (_importers.find(extension) != _importers.end())
	{
		rWarning() << "Cannot register more than one model importer for extension " << extension << std::endl;
		return;
	}

	_importers[extension] = importer;
}

void ModelFormatManager::unregisterImporter(const IModelImporterPtr& importer)
{
	assert(importer);

	std::string extension = boost::algorithm::to_upper_copy(importer->getExtension());

	if (_importers.find(extension) != _importers.end())
	{
		_importers.erase(extension);
		return;
	}

	rWarning() << "Cannot unregister importer for extension " << extension << std::endl;
}

IModelImporterPtr ModelFormatManager::getImporter(const std::string& extension)
{
	std::string extensionUpper = boost::algorithm::to_upper_copy(extension);

	ImporterMap::const_iterator found = _importers.find(extensionUpper);

	return found != _importers.end() ? found->second : _nullModelLoader;
}

void ModelFormatManager::registerExporter(const IModelExporterPtr& exporter)
{
	assert(exporter);

	std::string extension = boost::algorithm::to_upper_copy(exporter->getExtension());

	if (_exporters.find(extension) != _exporters.end())
	{
		rWarning() << "Cannot register more than one model exporter for extension " << extension << std::endl;
		return;
	}

	_exporters[extension] = exporter;
}

void ModelFormatManager::unregisterExporter(const IModelExporterPtr& exporter)
{
	assert(exporter);

	std::string extension = boost::algorithm::to_upper_copy(exporter->getExtension());

	if (_exporters.find(extension) != _exporters.end())
	{
		_exporters.erase(extension);
		return;
	}

	rWarning() << "Cannot unregister exporter for extension " << extension << std::endl;
}

IModelExporterPtr ModelFormatManager::getExporter(const std::string& extension)
{
	std::string extensionUpper = boost::algorithm::to_upper_copy(extension);

	ExporterMap::const_iterator found = _exporters.find(extensionUpper);

	return found != _exporters.end() ? found->second : IModelExporterPtr();
}

module::StaticModule<ModelFormatManager> _staticModelFormatManagerModule;

}
