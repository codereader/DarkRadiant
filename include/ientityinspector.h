#ifndef _IENTITY_INSPECTOR_H_
#define _IENTITY_INSPECTOR_H_

#include "imodule.h"

// Forward decl.
typedef struct _GtkWidget GtkWidget;

namespace ui
{

class IEntityInspector :
	public RegisterableModule
{
public:
	// Returns the top-level widget
	virtual GtkWidget* getWidget() = 0;

	// Lets the EntityInspector restore its settings from the Registry
	virtual void restoreSettings() = 0;
};

} // namespace ui

const std::string MODULE_ENTITYINSPECTOR("EntityInspector");

inline ui::IEntityInspector& GlobalEntityInspector()
{
	// Cache the reference locally
	static ui::IEntityInspector& _inspector(
		*boost::static_pointer_cast<ui::IEntityInspector>(
			module::GlobalModuleRegistry().getModule(MODULE_ENTITYINSPECTOR)
		)
	);
	return _inspector;
}

#endif /* _IENTITY_INSPECTOR_H_ */
