/*
 * FTGL - OpenGL font library
 *
 * Copyright (c) 2001-2004 Henry Maddocks <ftgl@opengl.geek.nz>
 * Copyright (c) 2008 Sam Hocevar <sam@hocevar.net>
 * Copyright (c) 2008 Sean Morrison <learner@brlcad.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __ftgl__
#   warning This header is deprecated. Please use <FTGL/ftgl.h> from now.
#   include <FTGL/ftgl.h>
#endif

#ifndef __FTPolygonFont__
#define __FTPolygonFont__

#ifdef __cplusplus

#include <vector>

/**
 * FTPolygonFont is a specialisation of the FTFont class for handling
 * tesselated Polygon Mesh fonts
 *
 * @see     FTFont
 */
class FTGL_EXPORT FTPolygonFont : public FTFont
{
    public:
        /**
         * Open and read a font file. Sets Error flag.
         *
         * @param fontFilePath  font file path.
         */
        FTPolygonFont(const char* fontFilePath);

        /**
         * Open and read a font from a buffer in memory. Sets Error flag.
         * The buffer is owned by the client and is NOT copied by FTGL. The
         * pointer must be valid while using FTGL.
         *
         * @param pBufferBytes  the in-memory buffer
         * @param bufferSizeInBytes  the length of the buffer in bytes
         */
        FTPolygonFont(const unsigned char *pBufferBytes,
                      size_t bufferSizeInBytes);

        /**
         * Destructor
         */
        ~FTPolygonFont();


    protected:
        /**
         * Construct a glyph of the correct type.
         *
         * Clients must override the function and return their specialised
         * FTGlyph.
         *
         * @param slot  A FreeType glyph slot.
         * @return  An FT****Glyph or <code>null</code> on failure.
         */
        virtual FTGlyph* MakeGlyph(FT_GlyphSlot slot);
};

#define FTGLPolygonFont FTPolygonFont

#endif //__cplusplus

FTGL_BEGIN_C_DECLS

/**
 * Create a specialised FTGLfont object for handling tesselated polygon
 * mesh fonts.
 *
 * @param file  The font file name.
 * @return  An FTGLfont* object.
 *
 * @see  FTGLfont
 */
FTGL_EXPORT FTGLfont *ftglCreatePolygonFont(const char *file);

/**
 * Create a specialised FTGLfont object for handling tesselated polygon
 * mesh fonts from a buffer in memory. Sets Error flag. The buffer is owned
 * by the client and is NOT copied by FTGL. The pointer must be valid while
 * using FTGL.
 *
 * @param bytes  the in-memory buffer
 * @param len  the length of the buffer in bytes
 * @return  An FTGLfont* object.
 */
FTGL_EXPORT FTGLfont *ftglCreatePolygonFontFromMem(const unsigned char *bytes,
                                                   size_t len);

FTGL_END_C_DECLS

#endif  //  __FTPolygonFont__
