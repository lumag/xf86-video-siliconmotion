/*
Copyright (C) 2006 Dennis De Winter  All Rights Reserved.

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

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "smi.h"


static void
SMI_EXASync(ScreenPtr pScreen, int marker);

static Bool
SMI_PrepareCopy(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap,
		int xdir, int ydir, int alu, Pixel planemask);

static void
SMI_Copy(PixmapPtr pDstPixmap, int srcX, int srcY, int dstX, int dstY, int width, int height);

static void
SMI_DoneCopy(PixmapPtr pDstPixmap);

static Bool
SMI_PrepareSolid(PixmapPtr pPixmap, int alu, Pixel planemask, Pixel fg);

static void
SMI_Solid(PixmapPtr pPixmap, int x1, int y1, int x2, int y2);

static void
SMI_DoneSolid(PixmapPtr pPixmap);

Bool
SMI_UploadToScreen(PixmapPtr pDst, int x, int y, int w, int h, char *src, int src_pitch);

Bool
SMI_DownloadFromScreen(PixmapPtr pSrc, int x, int y, int w, int h, char *dst, int dst_pitch);

Bool
SMI_EXAInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    SMIPtr pSmi = SMIPTR(pScrn);
	
    ENTER_PROC("SMI_EXAInit");

    if (!(pSmi->EXADriverPtr = exaDriverAlloc())) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to allocate EXADriverRec.\n");
	LEAVE_PROC("SMI_EXAInit");
	return FALSE;
    }

    pSmi->EXADriverPtr->exa_major = 2;
    pSmi->EXADriverPtr->exa_minor = 0;

    SMI_EngineReset(pScrn);

    /* Memory Manager */
    pSmi->EXADriverPtr->memoryBase = pSmi->FBBase + pSmi->FBOffset;
    pSmi->EXADriverPtr->memorySize = pSmi->FBReserved - 1024;
    pSmi->EXADriverPtr->offScreenBase = pSmi->width * pSmi->height * pSmi->Bpp + 1024;

    /* Flags */
    pSmi->EXADriverPtr->flags = EXA_TWO_BITBLT_DIRECTIONS;
    if (pSmi->EXADriverPtr->memorySize > pSmi->EXADriverPtr->offScreenBase) {
	pSmi->EXADriverPtr->flags |= EXA_OFFSCREEN_PIXMAPS;
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
			"EXA offscreen memory manager enabled.\n");
    } else {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Not enough video RAM for EXA offscreen memory manager.\n");
    }

    /* Offscreen Pixmaps */
    if (pScrn->bitsPerPixel == 24) {
	pSmi->EXADriverPtr->maxX = 4096 / 3;

	if (pSmi->Chipset == SMI_LYNX) {
	    pSmi->EXADriverPtr->maxY = 4096 / 3;
	}
    } else {
	pSmi->EXADriverPtr->maxX = 4096;
	pSmi->EXADriverPtr->maxY = 4096;
    }

    pSmi->EXADriverPtr->pixmapPitchAlign  = 16;
    pSmi->EXADriverPtr->pixmapOffsetAlign = 16;

    /* Sync */
    pSmi->EXADriverPtr->WaitMarker = SMI_EXASync;

    /* Copy */
    pSmi->EXADriverPtr->PrepareCopy = SMI_PrepareCopy;
    pSmi->EXADriverPtr->Copy = SMI_Copy;
    pSmi->EXADriverPtr->DoneCopy = SMI_DoneCopy;

    /* Solid */
    pSmi->EXADriverPtr->PrepareSolid = SMI_PrepareSolid;
    pSmi->EXADriverPtr->Solid = SMI_Solid;
    pSmi->EXADriverPtr->DoneSolid = SMI_DoneSolid;

    /* DFS & UTS */
    pSmi->EXADriverPtr->UploadToScreen = SMI_UploadToScreen;
    pSmi->EXADriverPtr->DownloadFromScreen = SMI_DownloadFromScreen;

    if(!exaDriverInit(pScreen, pSmi->EXADriverPtr)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "exaDriverInit failed.\n");
	LEAVE_PROC("SMI_EXAInit");
	return FALSE;
    } else {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "EXA Acceleration enabled.\n");
	LEAVE_PROC("SMI_EXAInit");
	return TRUE;
    }
}

