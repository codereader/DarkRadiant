#pragma once

#include "registry/buffer.h"
#include <wx/panel.h>

#include "ipreferencesystem.h"
#include "PreferenceItemBase.h"

class wxTreebook;
class wxFlexGridSizer;
class wxStaticText;
class wxSizer;
class wxScrolledWindow;

namespace ui {

class PrefPage;
typedef std::shared_ptr<PrefPage> PrefPagePtr;

class PrefPage :
	public IPreferencesPage,
	public std::enable_shared_from_this<PrefPage>
{
private:
	// We're holding back any registry write operations until the user clicks OK
	registry::Buffer _registryBuffer;

	// A signal chain all registry key-bound widgets are connected with
	// when emitted, the widgets reload the values from the registry.
	sigc::signal<void> _resetValuesSignal;

	// The table this page is adding the widgets to
	wxFlexGridSizer* _table;

  	// The list of child pages
	std::vector<PrefPagePtr> _children;

	// The name (caption) of this page
	std::string _name;

	// The full path of this object
	std::string _path;

	// The actual page that gets attached to the notebook
	wxScrolledWindow* _pageWidget;

	wxStaticText* _titleLabel;

	// The items of this page
	std::vector<PreferenceItemBasePtr> _items;

public:
	/** greebo: Constructor
	 *
	 * @name: The display caption of this prefpage
	 * @parentPath: the path to the parent of this page
	 */
	PrefPage(const std::string& name,
			 const PrefPagePtr& parentPage = PrefPagePtr());

	/** greebo: Sets the title caption that is displayed on the right.
	 * 			Overrides the default title that is generated
	 * 			on construction (the one with the " Settings" postfix).
	 */
	void setTitle(const std::string& title);

	/** greebo: Returns the full path to this PrefPage
	 */
	const std::string& getPath() const;

	/** greebo: Returns the name (caption) of this Page (e.g. "Settings")
	 */
	std::string getName() const;

	/**
	 * Commit all pending registry write operations.
	 */
	void saveChanges();

	/** 
	 * Discard all pending registry write operations.
	 */
	void discardChanges();

	// Creates the widget structure for packing into the notebook.
	wxWindow* createWidget(wxTreebook* parent);

	wxWindow* getWidget();

	void destroyWidgets();

	void foreachPage(const std::function<void(PrefPage&)>& functor);

	// Appends a simple static label
	void appendLabel(const std::string& caption);

	/* greebo: This adds a checkbox and connects it to an XMLRegistry key.
	 * @returns: the pointer to the created GtkWidget */
	void appendCheckBox(const std::string& name, const std::string& flag, const std::string& registryKey);

	/* greebo: This adds a horizontal slider to the internally referenced VBox and connects
	 * it to the given registryKey. */
	void appendSlider(const std::string& name, const std::string& registryKey,
	                  double value, double lower, double upper, double step_increment, double page_increment) ;

    void appendCombo(const std::string& name,
                     const std::string& registryKey,
                     const ComboBoxValueList& valueList,
                     bool storeValueNotIndex = false);

	/* greebo: Appends an entry field with <name> as caption which is connected to the given registryKey
	 */
	void appendEntry(const std::string& name, const std::string& registryKey);

	// greebo: Adds a PathEntry to choose files or directories (depending on the given boolean)
	void appendPathEntry(const std::string& name, const std::string& registryKey, bool browseDirectories);

	/* greebo: Appends an entry field with spinner buttons which retrieves its value from the given
	 * RegistryKey. The lower and upper values have to be passed as well.
	 */
	void appendSpinner(const std::string& name, const std::string& registryKey,
	                         double lower, double upper, int fraction);

	/** greebo: Performs a recursive lookup of the given path
	 * 			and creates any items that do not exist.
	 *
	 * @returns: the shared_ptr to the PrefPage, can be empty on error.
	 */
	PrefPagePtr createOrFindPage(const std::string& path);

private:
	void appendNamedWidget(const std::string& name, wxWindow* widget, bool useFullWidth = true);
};

} // namespace ui
