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

#pragma once

#include <cstdint>

/* dds definition */
enum ddsPF_t
{
    DDS_PF_ARGB8888,
    DDS_PF_DXT1,
    DDS_PF_DXT2,
    DDS_PF_DXT3,
    DDS_PF_DXT4,
    DDS_PF_DXT5,
    DDS_PF_DXT5_RXGB,   /* Doom 3's swizzled format */
    DDS_PF_UNKNOWN
};

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
#define DDS_LOW_5       0x001F;
#define DDS_MID_6       0x07E0;
#define DDS_HIGH_5      0xF800;
#define DDS_MID_555     0x03E0;
#define DDS_HI_555      0x7C00;


/* structures */
struct ddsColorKey_t
{
    unsigned int        colorSpaceLowValue;
    unsigned int        colorSpaceHighValue;
};

struct ddsCaps_t
{
    unsigned int        caps1;
    unsigned int        caps2;
    unsigned int        caps3;
    unsigned int        caps4;
};

struct ddsMultiSampleCaps_t
{
    unsigned short      flipMSTypes;
    unsigned short      bltMSTypes;
};

struct ddsPixelFormat_t
{
    unsigned int        size;
    unsigned int        flags;
    unsigned char       fourCC[4];
    union
    {
        unsigned int    rgbBitCount;
        unsigned int    yuvBitCount;
        unsigned int    zBufferBitDepth;
        unsigned int    alphaBitDepth;
        unsigned int    luminanceBitCount;
        unsigned int    bumpBitCount;
        unsigned int    privateFormatBitCount;
    };
    union
    {
        unsigned int    rBitMask;
        unsigned int    yBitMask;
        unsigned int    stencilBitDepth;
        unsigned int    luminanceBitMask;
        unsigned int    bumpDuBitMask;
        unsigned int    operations;
    };
    union
    {
        unsigned int    gBitMask;
        unsigned int    uBitMask;
        unsigned int    zBitMask;
        unsigned int    bumpDvBitMask;
        ddsMultiSampleCaps_t    multiSampleCaps;
    };
    union
    {
        unsigned int    bBitMask;
        unsigned int    vBitMask;
        unsigned int    stencilBitMask;
        unsigned int    bumpLuminanceBitMask;
    };
    union
    {
        unsigned int    rgbAlphaBitMask;
        unsigned int    yuvAlphaBitMask;
        unsigned int    luminanceAlphaBitMask;
        unsigned int    rgbZBitMask;
        unsigned int    yuvZBitMask;
    };
};

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
    char                magic[ 4 ];

    /* directdraw surface */
    uint32_t            size;
    uint32_t            flags;
    uint32_t            height;
    uint32_t            width;
    union
    {
        int32_t         pitch;
        uint32_t        linearSize;
    };
    uint32_t            backBufferCount;
    union
    {
        uint32_t        mipMapCount;
        uint32_t        refreshRate;
        uint32_t        srcVBHandle;
    };
    uint32_t            alphaBitDepth;
    uint32_t            reserved;
    uint32_t            surface;
    union
    {
        ddsColorKey_t   ckDestOverlay;
        uint32_t        emptyFaceColor;
    };
    ddsColorKey_t       ckDestBlt;
    ddsColorKey_t       ckSrcOverlay;
    ddsColorKey_t       ckSrcBlt;
    ddsPixelFormat_t    pixelFormat;
    ddsCaps_t           ddsCaps;
    uint32_t            textureStage;
};

struct ddsBuffer_t
{
    DDSHeader           header;

    /* data (Varying size) */
    unsigned char       data[ 4 ];
};

/** greebo: This represents a 64 bit DDS block containing
 *          either the alpha or the colour data.
 */
struct ddsColorBlock_t
{
    unsigned short      colors[ 2 ];// 32 bit (2 x 16 bit)
    unsigned char       row[ 4 ];   // 32 bit (4 x 8 bit)
};

struct ddsAlphaBlockExplicit_t
{
    unsigned short      row[ 4 ];
};

struct ddsAlphaBlock3BitLinear_t
{
    unsigned char       alpha0;
    unsigned char       alpha1;
    unsigned char       stuff[ 6 ];
};

struct ddsColor_t
{
    unsigned char       r, g, b, a;
};

/* public functions */
int DDSGetInfo( const DDSHeader* header, int *width, int *height, ddsPF_t *pf );
int DDSDecompress( const DDSHeader* header, const unsigned char* buffer, unsigned char *pixels );
