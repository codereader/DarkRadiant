#include "LanguageManager.h"

#include <glib.h>
#include <libintl.h>

#include "os/path.h"
#include "os/file.h"
#include "i18n.h"
#include "ipreferencesystem.h"
#include "itextstream.h"
#include <fstream>
#include <stdexcept>
#include "registry/registry.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace fs = boost::filesystem;

namespace language
{

class UnknownLanguageException :
	public std::runtime_error
{
public:
	UnknownLanguageException(const std::string& what) :
		std::runtime_error(what)
	{}
};

namespace
{
	const char* const LANGUAGE_SETTING_FILE = "darkradiant.language";
	const char* const DEFAULT_LANGUAGE = "en";
	const std::string RKEY_LANGUAGE("user/ui/language");
}

const std::string& LanguageManager::getName() const
{
	static std::string _name("LanguageManager");
	return _name;
}

const StringSet& LanguageManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
	}

	return _dependencies;
}

void LanguageManager::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called" << std::endl;

	// Fill array of supported languages
	loadSupportedLanguages();

	// Fill array of available languages
	findAvailableLanguages();

	int curLangIndex = 0; // english

	try
	{
		int index = getLanguageIndex(_curLanguage);

		// Get the offset into the array of available languages
		LanguageList::iterator found =
			std::find(_availableLanguages.begin(), _availableLanguages.end(), index);

		if (found != _availableLanguages.end())
		{
			curLangIndex = static_cast<int>(std::distance(_availableLanguages.begin(), found));
		}
	}
	catch (UnknownLanguageException&)
	{
		rWarning() << "Warning, unknown language found in " <<
			LANGUAGE_SETTING_FILE << ", reverting to English" << std::endl;
	}

	// Construct the list of available languages
	ComboBoxValueList langs;

	for (LanguageList::const_iterator i = _availableLanguages.begin();
		 i != _availableLanguages.end(); ++i)
	{
		const Language& lang = _supportedLanguages[*i];
		langs.push_back(lang.twoDigitCode + " - " + lang.displayName);
	}

	// Load the currently selected index into the registry
	registry::setValue(RKEY_LANGUAGE, curLangIndex);
	GlobalRegistry().setAttribute(RKEY_LANGUAGE, "volatile", "1"); // don't save this to user.xml

	// Add Preferences
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage(_("Settings/Language"));
	page->appendCombo(_("Language"), RKEY_LANGUAGE, langs);

	page->appendLabel(_("<b>Note:</b> You'll need to restart DarkRadiant after changing the language setting."));
}

void LanguageManager::shutdownModule()
{
	// Get the language setting from the registry (this is an integer)
	// and look up the language code (two digit)
	int langNum = registry::getValue<int>(RKEY_LANGUAGE);

	assert(langNum >= 0 && langNum < static_cast<int>(_availableLanguages.size()));

	// Look up the language index in the list of available languages
	int langIndex = _availableLanguages[langNum];

	assert(_supportedLanguages.find(langIndex) != _supportedLanguages.end());

	// Save the language code to the settings file
	saveLanguageSetting(_supportedLanguages[langIndex].twoDigitCode);
}

void LanguageManager::findAvailableLanguages()
{
	// English (index 0) is always available
	_availableLanguages.push_back(0);

	// Search folder
	fs::path start(_i18nPath);

	if (!fs::exists(start))
	{
		rWarning() << "Cannot find i18n directory, skipping search for language files." << std::endl;
		return;
	}

	for (fs::directory_iterator it(start); it != fs::directory_iterator(); ++it)
	{
		// Get the candidate
		const fs::path& candidate = *it;

		if (fs::is_directory(candidate))
		{
			// Get the index (is this a known language?)
			try
			{
				int index = getLanguageIndex(candidate.filename().string());

				// Add this to the list (could use more extensive checking, but this is enough for now)
				_availableLanguages.push_back(index);
			}
			catch (UnknownLanguageException&)
			{
				rWarning() << "Skipping unknown language: "
					<< candidate.filename() << std::endl;
				continue;
			}
		}
	}

	rMessage() << "Found " << _availableLanguages.size() << " language folders." << std::endl;
}

