/* -----------------------------------------------------------------------------

DDS Library 

Based on code from Nvidia's DDS example:
http://www.nvidia.com/object/dxtc_decompression_code.html

Copyright (c) 2003 Randy Reddig
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

Neither the names of the copyright holders nor the names of its contributors may
be used to endorse or promote products derived from this software without
specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------------- */



/* marker */
#ifndef DDSLIB_H
#define DDSLIB_H



/* dependencies */
#include <stdio.h>
#include <memory.h>



/* c++ marker */
#ifdef __cplusplus
extern "C"
{
#endif



/* dds definition */
typedef enum
{
	DDS_PF_ARGB8888,
	DDS_PF_DXT1,
	DDS_PF_DXT2,
	DDS_PF_DXT3,
	DDS_PF_DXT4,
	DDS_PF_DXT5,
	DDS_PF_DXT5_RXGB,	/* Doom 3's swizzled format */
	DDS_PF_UNKNOWN
}
ddsPF_t;

// greebo: General parameters which depend on the DDS format
/*struct DDSLoadInfo
{
	unsigned int divSize = 4;
	unsigned int blockBytes;
	GLenum internalFormat;
};

DDSLoadInfo loadInfoDXT1 = {
      8, GL_COMPRESSED_RGBA_S3TC_DXT1
};
DdsLoadInfo loadInfoDXT3 = {
      16, GL_COMPRESSED_RGBA_S3TC_DXT3
};
DdsLoadInfo loadInfoDXT5 = {
      16, GL_COMPRESSED_RGBA_S3TC_DXT5
};*/

/* 16bpp stuff */
#define DDS_LOW_5		0x001F;
#define DDS_MID_6		0x07E0;
#define DDS_HIGH_5		0xF800;
#define DDS_MID_555		0x03E0;
#define DDS_HI_555		0x7C00;


/* structures */
typedef struct ddsColorKey_s
{
	unsigned int		colorSpaceLowValue;
	unsigned int		colorSpaceHighValue;
} 
ddsColorKey_t;


typedef struct ddsCaps_s
{
	unsigned int		caps1;
	unsigned int		caps2;
	unsigned int		caps3;
	unsigned int		caps4;
} 
ddsCaps_t;


typedef struct ddsMultiSampleCaps_s
{
	unsigned short		flipMSTypes;
	unsigned short		bltMSTypes;
}
ddsMultiSampleCaps_t;


typedef struct ddsPixelFormat_s
{
	unsigned int		size;
	unsigned int		flags;
	unsigned char		fourCC[4];
	union
	{
		unsigned int	rgbBitCount;
		unsigned int	yuvBitCount;
		unsigned int	zBufferBitDepth;
		unsigned int	alphaBitDepth;
		unsigned int	luminanceBitCount;
		unsigned int	bumpBitCount;
		unsigned int	privateFormatBitCount;
	};
	union
	{
		unsigned int	rBitMask;
		unsigned int	yBitMask;
		unsigned int	stencilBitDepth;
		unsigned int	luminanceBitMask;
		unsigned int	bumpDuBitMask;
		unsigned int	operations;
	};
	union
	{
		unsigned int	gBitMask;
		unsigned int	uBitMask;
		unsigned int	zBitMask;
		unsigned int	bumpDvBitMask;
		ddsMultiSampleCaps_t	multiSampleCaps;
	};
	union
	{
		unsigned int	bBitMask;
		unsigned int	vBitMask;
		unsigned int	stencilBitMask;
		unsigned int	bumpLuminanceBitMask;
	};
	union
	{
		unsigned int	rgbAlphaBitMask;
		unsigned int	yuvAlphaBitMask;
		unsigned int	luminanceAlphaBitMask;
		unsigned int	rgbZBitMask;
		unsigned int	yuvZBitMask;
	};
}
ddsPixelFormat_t;

//  DDS header flags
#define DDSD_CAPS         0x00000001 
#define DDSD_HEIGHT       0x00000002 
#define DDSD_WIDTH        0x00000004 
#define DDSD_PITCH        0x00000008 
#define DDSD_PIXELFORMAT  0x00001000 
#define DDSD_MIPMAPCOUNT  0x00020000 
#define DDSD_LINEARSIZE   0x00080000 
#define DDSD_DEPTH        0x00800000 

struct DDSHeader
{
	/* magic: 'dds ' */
	char				magic[ 4 ];
	
	/* directdraw surface */
	unsigned int		size;
	unsigned int		flags;
	unsigned int		height;
	unsigned int		width; 
	union
	{
		int				pitch;
		unsigned int	linearSize;
	};
	unsigned int		backBufferCount;
	union
	{
		unsigned int	mipMapCount;
		unsigned int	refreshRate;
		unsigned int	srcVBHandle;
	};
	unsigned int		alphaBitDepth;
	unsigned int		reserved;
	unsigned int		surface;	// greebo: Changed this to unsigned int for 64-bit compatibility (should be 32 bits wide)
	union
	{
		ddsColorKey_t	ckDestOverlay;   
		unsigned int	emptyFaceColor;
	};
	ddsColorKey_t		ckDestBlt;
	ddsColorKey_t		ckSrcOverlay;    
	ddsColorKey_t		ckSrcBlt;     
	ddsPixelFormat_t	pixelFormat;
	ddsCaps_t			ddsCaps;
	unsigned int		textureStage;
};

typedef struct ddsBuffer_s
{
	DDSHeader			header;
	
	/* data (Varying size) */
	unsigned char		data[ 4 ];
}
ddsBuffer_t;

/** greebo: This represents a 64 bit DDS block containing
 * 			either the alpha or the colour data. 
 */
typedef struct ddsColorBlock_s
{
	unsigned short		colors[ 2 ];// 32 bit (2 x 16 bit)
	unsigned char		row[ 4 ];	// 32 bit (4 x 8 bit)
}
ddsColorBlock_t;

typedef struct ddsAlphaBlockExplicit_s
{
	unsigned short		row[ 4 ];
}
ddsAlphaBlockExplicit_t;


typedef struct ddsAlphaBlock3BitLinear_s
{
	unsigned char		alpha0;
	unsigned char		alpha1;
	unsigned char		stuff[ 6 ];
}
ddsAlphaBlock3BitLinear_t;


typedef struct ddsColor_s
{
	unsigned char		r, g, b, a;
}
ddsColor_t;



/* public functions */
int						DDSGetInfo( const DDSHeader* header, int *width, int *height, ddsPF_t *pf );
int						DDSDecompress( const DDSHeader* header, const unsigned char* buffer, unsigned char *pixels );



/* end marker */
#ifdef __cplusplus
}
#endif

#endif
