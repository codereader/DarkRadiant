#include "ModelFormatManager.h"

#include "i18n.h"
#include "itextstream.h"
#include "ifiletypes.h"
#include "ipreferencesystem.h"
#include "string/case_conv.h"

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

	module::ModuleRegistry::Instance().signal_allModulesInitialised().connect(
		sigc::mem_fun(this, &ModelFormatManager::postModuleInitialisation)
	);
}

void ModelFormatManager::postModuleInitialisation()
{
	if (!_exporters.empty())
	{
		// Construct and Register the patch-related preferences
		IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Model Export"));

		ComboBoxValueList choices;

		for (const ExporterMap::value_type& pair : _exporters)
		{
			choices.push_back(pair.first);
		}

		page.appendCombo(_("Export Format for scaled Models"), RKEY_DEFAULT_MODEL_EXPORT_FORMAT, choices, true);

		// Register all exporter extensions to the FileTypeRegistry

		for (const ExporterMap::value_type& pair : _exporters)
		{
			std::string extLower = string::to_lower_copy(pair.second->getExtension());

			GlobalFiletypes().registerPattern(filetype::TYPE_MODEL_EXPORT, FileTypePattern(
				pair.second->getDisplayName(), 
				extLower,
				"*." + extLower));
		}
	}
}

void ModelFormatManager::registerImporter(const IModelImporterPtr& importer)
{
	assert(importer);

	std::string extension = string::to_upper_copy(importer->getExtension());

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

	std::string extension = string::to_upper_copy(importer->getExtension());

	if (_importers.find(extension) != _importers.end())
	{
		_importers.erase(extension);
		return;
	}

	rWarning() << "Cannot unregister importer for extension " << extension << std::endl;
}

IModelImporterPtr ModelFormatManager::getImporter(const std::string& extension)
{
	std::string extensionUpper = string::to_upper_copy(extension);

	ImporterMap::const_iterator found = _importers.find(extensionUpper);

	return found != _importers.end() ? found->second : _nullModelLoader;
}

void ModelFormatManager::registerExporter(const IModelExporterPtr& exporter)
{
	assert(exporter);

	std::string extension = string::to_upper_copy(exporter->getExtension());

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

	std::string extension = string::to_upper_copy(exporter->getExtension());

	if (_exporters.find(extension) != _exporters.end())
	{
		_exporters.erase(extension);
		return;
	}

	rWarning() << "Cannot unregister exporter for extension " << extension << std::endl;
}

IModelExporterPtr ModelFormatManager::getExporter(const std::string& extension)
{
	std::string extensionUpper = string::to_upper_copy(extension);

	ExporterMap::const_iterator found = _exporters.find(extensionUpper);

	// Return a cloned instance if we found a matching exporter
	return found != _exporters.end() ? found->second->clone() : IModelExporterPtr();
}

void ModelFormatManager::foreachImporter(const std::function<void(const IModelImporterPtr&)>& functor)
{
	for (const ImporterMap::value_type& pair : _importers)
	{
		functor(pair.second);
	}
}

void ModelFormatManager::foreachExporter(const std::function<void(const IModelExporterPtr&)>& functor)
{
	for (const ExporterMap::value_type& pair : _exporters)
	{
		functor(pair.second);
	}
}

module::StaticModule<ModelFormatManager> _staticModelFormatManagerModule;

}