static void
SMI_EXASync(ScreenPtr pScreen, int marker)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    SMIPtr pSmi = SMIPTR(xf86Screens[pScreen->myNum]);

    ENTER_PROC("SMI_EXASync");

    WaitIdleEmpty();

    LEAVE_PROC("SMI_EXASync");
}

/* ----------------------------------------------------- EXA Copy ---------------------------------------------- */

CARD8 SMI_BltRop[16] =	/* table stolen from KAA */
{
    /* GXclear      */      0x00,         /* 0 */
    /* GXand        */      0x88,         /* src AND dst */
    /* GXandReverse */      0x44,         /* src AND NOT dst */
    /* GXcopy       */      0xCC,         /* src */
    /* GXandInverted*/      0x22,         /* NOT src AND dst */
    /* GXnoop       */      0xAA,         /* dst */
    /* GXxor        */      0x66,         /* src XOR dst */
    /* GXor         */      0xEE,         /* src OR dst */
    /* GXnor        */      0x11,         /* NOT src AND NOT dst */
    /* GXequiv      */      0x99,         /* NOT src XOR dst */
    /* GXinvert     */      0x55,         /* NOT dst */
    /* GXorReverse  */      0xDD,         /* src OR NOT dst */
    /* GXcopyInverted*/     0x33,         /* NOT src */
    /* GXorInverted */      0xBB,         /* NOT src OR dst */
    /* GXnand       */      0x77,         /* NOT src OR NOT dst */
    /* GXset        */      0xFF,         /* 1 */
};

static Bool
SMI_PrepareCopy(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap, int xdir, int ydir,
		int alu, Pixel planemask)
{
    ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
    SMIPtr pSmi = SMIPTR(pScrn);

    ENTER_PROC("SMI_PrepareCopy");
    DEBUG((VERBLEV, "xdir=%d ydir=%d alu=%02X", xdir, ydir, alu));

    /* calculate pitch in pixel unit */
    int src_pitch  = exaGetPixmapPitch(pSrcPixmap) / pSmi->Bpp;
    int dst_pitch  = exaGetPixmapPitch(pDstPixmap) / pSmi->Bpp;
    /* calculate offset in 8 byte (64 bit) unit */
    int src_offset = exaGetPixmapOffset(pSrcPixmap) / 8;
    int dst_offset = exaGetPixmapOffset(pDstPixmap) / 8;

    pSmi->AccelCmd = SMI_BltRop[alu]
		   | SMI_BITBLT
		   | SMI_START_ENGINE;

    if (ydir < 0 || (ydir == 0 && xdir < 0)) {
	pSmi->AccelCmd |= SMI_RIGHT_TO_LEFT;
    }

    WaitQueue(1);
    /* Destination and Source Window Widths */
    WRITE_DPR(pSmi, 0x3C, (dst_pitch << 16) | (src_pitch & 0xFFFF));

    if (pScrn->bitsPerPixel == 24) {
	/* Bit Mask not supported in 24 bit mode */
	pSmi->depth = pSrcPixmap->drawable.depth;
	if (!EXA_PM_IS_SOLID(pSmi, planemask)) {
	    LEAVE_PROC("SMI_PrepareCopy");
	    return FALSE;
	}

	src_pitch *= 3;
	dst_pitch *= 3;
	WaitQueue(3);
    } else {
	WaitQueue(4);
	/* Bit Mask (planemask) */
	WRITE_DPR(pSmi, 0x28, planemask);
    }
    /* Destination and Source Row Pitch */
    WRITE_DPR(pSmi, 0x10, (dst_pitch << 16) | (src_pitch & 0xFFFF));
    /* Destination and Source Base Address (offset) */
    WRITE_DPR(pSmi, 0x40, src_offset);
    WRITE_DPR(pSmi, 0x44, dst_offset);

    LEAVE_PROC("SMI_PrepareCopy");
    return TRUE;
}

