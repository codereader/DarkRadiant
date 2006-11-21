#include "BrushExportOBJ.h"

/*	This exports the current selection into a WaveFrontOBJ file
 */
void export_selected(GtkWindow* mainWindow) {		
	// Obtain the path from a File Dialog Window
	const char* path = GlobalRadiant().m_pfnFileDialog(GTK_WIDGET(mainWindow), false, "Save as Obj", 0, 0);
	
	// Open the file
	TextFileOutputStream file(path);
	
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
