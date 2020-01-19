#pragma once

#include "imapformat.h"

namespace map
{

class PortableMapFormat :
	public MapFormat,
	public std::enable_shared_from_this<PortableMapFormat>
{
public:
	// Format version, will be exported as <map> tag attribute
	static std::size_t VERSION;

	typedef std::shared_ptr<PortableMapFormat> Ptr;

	// RegisterableModule implementation
	virtual const std::string& getName() const override;
	virtual const StringSet& getDependencies() const override;
	virtual void initialiseModule(const ApplicationContext& ctx) override;
	virtual void shutdownModule() override;

	virtual const std::string& getMapFormatName() const override;
	virtual const std::string& getGameType() const override;
	virtual IMapReaderPtr getMapReader(IMapImportFilter& filter) const override;
	virtual IMapWriterPtr getMapWriter() const override;

	virtual bool allowInfoFileCreation() const override;

	virtual bool canLoad(std::istream& stream) const override;
};

} // namespace map