static void
SMI_Copy(PixmapPtr pDstPixmap, int srcX, int srcY, int dstX,
	 int dstY, int width, int height)
{
    ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
    SMIPtr pSmi = SMIPTR(pScrn);

    ENTER_PROC("SMI_Copy");
    DEBUG((VERBLEV, "srcX=%d srcY=%d dstX=%d dstY=%d width=%d height=%d\n",
	   srcX, srcY, dstX, dstY, width, height));

    if (pSmi->AccelCmd & SMI_RIGHT_TO_LEFT) {
	srcX += width  - 1;
	srcY += height - 1;
	dstX += width  - 1;
	dstY += height - 1;
    }

    if (pScrn->bitsPerPixel == 24) {
	srcX  *= 3;
	dstX  *= 3;
	width *= 3;

    	if (pSmi->Chipset == SMI_LYNX) {
	    srcY *= 3;
	    dstY *= 3;
        }

	if (pSmi->AccelCmd & SMI_RIGHT_TO_LEFT) {
	    srcX += 2;
	    dstX += 2;
	}
    }

    WaitQueue(4);
    WRITE_DPR(pSmi, 0x00, (srcX  << 16) + (srcY & 0xFFFF));
    WRITE_DPR(pSmi, 0x04, (dstX  << 16) + (dstY & 0xFFFF));
    WRITE_DPR(pSmi, 0x08, (width << 16) + (height & 0xFFFF));
    WRITE_DPR(pSmi, 0x0C, pSmi->AccelCmd);

    LEAVE_PROC("SMI_Copy");
}

static void
SMI_DoneCopy(PixmapPtr pDstPixmap)
{
    ENTER_PROC("SMI_DoneCopy");

    LEAVE_PROC("SMI_DoneCopy");
}

/* ----------------------------------------------------- EXA Solid --------------------------------------------- */

CARD8 SMI_SolidRop[16] =	/* table stolen from KAA */
{
    /* GXclear      */      0x00,         /* 0 */
    /* GXand        */      0xA0,         /* src AND dst */
    /* GXandReverse */      0x50,         /* src AND NOT dst */
    /* GXcopy       */      0xF0,         /* src */
    /* GXandInverted*/      0x0A,         /* NOT src AND dst */
    /* GXnoop       */      0xAA,         /* dst */
    /* GXxor        */      0x5A,         /* src XOR dst */
    /* GXor         */      0xFA,         /* src OR dst */
    /* GXnor        */      0x05,         /* NOT src AND NOT dst */
    /* GXequiv      */      0xA5,         /* NOT src XOR dst */
    /* GXinvert     */      0x55,         /* NOT dst */
    /* GXorReverse  */      0xF5,         /* src OR NOT dst */
    /* GXcopyInverted*/     0x0F,         /* NOT src */
    /* GXorInverted */      0xAF,         /* NOT src OR dst */
    /* GXnand       */      0x5F,         /* NOT src OR NOT dst */
    /* GXset        */      0xFF,         /* 1 */
};

static Bool
SMI_PrepareSolid(PixmapPtr pPixmap, int alu, Pixel planemask, Pixel fg)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    SMIPtr pSmi = SMIPTR(pScrn);

    ENTER_PROC("SMI_PrepareSolid");
    DEBUG((VERBLEV, "alu=%02X\n", alu));

    /* calculate pitch in pixel unit */
    int pitch  = exaGetPixmapPitch(pPixmap) / pSmi->Bpp;
    /* calculate offset in 8 byte (64 bit) unit */
    int offset = exaGetPixmapOffset(pPixmap) / 8;

    pSmi->AccelCmd = SMI_SolidRop[alu]
		   | SMI_BITBLT
		   | SMI_START_ENGINE;

    WaitQueue(1);
    /* Destination Window Width */
    WRITE_DPR(pSmi, 0x3C, (pitch << 16));

    if (pScrn->bitsPerPixel == 24) {
	/* Bit Mask not supported in 24 bit mode */
	pSmi->depth = pPixmap->drawable.depth;
	if (!EXA_PM_IS_SOLID(pSmi, planemask)) {
	    LEAVE_PROC("SMI_PrepareSolid");
	    return FALSE;
	}

	pitch *= 3;
	WaitQueue(5);
    } else {
	WaitQueue(6);
	/* Bit Mask (planemask) */
	WRITE_DPR(pSmi, 0x28, planemask);
    }
    /* Destination Row Pitch */
    WRITE_DPR(pSmi, 0x12, pitch);
    /* Destination Base Address (offset) */
    WRITE_DPR(pSmi, 0x44, offset);
    /* Foreground Color */
    WRITE_DPR(pSmi, 0x14, fg);
    /* Mono Pattern High and Low */
    WRITE_DPR(pSmi, 0x34, 0xFFFFFFFF);
    WRITE_DPR(pSmi, 0x38, 0xFFFFFFFF);

    LEAVE_PROC("SMI_PrepareSolid");
    return TRUE;
}

