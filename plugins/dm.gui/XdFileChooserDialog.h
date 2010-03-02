#include "XDataLoader.h"
#include "gtkutil/window/BlockingTransientWindow.h"

namespace ui
{
	// Imports a given definition. If the definition has been found in multiple files, the dialog shows up asking which file to use.
	class XdFileChooserDialog :
		public gtkutil::BlockingTransientWindow
	{
	public:
		enum Result
		{
			RESULT_OK,
			RESULT_CANCEL,
			RESULT_IMPORT_FAILED,
			NUM_RESULTS,
		};

	private:
		// A container for storing enumerated widgets
		GtkWidget* _treeview;

		// Gets the selection of the _treeview and stores it in the _fileIterator
		void storeSelection();

		// Return value
		Result _result;

		// The chosen filename.
		std::string _chosenFile;
	public:

		// Imports the definition given by defName and stores the result in newXData and the corresponding filename.
		// If the definition is found in mutliple files an dialog shows up, asking the user which file to use.
		static Result import(const std::string& defName, XData::XDataPtr& newXData, std::string& filename, XData::XDataLoaderPtr& loader);

	private:
		// Private constructor called by run.
		XdFileChooserDialog(const XData::XDataMap& xdMap);

		static void onOk(GtkWidget* widget, XdFileChooserDialog* self);
		static void onCancel(GtkWidget* widget, XdFileChooserDialog* self);

	};


}