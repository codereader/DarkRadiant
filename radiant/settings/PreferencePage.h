#pragma once

#include "ipreferencesystem.h"
#include <vector>
#include <memory>

#include "PreferenceItemBase.h"

namespace settings
{

class PreferencePage;
typedef std::shared_ptr<PreferencePage> PreferencePagePtr;

class PreferencePage :
	public IPreferencePage,
	public std::enable_shared_from_this<PreferencePage>
{
private:
	// The name (caption) of this page
	std::string _name;

	// The title which is by default "{_name} Settings" but can be overridden
	std::string _title;

	// The full path of this object
	std::string _path;

	// The list of child pages
	std::vector<PreferencePagePtr> _children;

	// The registered items
	std::vector<PreferenceItemBasePtr> _items;

public:
	/** 
	 * Construct this passing the name and the optional parent page.
	*/
	PreferencePage(const std::string& name,
				   const PreferencePagePtr& parentPage = PreferencePagePtr());

	// Returns the title as displayed on top of the page's area
	const std::string& getTitle() const;

	/** 
	 * greebo: Sets the title caption that is displayed on the right.
	 * Overrides the default title that is generated
	 * on construction (the one with the " Settings" postfix).
	 */
	void setTitle(const std::string& title) override;

	// greebo: Returns the full path to this page
	const std::string& getPath() const;

	// greebo: Returns the name (caption) of this Page (e.g. "Settings")
	const std::string& getName() const;

	/** 
	 * greebo: Performs a recursive lookup of the given path
	 * and creates any items that do not exist.
 	 * @returns: the shared_ptr to the PrefPage.
	 * @throws: a std::logic_error if the given path is empty
	 */
	PreferencePage& createOrFindPage(const std::string& path);

	// Recursively visit all children of this page
	void foreachChildPage(const std::function<void(PreferencePage&)>& functor);

	// Hit the functor with each item on this page
	void foreachItem(const std::function<void(const PreferenceItemBasePtr&)>& functor) const;
	
	// Returns TRUE if this page doesn't hold any items
	bool isEmpty() const;

	// IPreferencePage implementation
	void appendCheckBox(const std::string& label, const std::string& registryKey) override;

	void appendSlider(const std::string& name, const std::string& registryKey,
		double lower, double upper, double stepIncrement, double pageIncrement) override;

	void appendCombo(const std::string& name,
		const std::string& registryKey,
		const ComboBoxValueList& valueList,
		bool storeValueNotIndex = false) override;

	void appendEntry(const std::string& name, const std::string& registryKey) override;

	void appendSpinner(const std::string& name, const std::string& registryKey,
		double lower, double upper, int fraction) override;

	void appendPathEntry(const std::string& name, const std::string& registryKey,
		bool browseDirectories) override;

	void appendLabel(const std::string& caption) override;
};

}