#pragma once

#include "i18n.h"
#include <memory>

namespace language
{

/**
 * Service class dispatching the getLocalisedString() lookup calls
 * to the attached localisation provider.
 */
class LanguageManager :
	public ILanguageManager
{
private:
	// For now, we only hold one provider instance
	ILocalisationProvider::Ptr _provider;

public:
	void registerProvider(const ILocalisationProvider::Ptr& instance) override;

	std::string getLocalisedString(const char* stringToLocalise) override;
};

} // namespace
