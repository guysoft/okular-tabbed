/*
 * Copyright (c) 1994 Paul Vojta.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * NOTE:
 *	xdvi is based on prior work as noted in the modification history, below.
 */

/*
 * DVI previewer for X.
 *
 * Eric Cooper, CMU, September 1985.
 *
 * Code derived from dvi-imagen.c.
 *
 * Modification history:
 * 1/1986	Modified for X.10	--Bob Scheifler, MIT LCS.
 * 7/1988	Modified for X.11	--Mark Eichin, MIT
 * 12/1988	Added 'R' option, toolkit, magnifying glass
 *					--Paul Vojta, UC Berkeley.
 * 2/1989	Added tpic support	--Jeffrey Lee, U of Toronto
 * 4/1989	Modified for System V	--Donald Richardson, Clarkson Univ.
 * 3/1990	Added VMS support	--Scott Allendorf, U of Iowa
 * 7/1990	Added reflection mode	--Michael Pak, Hebrew U of Jerusalem
 * 1/1992	Added greyscale code	--Till Brychcy, Techn. Univ. Muenchen
 *					  and Lee Hetherington, MIT
 * 4/1994	Added DPS support, bounding box
 *					--Ricardo Telichevesky
 *					  and Luis Miguel Silveira, MIT RLE.
 */

#include <stdlib.h>

#include <kpathsea/config.h>
#include <kpathsea/c-ctype.h>
#include <kpathsea/c-fopen.h>
#include <kpathsea/c-vararg.h>
#include "glyph.h"
#include "oconfig.h"


glyph::~glyph()
{
  if (bitmap.bits != NULL)
    free(bitmap.bits);
  clearShrunkCharacter();
}

void glyph::clearShrunkCharacter()
{
  if (SmallChar) {
    delete SmallChar;
    SmallChar = NULL;
  }
}

// This method returns the SmallChar of the glyph-class, if it exists.
// If not, it is generated by shrinking the bitmap according to the
// shrink_factor.
// TODO: It would be nice to improve the rendering so that it does not use
// unintuitive "Shrink-Factors" anymore.

QPixmap glyph::shrunkCharacter()
{
  if (SmallChar == NULL) {
    // I do not really understand what's going on here.
    // Could anybody please comment this?
    // -- Stefan Kebekus.

    // These machinations ensure that the character is shrunk according to
    // its hot point, rather than its upper left-hand corner.
    int rows, init_cols,cols;
    x2 = x / shrink_factor;
    init_cols = x - x2 * shrink_factor;
    if (init_cols <= 0)
      init_cols += shrink_factor;
    else
      ++x2;

    y2 = y / shrink_factor;
    rows = y - y2 * shrink_factor;
    if (rows <= 0) 
      rows += shrink_factor;
    else
      ++y2;


#define ROUNDUP(x,y) (((x)+(y)-1)/(y)) 

    int shrunk_height = y2 + ROUNDUP(((int) bitmap.h - y) , shrink_factor);
    int shrunk_width  = x2 + (((int) bitmap.w - x) / shrink_factor) + 1;

    QBitmap bm(bitmap.bytes_wide*8, (int)bitmap.h, (const uchar *)(bitmap.bits) ,TRUE);
    // The intermediate Pixmap pm is taken to be slightly too large. 
    // After the smoothscale(), this gives lighter characters which are easier to read.
    SmallChar= new QPixmap(bitmap.w+2*(shrink_factor/3), bitmap.h+2*(shrink_factor/3));
    

    // The rendering here is probably not optimally fast. 
    // Please improve.
    // -- Stefan Kebekus
    QPainter paint(SmallChar);
    paint.setBackgroundColor(Qt::white);
    paint.setPen( Qt::black );
    paint.fillRect(0,0,bitmap.w+10, bitmap.h+10, Qt::white);
    paint.drawPixmap(shrink_factor/3,shrink_factor/3,bm);
    paint.end();
    
    // Generate an Image and shrink it to the proper size. By the documentation
    // of smoothScale, the resulting Image will be 8-bit.
    QImage im = SmallChar->convertToImage().smoothScale(shrunk_width, shrunk_height);
    // Generate the alpha-channel. This is probably highly inefficient.
    // Would anybody please produce a faster routine?
    QImage im32 = im.convertDepth(32);
    im32.setAlphaBuffer(TRUE);
    for(int y=0; y<im.height(); y++) {
      QRgb *imag_scanline = (QRgb *)im32.scanLine(y);
      for(int x=0; x<im.width(); x++) {
	// Make White => Transaparent
	if ((0x00ffffff & *imag_scanline) == 0x00ffffff)
	  *imag_scanline &= 0x00ffffff;
	else
	  *imag_scanline |= 0xff000000;
	imag_scanline++; // Disgusting pointer arithmetic. Should be forbidden.
      }
    }
    SmallChar->convertFromImage(im32,0);
    // TODO: throw execption if SmallChar is empty
  }
  return *SmallChar; 
}
