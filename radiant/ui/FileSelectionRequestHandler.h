#pragma once

#include <map>
#include "imainframe.h"
#include "igame.h"
#include "iradiant.h"
#include "ifiletypes.h"
#include "messages/FileSelectionRequest.h"

#include "wxutil/FileChooser.h"

namespace ui
{

class FileSelectionRequestHandler
{
private:
	std::size_t _msgSubscription;

	// The mapping between type and last viewed path
	// like "map" => "/usr/local/game/doom3/maps/"
	// and "prefab" => "~/.doom3/darkmod/prefabs/"
	std::map<std::string, std::string> _lastDirs;

public:
	FileSelectionRequestHandler()
	{
		_msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
			radiant::IMessage::Type::FileSelectionRequest,
			radiant::TypeListener<radiant::FileSelectionRequest>(
				sigc::mem_fun(this, &FileSelectionRequestHandler::handleRequest)));

		// Load the default values
		_lastDirs[filetype::TYPE_MAP] = GlobalGameManager().getMapPath();
		_lastDirs[filetype::TYPE_PREFAB] = GlobalGameManager().getPrefabPath();
	}

	~FileSelectionRequestHandler()
	{
		GlobalRadiantCore().getMessageBus().removeListener(_msgSubscription);
	}

private:
	void handleRequest(radiant::FileSelectionRequest& request)
	{
		radiant::FileSelectionRequest::Result result;

		const auto& type = request.getType();

		// Check, if the lastdir contains at least anything and load
		// the default map path if it's empty
		if (_lastDirs.find(type) == _lastDirs.end())
		{
			// Default to the map path, if the type is not yet associated
			_lastDirs[type] = GlobalGameManager().getMapPath();
		}

		// Get the first extension from the list of possible patterns (e.g. *.pfb or *.map)
		FileTypePatterns patterns = GlobalFiletypes().getPatternsForType(type);

		std::string defaultExt = "";

		if (!patterns.empty())
		{
			defaultExt = patterns.begin()->extension; // "map"
		}

		// Display a file chooser dialog to get a new path
		wxutil::FileChooser fileChooser(request.getTitle(), 
			request.getMode() == radiant::FileSelectionRequest::Mode::Open, 
			type, defaultExt);

		fileChooser.setCurrentFile(request.getDefaultFile());
		fileChooser.setCurrentPath(_lastDirs[type]);

		std::string filePath = fileChooser.display();

		// If a filename was chosen, update the last path
		if (!filePath.empty())
		{
			_lastDirs[type] = filePath.substr(0, filePath.rfind("/"));
		}

		result.fullPath = filePath;
		result.mapFormatName = fileChooser.getSelectedMapFormat();

		request.setResult(result);
	}
};

}
