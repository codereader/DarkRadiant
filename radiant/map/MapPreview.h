#ifndef _MAP_PREVIEW_H_
#define _MAP_PREVIEW_H_

#include "gtkutil/filechooser.h"
#include <boost/shared_ptr.hpp>

namespace map {

class MapPreview :
	public gtkutil::FileChooser::Preview
{
public:
	MapPreview();

	// Retrieve the preview widget for packing into the dialog
	GtkWidget* getPreviewWidget();

	/**
	 * Gets called whenever the user changes the file selection.
	 * Note: this method must call the setPreviewActive() method on the
	 * FileChooser class to indicate whether the widget is active or not.
	 */
	void onFileSelectionChanged(const std::string& newFileName, gtkutil::FileChooser& fileChooser);
};
typedef boost::shared_ptr<MapPreview> MapPreviewPtr;

} // namespace map

#endif /* _MAP_PREVIEW_H_ */
