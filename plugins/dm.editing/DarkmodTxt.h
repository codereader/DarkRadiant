#pragma once

#include <string>
#include <memory>
#include <vector>
#include <stdexcept>

namespace map
{

class DarkmodTxt;
typedef std::shared_ptr<DarkmodTxt> DarkmodTxtPtr;

/**
* An object representing the darkmod.txt file as found in the 
* mission folder's root directory. It contains information shown
* in the "New Mission" section in TDM's main menu.
*/
class DarkmodTxt
{
public:
	class ParseException : 
		public std::runtime_error
	{
	public:
		ParseException(const std::string& msg) :
			runtime_error(msg.c_str())
		{}
	};

private:
	std::string _title;
	std::string _author;
	std::string _description;
	std::string _version;
	std::string _reqTdmVersion;

	std::vector<std::string> _missionTitles;

public:
	static const char* NAME()
	{
		return "darkmod.txt";
	}

	const std::string& getTitle();
	const std::string& getAuthor();
	const std::string& getDescription();
	const std::string& getVersion();
	const std::string& getReqTdmVersion();

	// Named constructor parsing the given string into a DarkmodTxt instance
	// A parse exception will be thrown if the file is not compliant
	static DarkmodTxtPtr CreateFromString(const std::string& contents);

	// Named constructor parsing the given stream into a DarkmodTxt instance
	// A parse exception will be thrown if the file is not compliant
	static DarkmodTxtPtr CreateFromStream(std::istream& stream);

	// A parse exception will be thrown if the file is not compliant
	static DarkmodTxtPtr LoadForCurrentMod();

private:
	static void ParseMissionTitles(std::vector<std::string>& titleList, const std::string& source);
};

}
