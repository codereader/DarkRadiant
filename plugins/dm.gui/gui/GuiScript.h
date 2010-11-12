#ifndef _GUI_SCRIPT_H_
#define _GUI_SCRIPT_H_

#include <list>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "Variable.h"

namespace parser { class DefTokeniser; }

namespace gui
{

class GuiWindowDef;

// A single script statement
// Statement arguments remain in std::string form and are evaluated on the fly
struct Statement
{
	// Possible statement types
	enum Type
	{
		ST_NOP,
		ST_JMP,
		ST_SET,
		ST_TRANSITION,
		ST_IF,
		ST_SET_FOCUS,
		ST_ENDGAME,
		ST_RESET_TIME,
		ST_SHOW_CURSOR,
		ST_RESET_CINEMATICS,
		ST_LOCALSOUND,
		ST_RUNSCRIPT,
		ST_EVALREGS,
	};

	// The statement type
	Type type;

	typedef std::vector<std::string> Arguments;
	Arguments args;

	// The jump destination used by ST_IF and ST_JMP
	std::size_t jmpDest;

	Statement(Type type_) :
		type(type_),
		jmpDest(0)
	{}
};
typedef boost::shared_ptr<Statement> StatementPtr;

class GuiScript
{
private:
	// The owning windowDef
	GuiWindowDef& _owner;

	// The compiled "code" consists of a series of statements
	typedef std::vector<StatementPtr> StatementList;
	StatementList _statements;

	// The instruction pointer
	std::size_t _ip;

	// Current block level for parsing the code
	std::size_t _curLevel;

public:
	GuiScript(GuiWindowDef& owner);

	// Construct this script from a token stream.
	// It expects the initial opening brace as first token.
	void constructFromTokens(parser::DefTokeniser& tokeniser);

	// Start or continue execution of this script
	void execute();

private:
	// Interprets the given string and returns the target object
	VariablePtr getVariableFromExpression(const std::string& expr);
	std::string getValueFromExpression(const std::string& expr);

	const Statement& getStatement(std::size_t index);

	// Adds a statement to the compiled list, returns the index of this statement
	std::size_t pushStatement(const StatementPtr& statement);

	// Returns the current position in the code
	std::size_t getCurPosition();

	std::string getExpression(parser::DefTokeniser& tokeniser);
	std::string getIfExpression(parser::DefTokeniser& tokeniser);

	// Reads a "statement", which can be single one or a group (surrounded by curly braces)
	void parseStatement(parser::DefTokeniser& tokeniser);

	// Contains a large if-else array to jump into the specialised methods below
	void switchOnToken(const std::string& token, parser::DefTokeniser& tokeniser);

	// Parse routines specialised on keywords
	void parseIfStatement(parser::DefTokeniser& tokeniser);
	void parseSetStatement(parser::DefTokeniser& tokeniser);
	void parseTransitionStatement(parser::DefTokeniser& tokeniser);
	void parseSetFocusStatement(parser::DefTokeniser& tokeniser);
	void parseEndGameStatement(parser::DefTokeniser& tokeniser);
	void parseResetTimeStatement(parser::DefTokeniser& tokeniser);
	void parseShowCursorStatement(parser::DefTokeniser& tokeniser);
	void parseResetCinematicStatement(parser::DefTokeniser& tokeniser);
	void parseLocalSoundStatement(parser::DefTokeniser& tokeniser);
	void parseRunScriptStatement(parser::DefTokeniser& tokeniser);
	void parseEvalRegsStatement(parser::DefTokeniser& tokeniser);
};
typedef boost::shared_ptr<GuiScript> GuiScriptPtr;

} // namespace

#endif /* _GUI_SCRIPT_H_ */