void LanguageManager::init(const ApplicationContext& ctx)
{
	// Instantiate a new language manager
	LanguageManagerPtr instancePtr(new LanguageManager);

	// Hand that over to the module registry
	module::getRegistry().registerModule(instancePtr);

	// Initialise the module manually
	instancePtr->initFromContext(ctx);
}

void LanguageManager::initFromContext(const ApplicationContext& ctx)
{
	// Initialise these members
	_languageSettingFile = ctx.getSettingsPath() + LANGUAGE_SETTING_FILE;
	_curLanguage = loadLanguageSetting();

	rMessage() << "Current language setting: " << _curLanguage << std::endl;

    // No handling of POSIX needed, since we don't use the LanguageManager on
    // POSIX
	_i18nPath = os::standardPathWithSlash(
        ctx.getApplicationPath() + "i18n"
    );

    // Set the LANG environment. As GLIB/GTK+ (in Win32) is using its own C
    // runtime, we need to call their GLIB setenv function for the environment
    // variable to take effect.
	g_setenv("LANG", _curLanguage.c_str(), TRUE);

	// Tell glib to load stuff from the given i18n path
	bindtextdomain(GETTEXT_PACKAGE, _i18nPath.c_str());

    // set encoding to utf-8 to prevent errors for Windows
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
}

std::string LanguageManager::loadLanguageSetting()
{
	std::string language = DEFAULT_LANGUAGE;

	// Check for an existing language setting file in the user settings folder
	std::ifstream str(_languageSettingFile.c_str());

	if (str.good())
	{
		str >> language;
	}

	return language;
}

void LanguageManager::saveLanguageSetting(const std::string& language)
{
	std::ofstream str(_languageSettingFile.c_str());

	str << language;

	str.flush();
	str.close();
}

