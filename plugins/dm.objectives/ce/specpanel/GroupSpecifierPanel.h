#ifndef GROUPSPECIFIERPANEL_H_
#define GROUPSPECIFIERPANEL_H_

#include "SpecifierPanel.h"
#include "TextSpecifierPanel.h"

typedef struct _GtkListStore GtkListStore;

namespace objectives {

namespace ce {

/**
 * SpecifierPanel subclass for the SPEC_GROUP specifier type.
 * It provides a text entry box with auto-completion functionality
 * for a few special cases like "loot_gold" etc.
 */
class GroupSpecifierPanel : 
	public SpecifierPanel
{
	// Map registration
	static struct RegHelper {
		RegHelper() { 
			SpecifierPanelFactory::registerType(
				SpecifierType::SPEC_GROUP().getName(),
				SpecifierPanelPtr(new GroupSpecifierPanel())
			);
		}
	} _regHelper;

private:
	// Main widget
	GtkWidget* _widget;

protected:
    // gtkutil::EditorWidget implementation
    virtual GtkWidget* _getWidget() const;

public:
	/**
	 * Construct a GroupSpecifierPanel.
	 */
	GroupSpecifierPanel();

	/**
	 * Destroy this GroupSpecifierPanel including all widgets.
	 */
	~GroupSpecifierPanel();

	// SpecifierPanel implementation
	SpecifierPanelPtr clone() const {
		return SpecifierPanelPtr(new GroupSpecifierPanel());
	}

	// gtkutil::EditorWidget implementation
    void setValue(const std::string& value);
    std::string getValue() const;

private:
	// Creates and fills the auto-completion liststore for this specifier panel
	GtkListStore* createCompletionListStore();
};

} // namespace objectives

} // namespace ce

#endif /* GROUPSPECIFIERPANEL_H_ */
