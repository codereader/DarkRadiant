#include "ModelFormatManager.h"

#include "imodule.h"
#include "i18n.h"
#include "itextstream.h"
#include "ifiletypes.h"
#include "ipreferencesystem.h"
#include "string/case_conv.h"

#include "module/StaticModule.h"

#include "import/FbxModelLoader.h"
#include "export/AseExporter.h"
#include "export/Lwo2Exporter.h"
#include "export/WavefrontExporter.h"
#include "command/ExecutionFailure.h"
#include "os/fs.h"

namespace model
{

const std::string& ModelFormatManager::getName() const
{
	static std::string _name(MODULE_MODELFORMATMANAGER);
	return _name;
}

const StringSet& ModelFormatManager::getDependencies() const
{
    static StringSet _dependencies { MODULE_COMMANDSYSTEM };
	return _dependencies;
}

void ModelFormatManager::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	_nullModelLoader.reset(new NullModelLoader);

	module::GlobalModuleRegistry().signal_allModulesInitialised().connect(
		sigc::mem_fun(this, &ModelFormatManager::postModuleInitialisation)
	);

    // Register the built-in model importers
    registerImporter(std::make_shared<FbxModelLoader>());

	// Register the built-in model exporters
	registerExporter(std::make_shared<AseExporter>());
	registerExporter(std::make_shared<Lwo2Exporter>());
	registerExporter(std::make_shared<WavefrontExporter>());

    GlobalCommandSystem().addCommand("ConvertModel", std::bind(&ModelFormatManager::convertModelCmd, this, std::placeholders::_1),
        { cmd::ARGTYPE_STRING, cmd::ARGTYPE_STRING, cmd::ARGTYPE_STRING });
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

	auto extension = string::to_upper_copy(importer->getExtension());

	if (_importers.count(extension) > 0)
	{
		rWarning() << "Cannot register more than one model importer for extension " << extension << std::endl;
		return;
	}

	_importers[extension] = importer;
}

void ModelFormatManager::unregisterImporter(const IModelImporterPtr& importer)
{
	assert(importer);

	auto extension = string::to_upper_copy(importer->getExtension());

	if (_importers.count(extension) > 0)
	{
		_importers.erase(extension);
		return;
	}

	rWarning() << "Cannot unregister importer for extension " << extension << std::endl;
}

IModelImporterPtr ModelFormatManager::getImporter(const std::string& extension)
{
	auto extensionUpper = string::to_upper_copy(extension);

	auto found = _importers.find(extensionUpper);

	return found != _importers.end() ? found->second : _nullModelLoader;
}

void ModelFormatManager::registerExporter(const IModelExporterPtr& exporter)
{
	assert(exporter);

	auto extension = string::to_upper_copy(exporter->getExtension());

	if (_exporters.count(extension) > 0)
	{
		rWarning() << "Cannot register more than one model exporter for extension " << extension << std::endl;
		return;
	}

	_exporters[extension] = exporter;
}

void ModelFormatManager::unregisterExporter(const IModelExporterPtr& exporter)
{
	assert(exporter);

	auto extension = string::to_upper_copy(exporter->getExtension());

	if (_exporters.count(extension) > 0)
	{
		_exporters.erase(extension);
		return;
	}

	rWarning() << "Cannot unregister exporter for extension " << extension << std::endl;
}

IModelExporterPtr ModelFormatManager::getExporter(const std::string& extension)
{
	auto extensionUpper = string::to_upper_copy(extension);

	auto found = _exporters.find(extensionUpper);

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

void ModelFormatManager::convertModelCmd(const cmd::ArgumentList& args)
{
    if (args.size() != 3)
    {
        rWarning() << "Usage: ConvertModel <InputPath> <OutputPath> <ExportFormat>" << std::endl;
        return;
    }

    auto inputPath = args[0].getString();
    auto outputPathRaw = args[1].getString();
    auto outputFormat = args[2].getString();

    // Get the exporter
    auto exporter = getExporter(outputFormat);

    if (!exporter)
    {
        throw cmd::ExecutionFailure(fmt::format(_("Could not find any exporter for this format: {0}"), outputFormat));
    }

    // Load the input model
    IModelPtr model;

    foreachImporter([&](const model::IModelImporterPtr& importer)
    {
        if (!model)
        {
            model = importer->loadModelFromPath(inputPath);
        }
    });

    if (!model)
    {
        throw cmd::ExecutionFailure(fmt::format(_("Could not load model file {0}"), inputPath));
    }

    // Stream all model surfaces to the exporter
    for (int i = 0; i < model->getSurfaceCount(); ++i)
    {
        auto& surface = model->getSurface(static_cast<unsigned int>(i));
        exporter->addSurface(surface, Matrix4::getIdentity());
    }

    fs::path outputPath = outputPathRaw;

    rMessage() << "Exporting model to " << outputPath.string() << std::endl;

    try
    {
        exporter->exportToPath(outputPath.parent_path().string(), outputPath.filename().string());
    }
    catch (const std::runtime_error& ex)
    {
        throw cmd::ExecutionFailure(fmt::format(_("Failed to export model to {0}: {1}"), outputPath.string(), ex.what()));
    }
}

module::StaticModuleRegistration<ModelFormatManager> _staticModelFormatManagerModule;

}