void LanguageManager::loadSupportedLanguages()
{
	_supportedLanguages.clear();

	int index = 0;

	_supportedLanguages[index++] = Language("en", _("English"));
	_supportedLanguages[index++] = Language("ab", _("Abkhazian"));
	_supportedLanguages[index++] = Language("ae", _("Avestan"));
	_supportedLanguages[index++] = Language("af", _("Afrikaans"));
	_supportedLanguages[index++] = Language("ak", _("Akan"));
	_supportedLanguages[index++] = Language("am", _("Amharic"));
	_supportedLanguages[index++] = Language("an", _("Aragonese"));
	_supportedLanguages[index++] = Language("ar", _("Arabic"));
	_supportedLanguages[index++] = Language("as", _("Assamese"));
	_supportedLanguages[index++] = Language("av", _("Avaric"));
	_supportedLanguages[index++] = Language("ay", _("Aymara"));
	_supportedLanguages[index++] = Language("az", _("Azerbaijani"));
	_supportedLanguages[index++] = Language("ba", _("Bashkir"));
	_supportedLanguages[index++] = Language("be", _("Belarusian"));
	_supportedLanguages[index++] = Language("bg", _("Bulgarian"));
	_supportedLanguages[index++] = Language("bh", _("Bihari languages"));
	_supportedLanguages[index++] = Language("bi", _("Bislama"));
	_supportedLanguages[index++] = Language("bm", _("Bambara"));
	_supportedLanguages[index++] = Language("bn", _("Bengali"));
	_supportedLanguages[index++] = Language("bo", _("Tibetan"));
	_supportedLanguages[index++] = Language("br", _("Breton"));
	_supportedLanguages[index++] = Language("bs", _("Bosnian"));
	_supportedLanguages[index++] = Language("ca", _("Catalan"));
	_supportedLanguages[index++] = Language("ce", _("Chechen"));
	_supportedLanguages[index++] = Language("ch", _("Chamorro"));
	_supportedLanguages[index++] = Language("co", _("Corsican"));
	_supportedLanguages[index++] = Language("cr", _("Cree"));
	_supportedLanguages[index++] = Language("cs", _("Czech"));
	_supportedLanguages[index++] = Language("cv", _("Chuvash"));
	_supportedLanguages[index++] = Language("cy", _("Welsh"));
	_supportedLanguages[index++] = Language("cy", _("Welsh"));
	_supportedLanguages[index++] = Language("da", _("Danish"));
	_supportedLanguages[index++] = Language("de", _("German"));
	_supportedLanguages[index++] = Language("dv", _("Divehi"));
	_supportedLanguages[index++] = Language("dz", _("Dzongkha"));
	_supportedLanguages[index++] = Language("ee", _("Ewe"));
	_supportedLanguages[index++] = Language("el", _("Greek"));
	_supportedLanguages[index++] = Language("en", _("English"));
	_supportedLanguages[index++] = Language("eo", _("Esperanto"));
	_supportedLanguages[index++] = Language("es", _("Spanish"));
	_supportedLanguages[index++] = Language("et", _("Estonian"));
	_supportedLanguages[index++] = Language("eu", _("Basque"));
	_supportedLanguages[index++] = Language("fa", _("Persian"));
	_supportedLanguages[index++] = Language("ff", _("Fulah"));
	_supportedLanguages[index++] = Language("fi", _("Finnish"));
	_supportedLanguages[index++] = Language("fj", _("Fijian"));
	_supportedLanguages[index++] = Language("fo", _("Faroese"));
	_supportedLanguages[index++] = Language("fr", _("French"));
	_supportedLanguages[index++] = Language("fy", _("Western Frisian"));
	_supportedLanguages[index++] = Language("ga", _("Irish"));
	_supportedLanguages[index++] = Language("gd", _("Gaelic; Scottish Gaelic"));
	_supportedLanguages[index++] = Language("gl", _("Galician"));
	_supportedLanguages[index++] = Language("gn", _("Guarani"));
	_supportedLanguages[index++] = Language("gu", _("Gujarati"));
	_supportedLanguages[index++] = Language("gv", _("Manx"));
	_supportedLanguages[index++] = Language("ha", _("Hausa"));
	_supportedLanguages[index++] = Language("he", _("Hebrew"));
	_supportedLanguages[index++] = Language("hi", _("Hindi"));
	_supportedLanguages[index++] = Language("ho", _("Hiri Motu"));
	_supportedLanguages[index++] = Language("hr", _("Croatian"));
	_supportedLanguages[index++] = Language("ht", _("Haitian"));
	_supportedLanguages[index++] = Language("hu", _("Hungarian"));
	_supportedLanguages[index++] = Language("hy", _("Armenian"));
	_supportedLanguages[index++] = Language("hz", _("Herero"));
	_supportedLanguages[index++] = Language("ia", _("Interlingua"));
	_supportedLanguages[index++] = Language("id", _("Indonesian"));
	_supportedLanguages[index++] = Language("ie", _("Interlingue; Occidental"));
	_supportedLanguages[index++] = Language("ig", _("Igbo"));
	_supportedLanguages[index++] = Language("ii", _("Sichuan Yi; Nuosu"));
	_supportedLanguages[index++] = Language("ik", _("Inupiaq"));
	_supportedLanguages[index++] = Language("io", _("Ido"));
	_supportedLanguages[index++] = Language("is", _("Icelandic"));
	_supportedLanguages[index++] = Language("it", _("Italian"));
	_supportedLanguages[index++] = Language("iu", _("Inuktitut"));
	_supportedLanguages[index++] = Language("ja", _("Japanese"));
	_supportedLanguages[index++] = Language("jv", _("Javanese"));
	_supportedLanguages[index++] = Language("ka", _("Georgian"));
	_supportedLanguages[index++] = Language("kg", _("Kongo"));
	_supportedLanguages[index++] = Language("ki", _("Kikuyu"));
	_supportedLanguages[index++] = Language("kj", _("Kuanyama"));
	_supportedLanguages[index++] = Language("kk", _("Kazakh"));
	_supportedLanguages[index++] = Language("kl", _("Kalaallisut; Greenlandic"));
	_supportedLanguages[index++] = Language("km", _("Central Khmer"));
	_supportedLanguages[index++] = Language("kn", _("Kannada"));
	_supportedLanguages[index++] = Language("ko", _("Korean"));
	_supportedLanguages[index++] = Language("kr", _("Kanuri"));
	_supportedLanguages[index++] = Language("ks", _("Kashmiri"));
	_supportedLanguages[index++] = Language("ku", _("Kurdish"));
	_supportedLanguages[index++] = Language("kv", _("Komi"));
	_supportedLanguages[index++] = Language("kw", _("Cornish"));
	_supportedLanguages[index++] = Language("ky", _("Kirghiz"));
	_supportedLanguages[index++] = Language("la", _("Latin"));
	_supportedLanguages[index++] = Language("lb", _("Luxembourgish"));
	_supportedLanguages[index++] = Language("lg", _("Ganda"));
	_supportedLanguages[index++] = Language("li", _("Limburgan"));
	_supportedLanguages[index++] = Language("ln", _("Lingala"));
	_supportedLanguages[index++] = Language("lo", _("Lao"));
	_supportedLanguages[index++] = Language("lt", _("Lithuanian"));
	_supportedLanguages[index++] = Language("lu", _("Luba-Katanga"));
	_supportedLanguages[index++] = Language("lv", _("Latvian"));
	_supportedLanguages[index++] = Language("mg", _("Malagasy"));
	_supportedLanguages[index++] = Language("mh", _("Marshallese"));
	_supportedLanguages[index++] = Language("mi", _("Maori"));
	_supportedLanguages[index++] = Language("mk", _("Macedonian"));
	_supportedLanguages[index++] = Language("ml", _("Malayalam"));
	_supportedLanguages[index++] = Language("mn", _("Mongolian"));
	_supportedLanguages[index++] = Language("mr", _("Marathi"));
	_supportedLanguages[index++] = Language("ms", _("Malay"));
	_supportedLanguages[index++] = Language("mt", _("Maltese"));
	_supportedLanguages[index++] = Language("my", _("Burmese"));
	_supportedLanguages[index++] = Language("na", _("Nauru"));
	_supportedLanguages[index++] = Language("nd", _("Ndebele, North"));
	_supportedLanguages[index++] = Language("ne", _("Nepali"));
	_supportedLanguages[index++] = Language("ng", _("Ndonga"));
	_supportedLanguages[index++] = Language("nl", _("Dutch"));
	_supportedLanguages[index++] = Language("no", _("Norwegian"));
	_supportedLanguages[index++] = Language("nr", _("Ndebele, South"));
	_supportedLanguages[index++] = Language("nv", _("Navajo"));
	_supportedLanguages[index++] = Language("ny", _("Chichewa"));
	_supportedLanguages[index++] = Language("oc", _("Occitan"));
	_supportedLanguages[index++] = Language("oj", _("Ojibwa"));
	_supportedLanguages[index++] = Language("om", _("Oromo"));
	_supportedLanguages[index++] = Language("or", _("Oriya"));
	_supportedLanguages[index++] = Language("os", _("Ossetian"));
	_supportedLanguages[index++] = Language("pa", _("Panjabi"));
	_supportedLanguages[index++] = Language("pi", _("Pali"));
	_supportedLanguages[index++] = Language("pl", _("Polish"));
	_supportedLanguages[index++] = Language("ps", _("Pushto"));
	_supportedLanguages[index++] = Language("pt", _("Portuguese"));
	_supportedLanguages[index++] = Language("qu", _("Quechua"));
	_supportedLanguages[index++] = Language("rm", _("Romansh"));
	_supportedLanguages[index++] = Language("rn", _("Rundi"));
	_supportedLanguages[index++] = Language("ro", _("Romanian"));
	_supportedLanguages[index++] = Language("ru", _("Russian"));
	_supportedLanguages[index++] = Language("rw", _("Kinyarwanda"));
	_supportedLanguages[index++] = Language("sa", _("Sanskrit"));
	_supportedLanguages[index++] = Language("sc", _("Sardinian"));
	_supportedLanguages[index++] = Language("sd", _("Sindhi"));
	_supportedLanguages[index++] = Language("se", _("Northern Sami"));
	_supportedLanguages[index++] = Language("sg", _("Sango"));
	_supportedLanguages[index++] = Language("si", _("Sinhala"));
	_supportedLanguages[index++] = Language("sk", _("Slovak"));
	_supportedLanguages[index++] = Language("sl", _("Slovenian"));
	_supportedLanguages[index++] = Language("sm", _("Samoan"));
	_supportedLanguages[index++] = Language("sn", _("Shona"));
	_supportedLanguages[index++] = Language("so", _("Somali"));
	_supportedLanguages[index++] = Language("sq", _("Albanian"));
	_supportedLanguages[index++] = Language("sr", _("Serbian"));
	_supportedLanguages[index++] = Language("ss", _("Swati"));
	_supportedLanguages[index++] = Language("st", _("Sotho, Southern"));
	_supportedLanguages[index++] = Language("su", _("Sundanese"));
	_supportedLanguages[index++] = Language("sv", _("Swedish"));
	_supportedLanguages[index++] = Language("sw", _("Swahili"));
	_supportedLanguages[index++] = Language("ta", _("Tamil"));
	_supportedLanguages[index++] = Language("te", _("Telugu"));
	_supportedLanguages[index++] = Language("tg", _("Tajik"));
	_supportedLanguages[index++] = Language("th", _("Thai"));
	_supportedLanguages[index++] = Language("ti", _("Tigrinya"));
	_supportedLanguages[index++] = Language("tk", _("Turkmen"));
	_supportedLanguages[index++] = Language("tl", _("Tagalog"));
	_supportedLanguages[index++] = Language("tn", _("Tswana"));
	_supportedLanguages[index++] = Language("to", _("Tonga"));
	_supportedLanguages[index++] = Language("tr", _("Turkish"));
	_supportedLanguages[index++] = Language("ts", _("Tsonga"));
	_supportedLanguages[index++] = Language("tt", _("Tatar"));
	_supportedLanguages[index++] = Language("tw", _("Twi"));
	_supportedLanguages[index++] = Language("ty", _("Tahitian"));
	_supportedLanguages[index++] = Language("ug", _("Uighur"));
	_supportedLanguages[index++] = Language("uk", _("Ukrainian"));
	_supportedLanguages[index++] = Language("ur", _("Urdu"));
	_supportedLanguages[index++] = Language("uz", _("Uzbek"));
	_supportedLanguages[index++] = Language("ve", _("Venda"));
	_supportedLanguages[index++] = Language("vi", _("Vietnamese"));
	_supportedLanguages[index++] = Language("vo", _("Volapuek"));
	_supportedLanguages[index++] = Language("wa", _("Walloon"));
	_supportedLanguages[index++] = Language("wo", _("Wolof"));
	_supportedLanguages[index++] = Language("xh", _("Xhosa"));
	_supportedLanguages[index++] = Language("yi", _("Yiddish"));
	_supportedLanguages[index++] = Language("yo", _("Yoruba"));
	_supportedLanguages[index++] = Language("za", _("Zhuang"));
	_supportedLanguages[index++] = Language("zh", _("Chinese"));
	_supportedLanguages[index++] = Language("zu", _("Zulu"));
}

int LanguageManager::getLanguageIndex(const std::string& languageCode)
{
	std::string code = boost::algorithm::to_lower_copy(languageCode);

	for (LanguageMap::const_iterator i = _supportedLanguages.begin();
		 i != _supportedLanguages.end(); ++i)
	{
		if (i->second.twoDigitCode == code)
		{
			return i->first;
		}
	}

	throw UnknownLanguageException("Unknown language: " + languageCode);
}

} // namespace