static void
SMI_Solid(PixmapPtr pPixmap, int x1, int y1, int x2, int y2)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    SMIPtr pSmi = SMIPTR(pScrn);

    ENTER_PROC("SMI_Solid");
    DEBUG((VERBLEV, "x1=%d y1=%d x2=%d y2=%d\n", x1, y1, x2, y2));

    int w = (x2 - x1);
    int h = (y2 - y1);

    if (pScrn->bitsPerPixel == 24) {
	x1 *= 3;
	w  *= 3;

	if (pSmi->Chipset == SMI_LYNX) {
	    y1 *= 3;
	}
    }

    WaitQueue(3);
    WRITE_DPR(pSmi, 0x04, (x1 << 16) | (y1 & 0xFFFF));
    WRITE_DPR(pSmi, 0x08, (w  << 16) | (h  & 0xFFFF));
    WRITE_DPR(pSmi, 0x0C, pSmi->AccelCmd);

    LEAVE_PROC("SMI_Solid");
}

static void
SMI_DoneSolid(PixmapPtr pPixmap)
{
    ENTER_PROC("SMI_DoneSolid");

    LEAVE_PROC("SMI_DoneSolid");
}

/* --------------------------------------- EXA DFS & UTS ---------------------------------------- */

Bool
SMI_DownloadFromScreen(PixmapPtr pSrc, int x, int y, int w, int h,
		       char *dst, int dst_pitch)
{
    ScrnInfoPtr pScrn = xf86Screens[pSrc->drawable.pScreen->myNum];
    SMIPtr pSmi = SMIPTR(pScrn);

    ENTER_PROC("SMI_DownloadFromScreen");
    DEBUG((VERBLEV, "x=%d y=%d w=%d h=%d dst=%d dst_pitch=%d\n",
	   x, y, w, h, dst, dst_pitch));

    unsigned char *src = pSrc->devPrivate.ptr;
    int src_pitch = exaGetPixmapPitch(pSrc);

    exaWaitSync(pSrc->drawable.pScreen);

    src += (y * src_pitch) + (x * pSmi->Bpp);
    w   *= pSmi->Bpp;

    while (h--) {
	memcpy(dst, src, w);
	src += src_pitch;
	dst += dst_pitch;
    }

    return TRUE;
    LEAVE_PROC("SMI_DownloadFromScreen");
}

Bool
SMI_UploadToScreen(PixmapPtr pDst, int x, int y, int w, int h,
		   char *src, int src_pitch)
{
    ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
    SMIPtr pSmi = SMIPTR(pScrn);

    ENTER_PROC("SMI_UploadToScreen");
    DEBUG((VERBLEV, "x=%d y=%d w=%d h=%d src=%d src_pitch=%d\n",
	   x, y, w, h, src, src_pitch));

    char *dst = pDst->devPrivate.ptr;
    int dst_pitch = exaGetPixmapPitch(pDst);

    exaWaitSync(pDst->drawable.pScreen);

    dst += (y * dst_pitch) + (x * pSmi->Bpp);
    w   *= pSmi->Bpp;

    while (h--) {
	memcpy(dst, src, w);
	src += src_pitch;
	dst += dst_pitch;
    }

    return TRUE;
    LEAVE_PROC("SMI_UploadToScreen");
}

