/*
Copyright (C) 2008 Francisco Jerez.  All Rights Reserved.

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
#include "smi_crtc.h"
#include "smilynx.h"
#include "smi_501.h"

static void
SMI_CrtcDPMS(xf86CrtcPtr		crtc,
	     int		    	mode)
{
    ENTER();

    /* Nothing */

    LEAVE();
}

static Bool
SMI_CrtcLock (xf86CrtcPtr crtc)
{
    ScrnInfoPtr pScrn = crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);

    ENTER();

    WaitIdle();

    RETURN(FALSE);
}

static void
SMI_CrtcUnlock (xf86CrtcPtr crtc)
{
    ENTER();

    /* Nothing */

    LEAVE();
}

static Bool
SMI_CrtcModeFixup(xf86CrtcPtr crtc,
		  DisplayModePtr mode,
		  DisplayModePtr adjusted_mode)
{
    ENTER();

    /* Nothing */

    RETURN(TRUE);
}

static void
SMI_CrtcPrepare(xf86CrtcPtr crtc)
{
    ScrnInfoPtr pScrn = crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);

    ENTER();

    if (!pSmi->Dualhead && pSmi->HwCursor)
	crtc->funcs->hide_cursor(crtc);

    LEAVE();
}

static void
SMI_CrtcCommit(xf86CrtcPtr crtc)
{
    ScrnInfoPtr pScrn = crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);

    ENTER();

    /* Problem:
     *	When starting in Dualhead mode, both hw cursors will be shown,
     *	and at the same position, as both are at the same address.
     *	When reconfiguring with something like:
     *	$ xrandr --output VGA --right-of LVDS
     *	what will happen is basically:
     *	hide_cursor(panel)
     *	hide_cursor(crt)
     *	<set-crt-mode-and-modify-mapped-address>
     *	show_cursor(panel)	<- besides a sw argb cursor is being used...
     *
     *	It should not be a problem if argb cursors were supported,
     *	or only one output is available...
     */
    if (!pSmi->Dualhead && pSmi->HwCursor)
	xf86_reload_cursors(pScrn->pScreen);

    LEAVE();
}

static void
SMI_CrtcGammaSet(xf86CrtcPtr crtc, CARD16 *red, CARD16 *green, CARD16 *blue,
	     int size)
{
    SMICrtcPrivatePtr crtcPriv = SMICRTC(crtc);
    int i;

    ENTER();

    for(i=0; i<256; i++){
	crtcPriv->lut_r[i] = red[i * size >> 8];
	crtcPriv->lut_g[i] = green[i * size >> 8];
	crtcPriv->lut_b[i] = blue[i * size >> 8];
    }

    crtcPriv->load_lut(crtc);

    LEAVE();
}

static void *
SMI_CrtcShadowAllocate (xf86CrtcPtr crtc, int width, int height)
{
    ScrnInfoPtr pScrn = crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    SMICrtcPrivatePtr crtcPriv = SMICRTC(crtc);

    ENTER();

    if(!pSmi->NoAccel && pSmi->useEXA){
	int aligned_pitch = (width * pSmi->Bpp + 15) & ~15;

	crtcPriv->shadowArea = exaOffscreenAlloc(pScrn->pScreen, aligned_pitch * height, 16, TRUE, NULL, NULL);

	if(crtcPriv->shadowArea)
	    RETURN(pSmi->FBBase + crtcPriv->shadowArea->offset);
    }

    RETURN(NULL);
}

static PixmapPtr
SMI_CrtcShadowCreate (xf86CrtcPtr crtc, void *data, int width, int height)
{
    ScrnInfoPtr pScrn = crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    int aligned_pitch;

    ENTER();

    aligned_pitch = (width * pSmi->Bpp + 15) & ~15;

    RETURN(GetScratchPixmapHeader(pScrn->pScreen,width,height,pScrn->depth,
				  pScrn->bitsPerPixel,aligned_pitch,data));
}

static void
SMI_CrtcShadowDestroy (xf86CrtcPtr crtc, PixmapPtr pPixmap, void *data)
{
    ScrnInfoPtr pScrn = crtc->scrn;
    SMICrtcPrivatePtr crtcPriv = SMICRTC(crtc);

    ENTER();

    if(pPixmap)
	FreeScratchPixmapHeader(pPixmap);

    if(crtcPriv->shadowArea){
	exaOffscreenFree(pScrn->pScreen, crtcPriv->shadowArea);
	crtcPriv->shadowArea = NULL;
    }

    LEAVE();
}

static void
SMI_CrtcDestroy (xf86CrtcPtr	crtc)
{
    ENTER();

    /* Nothing */

    LEAVE();
}

