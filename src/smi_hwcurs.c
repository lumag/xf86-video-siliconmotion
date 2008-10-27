/* Header:   //Mercury/Projects/archives/XFree86/4.0/smi_hwcurs.c-arc   1.12   27 Nov 2000 15:47:48   Frido  $ */

/*
Copyright (C) 1994-1999 The XFree86 Project, Inc.  All Rights Reserved.
Copyright (C) 2000 Silicon Motion, Inc.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FIT-
NESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the XFree86 Project and
Silicon Motion shall not be used in advertising or otherwise to promote the
sale, use or other dealings in this Software without prior written
authorization from the XFree86 Project and Silicon Motion.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cursorstr.h"
#include "smi.h"

static unsigned short
InterleaveBytes(int source, int mask)
{
    unsigned char	ibit;
    unsigned short	usWord = 0;
    unsigned char	ucBitMask = 0x01;

    /*
     * This function will interleave the bits in the source and mask bytes
     * to create a word that looks like this:
     *
     *    [M7 M6 M5 M4 M3 M2 M1 M0] [S7 S6 S5 S4 S3 S2 S1 S0]
     * Results in:
     *    [M7 S7 M6 S6 M5 S5 M4 S4 M3 S3 M2 S2 M1 S1 M0 S0]
     */

    for (ibit = 0; ibit < 8; ibit++) {
	usWord |= (source & ucBitMask) << ibit;
	usWord |= (mask & ucBitMask) << (ibit + 1);
	ucBitMask <<= 1;
    }

    return (usWord);
}

#if 0
/* From the SMI Windows CE driver */
static void
SMI501_RotateCursorShape(xf86CursorInfoPtr infoPtr, int angle,
			 unsigned char *pByte)
{
    BYTE		*pCursor;
    unsigned long	 ulBase, ulIndex;
    BYTE		 src[256], dst[256];	/* 128 = 8 x 32 */
    BYTE		 jMask, j, bitMask;
    int			 x, y, cx = 32, cy = 32;

    pCursor = pByte;

    memset (src, 0x00, sizeof (src));
    memset (dst, 0x00, sizeof (dst));

    /* Save the original pointer shape into local memory shapeRow[] */
    for (y = 0; y < cy; y++) {
	for (x = 0; x < cx / 4; x++)
	    src[y * 8 + x] = pByte[x];
	pByte += 16;
    }

    switch (angle) {
	case SMI_ROTATE_CCW:
	    for (y = 0; y < cy; y++) {
		jMask = 0x02 << ((y & 3) * 2);
		for (x = 0; x < cx; x++) {
		    j = src[y * 8 + x / 4];
		    bitMask = 0x02 << ((x & 3) * 2);

		    ulBase = (31 - x) * 8;
		    ulIndex = ulBase + y / 4;

		    if (j & bitMask)
			dst[ulIndex] |= jMask;
		    if (j & (bitMask >> 1))
			dst[ulIndex] |= jMask >> 1;
		}
	    }
	    break;

	case SMI_ROTATE_CW:
	    for (y = 0; y < cy; y++) {
		jMask = 0x80 >> ((y & 3) * 2);

		/* Write available bits into shapeRow */
		for (x = 0; x < cx; x++) {
		    j = src[y * 8 + x / 4];
		    bitMask = 0x02 << ((x & 3) * 2);

		    ulBase = x * 8;
		    ulIndex = ulBase + (31 - y) / 4;

		    if (j & bitMask)
			dst[ulIndex] |= jMask;
		    if (j & (bitMask >> 1))
			dst[ulIndex] |= jMask >> 1;
		}
	    }
	    break;
	default:
	    return;
    }

    for (y = 0; y < cy; y++) {
	for (x = 0; x < cx / 4; x++)
	    pCursor[x] = dst[y * 8 + x];
	pCursor += 16;
    }

}
#endif

