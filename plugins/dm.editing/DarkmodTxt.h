#pragma once

#include <string>
#include <memory>
#include <vector>

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
private:
	std::string _title;
	std::string _author;
	std::string _description;
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
	const std::string& getReqTdmVersion();

	// Named constructor parsing the given string into a DarkmodTxt instance
	static DarkmodTxtPtr CreateFromString(const std::string& contents);

	// Named constructor parsing the given stream into a DarkmodTxt instance
	static DarkmodTxtPtr CreateFromStream(std::istream& stream);

	static DarkmodTxtPtr LoadForCurrentMod();

private:
	static void ParseMissionTitles(std::vector<std::string>& titleList, const std::string& source);
};

}
