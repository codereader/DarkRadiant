#ifndef PREFPAGE_H_
#define PREFPAGE_H_

#include "ipreferencesystem.h"
#include "gtkutil/ifc/Widget.h"
#include "gtkutil/RegistryConnector.h"

namespace ui {

class PrefPage;
typedef boost::shared_ptr<PrefPage> PrefPagePtr;

class PrefPage : 
	public PreferencesPage
{
public:
	class Visitor 
	{
	public:
	    virtual ~Visitor() {}
		virtual void visit(PrefPagePtr prefPage) = 0;
	};

private:
	// The vbox this page is adding the widgets to
	GtkWidget* _vbox;
  
  	// The list of child pages
	std::vector<PrefPagePtr> _children;
	
	// The name (caption) of this page
	std::string _name;
	
	// The full path of this object
	std::string _path;
	
	// The notebook this page is packed into
	GtkWidget* _notebook;
	
	// The actual page that gets attached to the notebook
	GtkWidget* _pageWidget;
	
	GtkWidget* _titleLabel;
	
	// The reference to the dialog's connector object
	gtkutil::RegistryConnector& _connector;

	// A list of heap-allocated Widets, to be destroyed on PrefPage destruction
	std::list<gtkutil::WidgetPtr> _widgets;

public:
	/** greebo: Constructor
	 * 
	 * @name: The display caption of this prefpage
	 * @parentPath: the path to the parent of this page
	 * @notebook: The GtkNotebook widget this page is child of.
	 * @connector: the reference to the RegistryConnector that can
	 * 			   be used to connect the widget to the registry.
	 * 			   Usually, this owned by the PrefsDlg class.
	 */
	PrefPage(const std::string& name,
	         const std::string& parentPath,
	         GtkWidget* notebook,
	         gtkutil::RegistryConnector& connector);
	
	/** greebo: Sets the title caption that is displayed on the right.
	 * 			Overrides the default title that is generated 
	 * 			on construction (the one with the " Settings" postfix).
	 */
	void setTitle(const std::string& title);
	
	/** greebo: Returns the full path to this PrefPage
	 */
	std::string getPath() const;
	
	/** greebo: Returns the name (caption) of this Page (e.g. "Settings")
	 */
	std::string getName() const;
	
	/** greebo: Returns the widget that can be used to determine
	 * 			the notebook page number.
	 */
	GtkWidget* getWidget();
	
	void foreachPage(Visitor& visitor);
	
	// Appends a simple static label
	GtkWidget* appendLabel(const std::string& caption);
	
	/* greebo: This adds a checkbox and connects it to an XMLRegistry key.
	 * @returns: the pointer to the created GtkWidget */
	GtkWidget* appendCheckBox(const std::string& name, const std::string& flag, const std::string& registryKey);
	
	/* greebo: This adds a horizontal slider to the internally referenced VBox and connects
	 * it to the given registryKey. */
	void appendSlider(const std::string& name, const std::string& registryKey, bool drawValue,
	                  double value, double lower, double upper, double step_increment, double page_increment, double page_size) ;
	
    void appendCombo(const std::string& name,
                     const std::string& registryKey,
                     const ComboBoxValueList& valueList,
                     bool storeValueNotIndex = false);
	
	/* greebo: Appends an entry field with <name> as caption which is connected to the given registryKey
	 */
	GtkWidget* appendEntry(const std::string& name, const std::string& registryKey);
	
	// greebo: Adds a PathEntry to choose files or directories (depending on the given boolean)
	GtkWidget* appendPathEntry(const std::string& name, const std::string& registryKey, bool browseDirectories);
	
	/* greebo: Appends an entry field with spinner buttons which retrieves its value from the given
	 * RegistryKey. The lower and upper values have to be passed as well.
	 */
	GtkWidget* appendSpinner(const std::string& name, const std::string& registryKey,
	                         double lower, double upper, int fraction);

	/** greebo: Performs a recursive lookup of the given path
	 * 			and creates any items that do not exist.
	 * 
	 * @returns: the shared_ptr to the PrefPage, can be empty on error.
	 */
	PrefPagePtr createOrFindPage(const std::string& path);

private:
	void appendNamedWidget(const std::string& name, GtkWidget* widget);
};

} // namespace ui

#endif /*PREFPAGE_H_*/
