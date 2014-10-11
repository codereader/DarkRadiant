#pragma once

#include "TextSpecifierPanel.h"

namespace objectives
{

namespace ce
{

/**
 * SpecifierPanel subclass for the SPEC_GROUP specifier type.
 * It provides a text entry box with auto-completion functionality
 * for a few special cases like "loot_gold" etc.
 */
class GroupSpecifierPanel :
	public TextSpecifierPanel
{
private:
	// Map registration
	static struct RegHelper {
		RegHelper() {
			SpecifierPanelFactory::registerType(
				SpecifierType::SPEC_GROUP().getName(),
				SpecifierPanelPtr(new GroupSpecifierPanel())
			);
		}
	} _regHelper;

	GroupSpecifierPanel()
	{}

	/**
	 * Construct a GroupSpecifierPanel.
	 */
	GroupSpecifierPanel(wxWindow* parent);

public:
	// SpecifierPanel implementation
	SpecifierPanelPtr create(wxWindow* parent) const
	{
		return SpecifierPanelPtr(new GroupSpecifierPanel(parent));
	}
};

} // namespace objectives

} // namespace ce
