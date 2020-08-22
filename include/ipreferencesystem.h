#pragma once

#include <list>
#include "imodule.h"

// A list containing possible values for a combo box widgets
typedef std::list<std::string> ComboBoxValueList;

// Common interface of all preference items (labels, combo boxes, etc.)
// All of them can carry a title and a registry key
class IPreferenceItemBase
{
public:
	virtual ~IPreferenceItemBase() {}

	typedef std::shared_ptr<IPreferenceItemBase> Ptr;

	virtual const std::string& getLabel() const = 0;

	virtual const std::string& getRegistryKey() const = 0;

	virtual void setRegistryKey(const std::string& key) = 0;
};

class IPreferenceLabel :
	public virtual IPreferenceItemBase
{
public:
	virtual ~IPreferenceLabel() {}
};

class IPreferenceEntry :
	public virtual IPreferenceItemBase
{
public:
	virtual ~IPreferenceEntry() {}
};

class IPreferenceCheckbox :
	public virtual IPreferenceItemBase
{
public:
	virtual ~IPreferenceCheckbox() {}
};

class IPreferenceCombobox :
	public virtual IPreferenceItemBase
{
public:
	virtual ~IPreferenceCombobox() {}

	virtual const ComboBoxValueList& getValues() const = 0;
	virtual bool storeValueNotIndex() const = 0;
};

class IPreferencePathEntry :
	public virtual IPreferenceItemBase
{
public:
	virtual ~IPreferencePathEntry() {}

	virtual bool browseDirectories() const = 0;
};

class IPreferenceSpinner :
	public virtual IPreferenceItemBase
{
public:
	virtual ~IPreferenceSpinner() {}

	virtual double getLower() = 0;
	virtual double getUpper() = 0;
	virtual int getFraction() = 0;
};

class IPreferenceSlider :
	public virtual IPreferenceItemBase
{
public:
	virtual ~IPreferenceSlider() {}

	virtual double getLower() = 0;
	virtual double getUpper() = 0;
	virtual double getStepIncrement() = 0;
	virtual double getPageIncrement() = 0;
	virtual int getFactor() = 0;
};

/* greebo: This is the interface the preference page has to provide for adding
 * elements to the dialog page. */
class IPreferencePage
{
public:
    // destructor
	virtual ~IPreferencePage() {}

	// Returns the title as displayed on top of the page's area
	virtual const std::string& getTitle() const = 0;

	/** 
	 * greebo: Allows to set a custom title of this page. The default title
	 * upon construction is "guessed" by adding a " Settings" to
	 * the page name, "Settings/Patch" gets assigned
	 * a "Patch Settings" as default title.
	 * Use this method to change this to fit your needs.
	 */
	virtual void setTitle(const std::string& title) = 0;

	// greebo: Returns the name (caption) of this Page (e.g. "Settings")
	virtual const std::string& getName() const = 0;

	// The preference path leading to this page
	virtual const std::string& getPath() const = 0;

	// Returns TRUE if this page doesn't hold any items
	virtual bool isEmpty() const = 0;

	virtual void foreachItem(const std::function<void(const IPreferenceItemBase::Ptr&)>& functor) const = 0;

	// greebo: Use this to add a checkbox to the preference dialog that is connected to a registry value
	virtual void appendCheckBox(const std::string& label, const std::string& registryKey) = 0;

	// greebo: This adds a horizontal slider and connects it to the given registryKey.
	virtual void appendSlider(const std::string& name, const std::string& registryKey, 
		double lower, double upper, double step_increment, double page_increment) = 0;

   /**
    * \brief
    * Add a drop-down combo box to the preference page.
    *
    * \param name
    * The name to be displayed next to the combo box.
    *
    * \param registryKey
    * The registry key which stores the value of the combo box.
    *
    * \param valueList
    * List of strings containing the values that should be displayed in the
    * combo box.
    *
    * \param storeValueNotIndex
    * If true, store the selected text in the registry key. If false, store the
    * numeric index of the selected item in the registry key. The default is
    * false.
    */
   virtual void appendCombo(const std::string& name,
                            const std::string& registryKey,
                            const ComboBoxValueList& valueList,
                            bool storeValueNotIndex = false) = 0;

	/* greebo: Appends an entry field with <name> as caption which is connected to the given registryKey
	 */
	virtual void appendEntry(const std::string& name, const std::string& registryKey) = 0;

	/* greebo: Appends an entry field with spinner buttons which retrieves its value from the given
	 * RegistryKey. The lower and upper values have to be passed as well.
	 */
	virtual void appendSpinner(const std::string& name, const std::string& registryKey,
									 double lower, double upper, int fraction) = 0;

	// greebo: Adds a PathEntry to choose files or directories (depending on the given boolean)
	virtual void appendPathEntry(const std::string& name,
									   const std::string& registryKey,
									   bool browseDirectories) = 0;

	// Appends a static label (to add some text to the preference page)
	virtual void appendLabel(const std::string& caption) = 0;
};

const char* const MODULE_PREFERENCESYSTEM("PreferenceSystem");

class IPreferenceSystem :
	public RegisterableModule
{
public:
	/** 
	 * greebo: Retrieves the page for the given path. If the page 
	 * doesn't exist yet, it will be created at the given path, for example:
	 *
	 * "Settings/Patch Settings"
	 * (spaces are ok, slashes are treated as delimiters, don't use them in the page name)
	 *
	 * Use the page interface to add widgets and connect them to registry keys.

	 * @path: The path to lookup/create
	 * @returns: the IPreferencePage reference.
	 */
	virtual IPreferencePage& getPage(const std::string& path) = 0;

	/**
	 * Visit each page, covering the whole tree
	 */
	virtual void foreachPage(const std::function<void(IPreferencePage&)>& functor) = 0;
};

inline IPreferenceSystem& GlobalPreferenceSystem()
{
	// Cache the reference locally
	static IPreferenceSystem& _prefSystem(
		*std::static_pointer_cast<IPreferenceSystem>(
			module::GlobalModuleRegistry().getModule(MODULE_PREFERENCESYSTEM)
		)
	);
	return _prefSystem;
}
