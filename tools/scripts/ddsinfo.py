#!/usr/bin/python
import sys, struct

# Lookup tables for DDS flags

DDSD_FLAG_TABLE = \
{
'DDSD_CAPS'			: 0x1,
'DDSD_HEIGHT'		: 0x2,
'DDSD_WIDTH'		: 0x4,
'DDSD_PITCH'		: 0x8,
'DDSD_PIXELFORMAT'	: 0x1000,
'DDSD_MIPMAPCOUNT'	: 0x20000,
'DDSD_LINEARSIZE'	: 0x80000,
'DDSD_DEPTH'		: 0x800000
}

DDPF_FLAG_TABLE = \
{
'DDPF_ALPHAPIXELS'	: 0x1,
'DDPF_FOURCC'		: 0x4,
'DDPF_RGB'			: 0x40
}

# -----------------------------------------------------------------------------
# Main script

inFile = open(sys.argv[1])

# Read and unpack the header
rawData = inFile.read(32)
(	sMagic, 
	dwSize, 
	dwFlags, 
	dwHeight, 
	dwWidth,
	dwPitchOrLinearSize,
	dwDepth,
	dwMipMapCount
) = \
	struct.unpack('4s I I I I I I I', rawData)

# Must have magic header
if (sMagic != 'DDS ' or dwSize != 124):
	print "Not a valid DDS file."
	sys.exit(1)

# Print out header information
print "DDS file, %dx%d" % (dwWidth, dwHeight)

if (dwFlags & DDSD_FLAG_TABLE['DDSD_LINEARSIZE']):
	print "\t%d bytes" % dwPitchOrLinearSize

if (dwFlags & DDSD_FLAG_TABLE['DDSD_MIPMAPCOUNT']):
	print "\t%d mipmaps" % dwMipMapCount

# Discard the 11 reserved DWORDS
inFile.read(11 * 4)

# Read the pixel format
rawPixelFormat = inFile.read(32)
(
	dwSize,
	dwFlags,
	sFourCC,
	dwRGBBitCount,
	dwRBitMask,
	dwGBitMask,
	dwBBitMask,
	dwRGBAlphaBitMask
) = \
	struct.unpack('2I 4s 5I', rawPixelFormat)

# Print out pixel format information
if (DDPF_FLAG_TABLE['DDPF_FOURCC'] & dwFlags):
	print "\t%s" % sFourCC

# Close file
inFile.close()


