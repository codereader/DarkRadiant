#ifndef _MAP_FILECHOOSER_PREVIEW_H_
#define _MAP_FILECHOOSER_PREVIEW_H_

#include "imapresource.h"

#include "ui/common/MapPreview.h"

#include "gtkutil/FileChooser.h"
#include <boost/shared_ptr.hpp>

#include <gtkmm/box.h>

namespace Gtk { class TextView; }

namespace map
{

class MapFileChooserPreview :
	public Gtk::VBox,
	public gtkutil::FileChooser::Preview
{
private:
	// The loaded map resource
	IMapResourcePtr _mapResource;

	// The name of the map being previewed
	std::string _mapName;

	// The description widget (text entry)
	Gtk::TextView* _usageInfo;

	// The actual MapPreview widget
	ui::MapPreview* _preview;

public:
	MapFileChooserPreview();

	virtual ~MapFileChooserPreview();

	virtual Gtk::Widget& getPreviewWidget();

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
	Gtk::Widget& createUsagePanel();

	// Sets the name of the map to preview, returns TRUE on success
	bool setMapName(const std::string& name);
};
typedef boost::shared_ptr<MapFileChooserPreview> MapFileChooserPreviewPtr;

} // namespace map

#endif /* _MAP_FILECHOOSER_PREVIEW_H_ */
