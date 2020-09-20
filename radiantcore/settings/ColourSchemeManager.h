#pragma once

#include <string>
#include <map>
#include "icolourscheme.h"

#include "ColourScheme.h"

namespace colours
{

class ColourSchemeManager :
	public IColourSchemeManager
{
private:
	// A map storing all the ColourSchemes with the name as key
	std::map<const std::string, ColourScheme> _colourSchemes;

	// the name of the active colourscheme
	std::string _activeScheme;

public:
	void foreachScheme(const std::function<void(const std::string&, IColourScheme&)>& functor) override;

	bool schemeExists(const std::string& name) override;

	bool isActive(const std::string& name);
	void setActive(const std::string& name) override;

	// Returns the requested colour from the currently active scheme
	Vector3 	getColour(const std::string& colourName);

	ColourScheme& getActiveScheme() override;
	ColourScheme& getColourScheme(const std::string& name) override;

	// Loads/saves all the schemes from the registry
	void loadColourSchemes();
	void saveColourSchemes();

	// Saves the specified scheme into the registry
	void saveScheme(const std::string& name) override;
	void deleteScheme(const std::string& name) override;
	void copyScheme(const std::string& fromName, const std::string& toName) override;

	// Reverts all changes to the current objects and re-load them from the registry
	void restoreColourSchemes();

	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;
};

} // namespace
