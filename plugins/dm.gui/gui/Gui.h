#pragma once

#include "igui.h"

#include <map>
#include <unordered_map>
#include <memory>
#include "parser/DefTokeniser.h"
#include "GuiWindowDef.h"

namespace gui
{

class Gui;
typedef std::shared_ptr<Gui> GuiPtr;

class GuiWindowDef;
typedef std::shared_ptr<GuiWindowDef> GuiWindowDefPtr;

/**
 * greebo: This class represents a single D3 GUI. It holds all
 * the windowDefs and the source code behind.
 */
class Gui :
	public IGui
{
private:
	// The desktop window
	IGuiWindowDefPtr _desktop;

	// The global GUI state variables
	typedef std::unordered_map<std::string, std::string> GuiState;
	GuiState _state;

	typedef std::unordered_map<std::string, sigc::signal<void>> GuiStateChangedSignals;
	GuiStateChangedSignals _stateSignals;

public:
	const IGuiWindowDefPtr& getDesktop() const override;
	void setDesktop(const IGuiWindowDefPtr& newDesktop) override;

	// Sets the given state variable (gui::<key> = <value>)
	void setStateString(const std::string& key, const std::string& value) override;

	// Allocate and/or return the changed signal for the given key
	sigc::signal<void>& getChangedSignalForState(const std::string& key) override;

	// Returns the state string "gui::<key>" or an empty string if non-existent
	std::string getStateString(const std::string& key) override;

	// Sets up the time of the entire GUI (all windowDefs)
	void initTime(const std::size_t time) override;

	// "Think" routine, advances all active windowDefs (where notime == false)
	void update(const std::size_t timestep) override;

	// Returns a reference to the named windowDef, returns NULL if not found
	IGuiWindowDefPtr findWindowDef(const std::string& name) override;

	// Called by the GuiRenderer to re-compile text VBOs, etc.
	void pepareRendering() override;

	// Takes the given token stream and attempts to construct a GUI object from it
	// Returns NULL on failure
	static GuiPtr createFromTokens(parser::DefTokeniser& tokeniser);
};

} // namespace
