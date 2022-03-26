#pragma once

#include "igl.h"
#include "debugging/gl.h"

namespace render
{

// Encapsulates an openGL frame buffer object
class FrameBuffer
{
private:
    GLuint _fbo;
    std::size_t _width;
    std::size_t _height;
    GLuint _textureNumber;

    FrameBuffer() :
        _fbo(0),
        _width(0),
        _height(0),
        _textureNumber(0)
    {}

public:
    constexpr static std::size_t DefaultShadowMapSize = 1024 * 6;

    using Ptr = std::shared_ptr<FrameBuffer>;

    ~FrameBuffer()
    {
        glDeleteTextures(1, &_textureNumber);
        _textureNumber = 0;

        glDeleteFramebuffers(1, &_fbo);
        _fbo = 0;
    }

    std::size_t getWidth() const
    {
        return _width;
    }

    std::size_t getHeight() const
    {
        return _height;
    }

    void bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        debug::assertNoGlErrors();
    }

    void unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        debug::assertNoGlErrors();
    }

    GLuint getTextureNumber() const
    {
        return _textureNumber;
    }

    static Ptr CreateShadowMapBuffer(std::size_t size = DefaultShadowMapSize)
    {
        Ptr buffer(new FrameBuffer);

        // Generate an FBO and an image to attach to it
        glGenFramebuffers(1, &buffer->_fbo);
        glGenTextures(1, &buffer->_textureNumber);

        debug::assertNoGlErrors();

        glBindTexture(GL_TEXTURE_2D, buffer->_textureNumber);

        debug::assertNoGlErrors();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        debug::assertNoGlErrors();

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, 
            static_cast<GLsizei>(size), static_cast<GLsizei>(size),
            0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        debug::assertNoGlErrors();

        // Attach the texture to the FBO
        buffer->bind();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, buffer->_textureNumber, 0);

        debug::assertNoGlErrors();

        buffer->_width = size;
        buffer->_height = size;

        buffer->unbind();

        return buffer;
    }
};

}
