#include "ModelFormatManager.h"

#include "itextstream.h"
#include <boost/algorithm/string/case_conv.hpp>

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

}