static unsigned char *
SMI501_RealizeCursor(xf86CursorInfoPtr infoPtr, CursorPtr pCurs)
{
    CursorBitsPtr	 bits = pCurs->bits;
    unsigned char	*ram;
    unsigned short	*usram;
    unsigned char	*psource = bits->source;
    unsigned char	*pmask = bits->mask;
    int			 x, y, srcwidth, i;
    unsigned int	 MaxCursor;

    ENTER();

    /* Allocate memory */
    ram = (unsigned char *) xcalloc (1, SMI501_CURSOR_SIZE);

    usram = (unsigned short *) ram;
    MaxCursor = SMI501_MAX_CURSOR;

    if (ram == NULL)
	RETURN(NULL);

    /* Calculate cursor information */
    srcwidth = ((bits->width + 31) / 8) & ~3;

    i = 0;

    /* Copy cursor image */
    for (y = 0; y < min (MaxCursor, bits->height); y++) {
	for (x = 0; x < min (MaxCursor / 8, srcwidth); x++) {
	    unsigned char	mask = *pmask++;
	    unsigned char	source = *psource++ & mask;

	    usram[i++] = InterleaveBytes (source, mask);
	}

	pmask += srcwidth - x;
	psource += srcwidth - x;

	/* Fill remaining part of line with no shape */
	for (; x < MaxCursor / 8; x++)
	    usram[i++] = 0x0000;
    }

    /* Fill remaining part of memory with no shape */
    for (; y < MaxCursor; y++) {
	for (x = 0; x < MaxCursor / 8; x++)
	    usram[i++] = 0x0000;
    }

#if 0
    SMI501_RotateCursorShape(infoPtr, pSmi->rotate, ram);
#endif

    RETURN(ram);
}

static void
SMI_LoadCursorImage(ScrnInfoPtr pScrn, unsigned char *src)
{
    SMIPtr pSmi = SMIPTR(pScrn);

    ENTER();

    if (IS_MSOC(pSmi)) {
	/* Write address, disabling the HW cursor */
	if (!pSmi->IsSecondary) {
	    /* Panel HWC Addr */
	    WRITE_DCR(pSmi, 0x00f0, pSmi->FBCursorOffset);
	    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, VERBLEV,
			   "Primary FBCursorOffset at 0x%08X\n",
			   (unsigned int)pSmi->FBCursorOffset);
	}
	else {
	    /* CRT   HWC Addr */
	    WRITE_DCR(pSmi, 0x0230, pSmi->videoRAMBytes + pSmi->FBCursorOffset);
	    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, VERBLEV,
			   "Secondary FBCursorOffset at 0x%08X\n",
			   (unsigned int)pSmi->FBCursorOffset);
	}
    }

    /* Copy cursor image to framebuffer storage */
    memcpy(pSmi->FBBase + pSmi->FBCursorOffset, src, 1024);

    LEAVE();
}

static void
SMI_ShowCursor(ScrnInfoPtr pScrn)
{
    SMIPtr pSmi = SMIPTR(pScrn);

    ENTER();

    if (IS_MSOC(pSmi)) {
	CARD32	uiPanelTmp;
	CARD32	uiCrtTmp;

	if (!pSmi->IsSecondary) {
	    uiPanelTmp = READ_DCR(pSmi, 0x00f0);
	    uiPanelTmp |= SMI501_MASK_HWCENABLE;
	    WRITE_DCR(pSmi, 0x00f0, uiPanelTmp);
	}
	else {
	    uiCrtTmp = READ_DCR(pSmi, 0x0230);
	    uiCrtTmp |= SMI501_MASK_HWCENABLE;
	    WRITE_DCR(pSmi, 0x0230, uiCrtTmp);
	}
    }

    LEAVE();
}

static void
SMI_HideCursor(ScrnInfoPtr pScrn)
{
    SMIPtr pSmi = SMIPTR(pScrn);

    ENTER();

    if (IS_MSOC(pSmi)) {
	CARD32	uiPanelTmp;
	CARD32	uiCrtTmp;

	if (!pSmi->IsSecondary) {
	    uiPanelTmp = READ_DCR(pSmi, 0x00f0);
	    uiPanelTmp &= ~SMI501_MASK_HWCENABLE;
	    WRITE_DCR(pSmi, 0x00f0, uiPanelTmp);
	}
	else {
	    uiCrtTmp = READ_DCR(pSmi, 0x0230);
	    uiCrtTmp &= ~SMI501_MASK_HWCENABLE;
	    WRITE_DCR(pSmi, 0x0230, uiCrtTmp);
	}
    }

    LEAVE();
}

