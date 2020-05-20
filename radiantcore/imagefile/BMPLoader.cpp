#include "BMPLoader.h"

#include "itextstream.h"
#include "ifilesystem.h"

typedef unsigned char byte;

#include "RGBAImage.h"
#include "stream/ScopedArchiveBuffer.h"
#include "stream/PointerInputStream.h"
#include "stream/utils.h"

namespace image
{

typedef unsigned char PaletteEntry[4];
typedef struct
{
    char id[2];
    unsigned long fileSize;
    unsigned long reserved0;
    unsigned long bitmapDataOffset;
    unsigned long bitmapHeaderSize;
    unsigned long width;
    unsigned long height;
    unsigned short planes;
    unsigned short bitsPerPixel;
    unsigned long compression;
    unsigned long bitmapDataSize;
    unsigned long hRes;
    unsigned long vRes;
    unsigned long colors;
    unsigned long importantColors;
    PaletteEntry palette[256];
} BMPHeader_t;

class ReadPixel8
{
    PaletteEntry* m_palette;
public:
    ReadPixel8(PaletteEntry* palette) : m_palette(palette)
    {
    }
    void operator()(stream::PointerInputStream& inputStream, byte*& pixbuf) const
    {
        byte palIndex;
        inputStream.read(&palIndex, 1);
        *pixbuf++ = m_palette[palIndex][2];
        *pixbuf++ = m_palette[palIndex][1];
        *pixbuf++ = m_palette[palIndex][0];
        *pixbuf++ = 0xff;
    }
};

class ReadPixel16
{
public:
    void operator()(stream::PointerInputStream& inputStream, byte*& pixbuf) const
    {
        unsigned short shortPixel;
        inputStream.read(reinterpret_cast<byte*>(&shortPixel), sizeof(unsigned short)); //!\todo Is this endian safe?
        *pixbuf++ = static_cast<byte>(shortPixel & (31 << 10)) >> 7;
        *pixbuf++ = static_cast<byte>(shortPixel & (31 << 5)) >> 2;
        *pixbuf++ = static_cast<byte>(shortPixel & (31)) << 3;
        *pixbuf++ = 0xff;
    }
};

class ReadPixel24
{
public:
    void operator()(stream::PointerInputStream& inputStream, byte*& pixbuf) const
    {
        byte bgr[3];
        inputStream.read(bgr, 3);
        *pixbuf++ = bgr[2];
        *pixbuf++ = bgr[1];
        *pixbuf++ = bgr[0];
        *pixbuf++ = 255;
    }
};

class ReadPixel32
{
public:
    void operator()(stream::PointerInputStream& inputStream, byte*& pixbuf) const
    {
        byte bgra[4];
        inputStream.read(bgra, 4);
        *pixbuf++ = bgra[2];
        *pixbuf++ = bgra[1];
        *pixbuf++ = bgra[0];
        *pixbuf++ = bgra[3];
    }
};

template<typename ReadPixel>
void ReadBMP(stream::PointerInputStream& inputStream, byte* bmpRGBA, int rows, int columns, ReadPixel readPixel)
{
    for (int row = rows - 1; row >= 0; row--)
    {
        byte* pixbuf = bmpRGBA + row * columns * 4;

        for (int column = 0; column < columns; column++)
        {
            readPixel(inputStream, pixbuf);
        }
    }
}

RGBAImagePtr LoadBMPBuff(stream::PointerInputStream& inputStream, std::size_t length)
{
    BMPHeader_t bmpHeader;
    inputStream.read(reinterpret_cast<byte*>(bmpHeader.id), 2);
    bmpHeader.fileSize = stream::readLittleEndian<uint32_t>(inputStream);
    bmpHeader.reserved0 = stream::readLittleEndian<uint32_t>(inputStream);
    bmpHeader.bitmapDataOffset = stream::readLittleEndian<uint32_t>(inputStream);
    bmpHeader.bitmapHeaderSize = stream::readLittleEndian<uint32_t>(inputStream);
    bmpHeader.width = stream::readLittleEndian<uint32_t>(inputStream);
    bmpHeader.height = stream::readLittleEndian<uint32_t>(inputStream);
    bmpHeader.planes = stream::readLittleEndian<uint16_t>(inputStream);
    bmpHeader.bitsPerPixel = stream::readLittleEndian<uint16_t>(inputStream);
    bmpHeader.compression = stream::readLittleEndian<uint32_t>(inputStream);
    bmpHeader.bitmapDataSize = stream::readLittleEndian<uint32_t>(inputStream);
    bmpHeader.hRes = stream::readLittleEndian<uint32_t>(inputStream);
    bmpHeader.vRes = stream::readLittleEndian<uint32_t>(inputStream);
    bmpHeader.colors = stream::readLittleEndian<uint32_t>(inputStream);
    bmpHeader.importantColors = stream::readLittleEndian<uint32_t>(inputStream);

    if (bmpHeader.bitsPerPixel == 8)
    {
        int paletteSize = bmpHeader.colors * 4;
        inputStream.read(reinterpret_cast<byte*>(bmpHeader.palette), paletteSize);
    }

    if (bmpHeader.id[0] != 'B' && bmpHeader.id[1] != 'M')
    {
        rError() << "LoadBMP: only Windows-style BMP files supported\n";
        return RGBAImagePtr();
    }
    if (bmpHeader.fileSize != length)
    {
        rError() << "LoadBMP: header size does not match file size (" << bmpHeader.fileSize << " vs. " << length << ")\n";
        return RGBAImagePtr();
    }
    if (bmpHeader.compression != 0)
    {
        rError() << "LoadBMP: only uncompressed BMP files supported\n";
        return RGBAImagePtr();
    }
    if (bmpHeader.bitsPerPixel < 8)
    {
        rError() << "LoadBMP: monochrome and 4-bit BMP files not supported\n";
        return RGBAImagePtr();
    }

    int columns = bmpHeader.width;
    int rows = bmpHeader.height;
    if (rows < 0)
        rows = -rows;

    RGBAImagePtr image(new RGBAImage(columns, rows));

    switch (bmpHeader.bitsPerPixel)
    {
    case 8:
        ReadBMP(inputStream, image->getMipMapPixels(0), rows, columns, ReadPixel8(bmpHeader.palette));
        break;
    case 16:
        ReadBMP(inputStream, image->getMipMapPixels(0), rows, columns, ReadPixel16());
        break;
    case 24:
        ReadBMP(inputStream, image->getMipMapPixels(0), rows, columns, ReadPixel24());
        break;
    case 32:
        ReadBMP(inputStream, image->getMipMapPixels(0), rows, columns, ReadPixel32());
        break;
    default:
        rError() << "LoadBMP: illegal pixel_size '" << bmpHeader.bitsPerPixel << "'\n";
        return RGBAImagePtr();
    }
    return image;
}

ImagePtr BMPLoader::load(ArchiveFile& file) const
{
    archive::ScopedArchiveBuffer buffer(file);

    stream::PointerInputStream inputStream(buffer.buffer);
    return LoadBMPBuff(inputStream, buffer.length);
}

ImageTypeLoader::Extensions BMPLoader::getExtensions() const
{
    Extensions extensions;
    extensions.push_back("bmp");
    return extensions;
}

}
