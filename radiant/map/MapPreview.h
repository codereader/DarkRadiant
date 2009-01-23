#ifndef _MAP_PREVIEW_H_
#define _MAP_PREVIEW_H_

#include "imapresource.h"

#include "MapPreviewCamera.h"

#include "gtkutil/filechooser.h"
#include <boost/shared_ptr.hpp>

namespace map {

// Forward decl.
class MapPreviewFilterObserver;
typedef boost::shared_ptr<MapPreviewFilterObserver> MapPreviewFilterObserverPtr;

class MapPreview :
	public gtkutil::FileChooser::Preview
{
	// The loaded map resource
	IMapResourcePtr _mapResource;

	// The name of the map being previewed
	std::string _mapName;

	GtkWidget* _previewContainer;

	MapPreviewCamera _camera;

	MapPreviewFilterObserverPtr _filterObserver;

public:
	MapPreview();

	~MapPreview();

	// Retrieve the preview widget for packing into the dialog
	GtkWidget* getPreviewWidget();

	/**
	 * Gets called whenever the user changes the file selection.
	 * Note: this method must call the setPreviewActive() method on the
	 * FileChooser class to indicate whether the widget is active or not.
	 */
	void onFileSelectionChanged(const std::string& newFileName, gtkutil::FileChooser& fileChooser);

	// Gets called by a local helper object on each FilterSystem change
	void onFiltersChanged();

private:
	// Sets the name of the map to preview, returns TRUE on success
	bool setMapName(const std::string& name);
};
typedef boost::shared_ptr<MapPreview> MapPreviewPtr;

} // namespace map

#endif /* _MAP_PREVIEW_H_ */