static void
SMI_SetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
    SMIPtr pSmi = SMIPTR(pScrn);
    int xoff, yoff;

    ENTER();

    /* Program coordinates */
    if (IS_MSOC(pSmi)) {
	CARD32 hwcLocVal;

	if (xoff >= 0)
	    hwcLocVal = xoff & SMI501_MASK_MAXBITS;
	else
	    hwcLocVal = (-xoff & SMI501_MASK_MAXBITS) | SMI501_MASK_BOUNDARY;

	if (yoff >= 0)
	    hwcLocVal |= (yoff & SMI501_MASK_MAXBITS) << 16;
	else
	    hwcLocVal |= ((-yoff & SMI501_MASK_MAXBITS) |
			  SMI501_MASK_BOUNDARY) << 16;

	/* Program combined coordinates */
	if (!pSmi->IsSecondary)
	    WRITE_DCR(pSmi, 0x00f4, hwcLocVal);		/* Panel HWC Location */
	else
	    WRITE_DCR(pSmi, 0x0234, hwcLocVal);		/* CRT   HWC Location */
    }

    LEAVE();
}

static void
SMI_SetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    SMIPtr pSmi = SMIPTR(pScrn);

    ENTER();

    if (IS_MSOC(pSmi)) {
	/* for the SMI501 HWCursor, there are 4 possible colors, one of which
	 * is transparent:  M,S:  0,0 = Transparent
	 *                                                0,1 = color 1
	 *                                                1,0 = color 2
	 *                                                1,1 = color 3
	 *  To simplify implementation, we use color2 == bg and
	 *                                                                         color3 == fg
	 *      Color 1 is don't care, so we set it to color 2's value
	 */
	unsigned int packedFGBG;

	/* Pack the true color components into 16 bit RGB -- 5:6:5 */
	packedFGBG = (bg & 0xF80000) >> 8
	    | (bg & 0x00FC00) >> 5 | (bg & 0x0000F8) >> 3;

	packedFGBG |= (bg & 0xF80000) << 8
	    | (bg & 0x00FC00) << 11 | (bg & 0x0000F8) << 13;

	if (!pSmi->IsSecondary)
	    WRITE_DCR(pSmi, 0x00f8, packedFGBG);	/* Panel HWC Color 1,2 */
	else
	    WRITE_DCR(pSmi, 0x0238, packedFGBG);	/* CRT  HWC Color 1,2 */


	packedFGBG = (fg & 0xF80000) >> 8
	    | (fg & 0x00FC00) >> 5 | (fg & 0x0000F8) >> 3;
	if (!pSmi->IsSecondary)
	    WRITE_DCR(pSmi, 0x00fc, packedFGBG);	/* Panel HWC Color 3 */
	else
	    WRITE_DCR(pSmi, 0x023c, packedFGBG);	/* CRT   HWC Color 3 */
    }

    LEAVE();
}

Bool
SMI_HWCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    SMIPtr pSmi = SMIPTR(pScrn);
    xf86CursorInfoPtr infoPtr;
    Bool ret;

    ENTER();

    /* Create cursor infor record */
    infoPtr = xf86CreateCursorInfoRec();
    if (infoPtr == NULL)
	RETURN(FALSE);

    pSmi->CursorInfoRec = infoPtr;

    /* Fill in the information */
    if (IS_MSOC(pSmi)) {
	infoPtr->MaxWidth  = SMI501_MAX_CURSOR;
	infoPtr->MaxHeight = SMI501_MAX_CURSOR;
	infoPtr->Flags	   = HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_1 |
			     HARDWARE_CURSOR_SWAP_SOURCE_AND_MASK;
	infoPtr->RealizeCursor = SMI501_RealizeCursor;
    }

    infoPtr->SetCursorColors   = SMI_SetCursorColors;
    infoPtr->SetCursorPosition = SMI_SetCursorPosition;
    infoPtr->LoadCursorImage   = SMI_LoadCursorImage;
    infoPtr->HideCursor        = SMI_HideCursor;
    infoPtr->ShowCursor        = SMI_ShowCursor;
    infoPtr->UseHWCursor       = NULL;

    /* Proceed with cursor initialization */
    ret = xf86InitCursor(pScreen, infoPtr);

    RETURN(ret);
}

