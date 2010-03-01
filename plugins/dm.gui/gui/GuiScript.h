#ifndef _GUI_SCRIPT_H_
#define _GUI_SCRIPT_H_

#include <boost/shared_ptr.hpp>

namespace parser { class DefTokeniser; }

namespace gui
{

class GuiWindowDef;

class GuiScript
{
private:
	// The owning windowDef
	GuiWindowDef& _owner;

public:
	GuiScript(GuiWindowDef& owner);

	// Construct this script from a token stream. 
	// It expects the initial opening brace as first token.
	void constructFromTokens(parser::DefTokeniser& tokeniser);

private:
	std::string getExpression(parser::DefTokeniser& tokeniser);
};
typedef boost::shared_ptr<GuiScript> GuiScriptPtr;

} // namespace

#endif /* _GUI_SCRIPT_H_ */
