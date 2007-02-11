#include "BrushExportOBJ.h"
#include "gtkutil/filechooser.h"

/*	This exports the current selection into a WaveFrontOBJ file
 */
void export_selected(GtkWindow* mainWindow) {		
	
	// Obtain the path from a File Dialog Window
	std::string path = file_dialog(GTK_WIDGET(mainWindow), 
								   false, 
								   "Save as Obj");
	
	// Open the file
	TextFileOutputStream file(path.c_str());
	
	if (!file.failed()) {
		// Instantiate the Exporter of type CExportFormatWavefront
		CExporter<CExportFormatWavefront> exporter(file);
		// Traverse through the selection and export it to the file
		GlobalSelectionSystem().foreachSelected(exporter);
	}
	else {
		GlobalRadiant().m_pfnMessageBox(GTK_WIDGET(mainWindow), "Unable to write to file", "Error",
                                    	eMB_OK, eMB_ICONERROR);
	}
}
