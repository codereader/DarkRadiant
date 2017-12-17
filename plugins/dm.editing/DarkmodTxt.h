#pragma once

#include <vector>
#include "MissionInfoTextFile.h"

namespace map
{

class DarkmodTxt;
typedef std::shared_ptr<DarkmodTxt> DarkmodTxtPtr;

/**
* An object representing the darkmod.txt file as found in the 
* mission folder's root directory. It contains information shown
* in the "New Mission" section in TDM's main menu.
*/
class DarkmodTxt :
	public MissionInfoTextFile
{
public:
	typedef std::vector<std::string> TitleList;

private:
	std::string _title;
	std::string _author;
	std::string _description;
	std::string _version;
	std::string _reqTdmVersion;

	TitleList _missionTitles;

public:
	static const char* NAME()
	{
		return "darkmod.txt";
	}

	std::string getFilename() override;

	const std::string& getTitle();
	void setTitle(const std::string& title);

	const std::string& getAuthor();
	void setAuthor(const std::string& author);

	const std::string& getDescription();
	void setDescription(const std::string& desc);

	const std::string& getVersion();
	void setVersion(const std::string& version);

	const std::string& getReqTdmVersion();
	void setReqTdmVersion(const std::string& reqVersion);

	// Returns the mission titles (the first element is the same as getTitle())
	const TitleList& getMissionTitles();
	void setMissionTitles(const TitleList& list);

	// Named constructor parsing the given string into a DarkmodTxt instance
	// A parse exception will be thrown if the file is not compliant
	static DarkmodTxtPtr CreateFromString(const std::string& contents);

	// Named constructor parsing the given stream into a DarkmodTxt instance
	// A parse exception will be thrown if the file is not compliant
	static DarkmodTxtPtr CreateFromStream(std::istream& stream);

	// A parse exception will be thrown if the file is not compliant
	static DarkmodTxtPtr LoadForCurrentMod();

	// Retrieves the text representation of this instance, as it will be written to the darkmod.txt file
	std::string toString() override;

private:
	static void ParseMissionTitles(std::vector<std::string>& titleList, const std::string& source);
};

}
