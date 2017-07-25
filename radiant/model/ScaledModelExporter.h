#pragma once

#include <string>
#include "imodel.h"
#include "imap.h"
#include <sigc++/connection.h>
#include "os/fs.h"

namespace map
{

/**
 * Helper class listening for the MapSaving event to have any
 * scaled models written to disk.
 */
class ScaledModelExporter
{
private:
	sigc::connection _mapEventConn;

public:
	void initialise();
	void shutdown();

private:
	// Signal function called by the Map module
	void onMapEvent(IMap::MapEvent ev);

	void saveScaledModels();
	void saveScaledModel(const scene::INodePtr& entity, const model::ModelNodePtr& childModel);

	fs::path getWritableGamePath();
	std::string generateUniqueModelFilename(const fs::path& outputPath,
		const fs::path& modelPath, const std::string& outputExtension);

	void exportModel(const model::IModelExporterPtr& exporter, 
		const fs::path& modelOutputPath, const std::string& modelFilename);
};

}