static Bool
SMI_CrtcConfigResize(ScrnInfoPtr       pScrn,
		     int               width,
		     int               height)
{
    SMIPtr pSmi = SMIPTR(pScrn);
    xf86CrtcConfigPtr crtcConf = XF86_CRTC_CONFIG_PTR(pScrn);
    int i;
    xf86CrtcPtr crtc;

    ENTER();

    /* Allocate another offscreen area and use it as screen, if it really has to be resized */
    if(!pSmi->NoAccel && pSmi->useEXA &&
       ( !pSmi->fbArea || width != pScrn->virtualX || height != pScrn->virtualY )){
	int aligned_pitch = (width*pSmi->Bpp + 15) & ~15;

	ExaOffscreenArea* fbArea = exaOffscreenAlloc(pScrn->pScreen, aligned_pitch*height, 16, TRUE, NULL, NULL);
	if(!fbArea){
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "SMI_CrtcConfigResize: Not enough memory to resize the framebuffer\n");
	    RETURN(FALSE);
	}

	if(pSmi->fbArea)
	    exaOffscreenFree(pScrn->pScreen, pSmi->fbArea);

	pSmi->fbArea = fbArea;
	pSmi->FBOffset = fbArea->offset;
	pScrn->fbOffset = pSmi->FBOffset + pSmi->fbMapOffset;

	if(pScrn->pixmapPrivate.ptr)
	    /* Framebuffer access is disabled */
	    pScrn->pixmapPrivate.ptr = pSmi->FBBase + pSmi->FBOffset;
	else
	    pScrn->pScreen->ModifyPixmapHeader(pScrn->pScreen->GetScreenPixmap(pScrn->pScreen),
					       -1,-1,-1,-1,-1, pSmi->FBBase + pSmi->FBOffset);

	/* Modify the screen pitch */
	pScrn->displayWidth = aligned_pitch / pSmi->Bpp;
	pScrn->pScreen->ModifyPixmapHeader(pScrn->pScreen->GetScreenPixmap(pScrn->pScreen),
					   -1, -1, -1, -1, aligned_pitch, NULL);
    }

    /* Modify the screen dimensions */
    pScrn->virtualX = width;
    pScrn->virtualY = height;
    pScrn->pScreen->ModifyPixmapHeader(pScrn->pScreen->GetScreenPixmap(pScrn->pScreen),
				       width, height, -1, -1, 0, NULL);

    /* Setup each crtc video processor */
    for(i=0;i<crtcConf->num_crtc;i++){
	crtc = crtcConf->crtc[i];
	SMICRTC(crtc)->video_init(crtc);
	SMICRTC(crtc)->adjust_frame(crtc,crtc->x,crtc->y);
    }

    RETURN(TRUE);
}

void
SMI_CrtcFuncsInit_base(xf86CrtcFuncsPtr crtcFuncs, SMICrtcPrivatePtr crtcPriv){
    memset(crtcFuncs,0,sizeof(xf86CrtcFuncsRec));
    memset(crtcPriv,0,sizeof(SMICrtcPrivatePtr));

    crtcFuncs->dpms = SMI_CrtcDPMS;
    crtcFuncs->lock = SMI_CrtcLock;
    crtcFuncs->unlock = SMI_CrtcUnlock;
    crtcFuncs->mode_fixup = SMI_CrtcModeFixup;
    crtcFuncs->prepare = SMI_CrtcPrepare;
    crtcFuncs->commit = SMI_CrtcCommit;
    crtcFuncs->gamma_set = SMI_CrtcGammaSet;
    crtcFuncs->shadow_allocate = SMI_CrtcShadowAllocate;
    crtcFuncs->shadow_create = SMI_CrtcShadowCreate;
    crtcFuncs->shadow_destroy = SMI_CrtcShadowDestroy;
    crtcFuncs->destroy = SMI_CrtcDestroy;
}

static xf86CrtcConfigFuncsRec SMI_CrtcConfigFuncs = {
    .resize = SMI_CrtcConfigResize
};

Bool
SMI_CrtcPreInit(ScrnInfoPtr pScrn)
{
    SMIPtr pSmi = SMIPTR(pScrn);

    ENTER();

    xf86CrtcConfigInit(pScrn,&SMI_CrtcConfigFuncs);

    xf86CrtcSetSizeRange(pScrn,128,128,4096,4096);

    if(SMI_MSOC_SERIES(pSmi->Chipset)){
	RETURN( SMI501_CrtcPreInit(pScrn) );
    }else{
	RETURN( SMILynx_CrtcPreInit(pScrn) );
    }
}
