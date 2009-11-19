#ifndef _MAP_FILECHOOSER_PREVIEW_H_
#define _MAP_FILECHOOSER_PREVIEW_H_

#include "imapresource.h"

#include "ui/common/MapPreview.h"

#include "gtkutil/FileChooser.h"
#include <boost/shared_ptr.hpp>

namespace map {

class MapFileChooserPreview :
	public gtkutil::FileChooser::Preview
{
	// The loaded map resource
	IMapResourcePtr _mapResource;

	// The name of the map being previewed
	std::string _mapName;

	GtkWidget* _previewContainer;

	// The description widget (text entry)
	GtkWidget* _usageInfo;

	// The actual MapPreview widget
	ui::MapPreview _preview;

public:
	MapFileChooserPreview();

	virtual ~MapFileChooserPreview() {}

	// Retrieve the preview widget for packing into the dialog
	GtkWidget* getPreviewWidget();

	/**
	 * Gets called whenever the user changes the file selection.
	 * Note: this method must call the setPreviewActive() method on the
	 * FileChooser class to indicate whether the widget is active or not.
	 */
	void onFileSelectionChanged(const std::string& newFileName, ui::IFileChooser& fileChooser);

private:
	// Updates the usage info based on the prefab's worldspawn spawnargs
	void updateUsageInfo();

	// The usage text view
	GtkWidget* createUsagePanel();

	// Sets the name of the map to preview, returns TRUE on success
	bool setMapName(const std::string& name);
};
typedef boost::shared_ptr<MapFileChooserPreview> MapFileChooserPreviewPtr;

} // namespace map

#endif /* _MAP_FILECHOOSER_PREVIEW_H_ */
