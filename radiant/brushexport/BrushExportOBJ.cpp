#include "BrushExportOBJ.h"
#include "gtkutil/filechooser.h"
#include "gtkutil/dialog.h"
#include "iregistry.h"
#include "modulesystem/ApplicationContextImpl.h"

/*	This exports the current selection into a WaveFrontOBJ file
 */
void export_selected(GtkWindow* mainWindow) {		
	
	// Obtain the path from a File Dialog Window
	std::string path = file_dialog(GTK_WIDGET(mainWindow), 
								   false, 
								   "Save as Obj",	// title
								   GlobalRegistry().get(RKEY_MAP_PATH),		// path
								   "", 		// pattern
								   ".obj"); // default extension
	
	// Open the file
	std::ofstream file(path.c_str());
	
	if (file.good()) {
		// Instantiate the Exporter of type CExportFormatWavefront
		CExporter<CExportFormatWavefront> exporter(file);
		// Traverse through the selection and export it to the file
		GlobalSelectionSystem().foreachSelected(exporter);
	}
	else {
		gtkutil::errorDialog("Unable to write to file", mainWindow);
	}
}
