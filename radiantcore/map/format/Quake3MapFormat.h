#pragma once

#include "imapformat.h"

namespace map
{

class Quake3MapFormatBase :
	public MapFormat
{
public:
	// RegisterableModule implementation
	virtual const StringSet& getDependencies() const override;
	virtual void initialiseModule(const IApplicationContext& ctx) override;
	virtual void shutdownModule() override;

    // Map reader is shared by Q3 and Q3 alternate
	virtual IMapReaderPtr getMapReader(IMapImportFilter& filter) const override;

	virtual bool allowInfoFileCreation() const override;
	virtual bool canLoad(std::istream& stream) const override;

protected:
    virtual std::shared_ptr<Quake3MapFormatBase> getSharedToThis() = 0;
};

class Quake3MapFormat :
	public Quake3MapFormatBase,
	public std::enable_shared_from_this<Quake3MapFormat>
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const override;

	virtual const std::string& getMapFormatName() const override;
	virtual const std::string& getGameType() const override;
	virtual IMapWriterPtr getMapWriter() const override;

protected:
    virtual std::shared_ptr<Quake3MapFormatBase> getSharedToThis() override
    {
        return shared_from_this();
    }
};

class Quake3AlternateMapFormat :
    public Quake3MapFormatBase,
    public std::enable_shared_from_this<Quake3AlternateMapFormat>
{
public:
    // RegisterableModule implementation
    virtual const std::string& getName() const override;

    virtual const std::string& getMapFormatName() const override;
    virtual const std::string& getGameType() const override;
    virtual IMapWriterPtr getMapWriter() const override;

protected:
    virtual std::shared_ptr<Quake3MapFormatBase> getSharedToThis() override
    {
        return shared_from_this();
    }
};

} // namespace
