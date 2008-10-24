/*
Copyright (C) 1994-1999 The XFree86 Project, Inc.  All Rights Reserved.
Copyright (C) 2000 Silicon Motion, Inc.  All Rights Reserved.
Copyright (C) 2008 Mandriva Linux.  All Rights Reserved.
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

Except as contained in this notice, the names of The XFree86 Project and
Silicon Motion shall not be used in advertising or otherwise to promote the
sale, use or other dealings in this Software without prior written
authorization from The XFree86 Project or Silicon Motion.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "smi.h"
#include "smi_crtc.h"
#include "smi_501.h"

/* Want to see register dumps for now */
#undef VERBLEV
#define VERBLEV		1

static void
SMI501_CrtcVideoInit_lcd(xf86CrtcPtr crtc)
{
    ScrnInfoPtr pScrn=crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    MSOCRegPtr mode = pSmi->mode;
    int		pitch, width;

    ENTER();

    mode->panel_display_ctl.value = READ_SCR(pSmi, PANEL_DISPLAY_CTL);
    mode->panel_fb_width.value = READ_SCR(pSmi, PANEL_FB_WIDTH);


    mode->panel_display_ctl.f.format =
	pScrn->bitsPerPixel == 8 ? 0 :
	pScrn->bitsPerPixel == 16 ? 1 : 2;

    pitch = (((crtc->rotatedData? crtc->mode.HDisplay : pScrn->displayWidth) *
	      pSmi->Bpp) + 15) & ~15;
    width = ((crtc->mode.HDisplay * pSmi->Bpp) + 15) & ~ 15;

    /* >> 4 because of the "unused bits" that should be set to 0 */
    mode->panel_fb_width.f.offset = pitch >> 4;
    mode->panel_fb_width.f.width = width >> 4;

    WRITE_SCR(pSmi, PANEL_DISPLAY_CTL, mode->panel_display_ctl.value);
    WRITE_SCR(pSmi, PANEL_FB_WIDTH, mode->panel_fb_width.value);

    LEAVE();
}

static void
SMI501_CrtcVideoInit_crt(xf86CrtcPtr crtc)
{
    ScrnInfoPtr pScrn=crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    MSOCRegPtr mode = pSmi->mode;
    int		pitch, width;

    ENTER();

    mode->crt_display_ctl.value = READ_SCR(pSmi, CRT_DISPLAY_CTL);
    mode->crt_fb_width.value = READ_SCR(pSmi, CRT_FB_WIDTH);


    mode->crt_display_ctl.f.format =
	pScrn->bitsPerPixel == 8 ? 0 :
	pScrn->bitsPerPixel == 16 ? 1 : 2;

    pitch = (((crtc->rotatedData? crtc->mode.HDisplay : pScrn->displayWidth) *
	      pSmi->Bpp) + 15) & ~15;
    width = ((crtc->mode.HDisplay * pSmi->Bpp) + 15) & ~ 15;

    /* >> 4 because of the "unused bits" that should be set to 0 */
    mode->crt_fb_width.f.offset = pitch >> 4;
    mode->crt_fb_width.f.width = width >> 4;


    WRITE_SCR(pSmi, CRT_DISPLAY_CTL, mode->crt_display_ctl.value);
    WRITE_SCR(pSmi, CRT_FB_WIDTH, mode->crt_fb_width.value);

    LEAVE();
}

static void
SMI501_CrtcAdjustFrame(xf86CrtcPtr crtc, int x, int y)
{
    ScrnInfoPtr pScrn=crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    xf86CrtcConfigPtr crtcConf = XF86_CRTC_CONFIG_PTR(pScrn);
    MSOCRegPtr mode = pSmi->mode;
    CARD32 Base;

    ENTER();

    if(crtc->rotatedData)
	Base = (char*)crtc->rotatedData - (char*)pSmi->FBBase;
    else
	Base = pSmi->FBOffset + (x + y * pScrn->displayWidth) * pSmi->Bpp;

    Base = (Base + 15) & ~15;

    if (crtc == crtcConf->crtc[0]) {
	mode->panel_fb_address.f.address = Base >> 4;
	mode->panel_fb_address.f.pending = 1;
	WRITE_SCR(pSmi, PANEL_FB_ADDRESS, mode->panel_fb_address.value);
    }
    else {
	mode->crt_display_ctl.f.pixel = ((x * pSmi->Bpp) & 15) / pSmi->Bpp;
	WRITE_SCR(pSmi, CRT_DISPLAY_CTL, mode->crt_display_ctl.value);
	mode->crt_fb_address.f.address = Base >> 4;
	mode->crt_fb_address.f.mselect = 0;
	mode->crt_fb_address.f.pending = 1;
	WRITE_SCR(pSmi, CRT_FB_ADDRESS, mode->crt_fb_address.value);
    }

    LEAVE();
}

static void
SMI501_CrtcModeSet_lcd(xf86CrtcPtr crtc,
		       DisplayModePtr xf86mode,
		       DisplayModePtr adjusted_mode,
		       int x, int y)
{
    ScrnInfoPtr pScrn=crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    MSOCRegPtr mode = pSmi->mode;
    double	p2_diff, pll_diff;
    int32_t	x2_select, x2_divider, x2_shift, x2_1xclck;

    ENTER();

    /* Initialize the display controller */

    SMI501_CrtcVideoInit_lcd(crtc);

    /* P2CLK have dividers 1, 3 and 5 */
    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, VERBLEV,
		   "Clock request %5.2f (max_divider %d)\n",
		   (double)xf86mode->Clock, 5);
    p2_diff = SMI501_FindClock(xf86mode->Clock, 5,
			       (uint32_t)mode->device_id.f.revision >= 0xc0,
			       &x2_1xclck, &x2_select, &x2_divider,
			       &x2_shift);
    mode->clock.f.p2_select = x2_select;
    mode->clock.f.p2_divider = x2_divider;
    mode->clock.f.p2_shift = x2_shift;
    mode->clock.f.p2_1xclck = x2_1xclck;

    /* Check if it is a SMI 502 */
    /* FIXME May need to add a Boolean option here, (or use extra
     * xorg.conf options?) to force it to not use 502 mode set. */
    if ((uint32_t)mode->device_id.f.revision >= 0xc0) {
	int32_t	m, n, xclck;

	pll_diff = SMI501_FindPLLClock(xf86mode->Clock, &m, &n, &xclck);
	if (pll_diff < p2_diff) {

	    /* Zero the pre 502 bitfield */
	    mode->clock.f.p2_select  = 0;
	    mode->clock.f.p2_divider = 0;
	    mode->clock.f.p2_shift   = 0;
	    mode->clock.f.p2_1xclck  = 0;

	    mode->clock.f.pll_select = 1;
	    mode->pll_ctl.f.m = m;
	    mode->pll_ctl.f.n = n;

	    /* 0: Crystal input
	     * 1: Test clock input */
	    mode->pll_ctl.f.select = 0;

	    /* 0: pll output divided by 1
	     * 1: pll output divided by 2 */
	    mode->pll_ctl.f.divider = xclck != 1;
	    mode->pll_ctl.f.power = 1;
	}
	else
	    mode->clock.f.pll_select = 0;
    }
    else
	mode->clock.f.pll_select = 0;

    mode->panel_display_ctl.f.enable = 1;
    mode->panel_display_ctl.f.timing = 1;

    mode->panel_wwidth.f.x = 0;
    mode->panel_wwidth.f.width = xf86mode->HDisplay;

    mode->panel_wheight.f.y = 0;
    mode->panel_wheight.f.height = xf86mode->VDisplay;

    mode->panel_plane_tl.f.top = 0;
    mode->panel_plane_tl.f.left = 0;

    mode->panel_plane_br.f.right = xf86mode->HDisplay - 1;
    mode->panel_plane_br.f.bottom = xf86mode->VDisplay - 1;

    /* 0 means pulse high */
    mode->panel_display_ctl.f.hsync = !(xf86mode->Flags & V_PHSYNC);
    mode->panel_display_ctl.f.vsync = !(xf86mode->Flags & V_PVSYNC);

    mode->panel_htotal.f.total = xf86mode->HTotal - 1;
    mode->panel_htotal.f.end = xf86mode->HDisplay - 1;

    mode->panel_hsync.f.start = xf86mode->HSyncStart - 1;
    mode->panel_hsync.f.width = xf86mode->HSyncEnd -
	xf86mode->HSyncStart;

    mode->panel_vtotal.f.total = xf86mode->VTotal - 1;
    mode->panel_vtotal.f.end = xf86mode->VDisplay - 1;

    mode->panel_vsync.f.start = xf86mode->VSyncStart;
    mode->panel_vsync.f.height = xf86mode->VSyncEnd -
	xf86mode->VSyncStart;


    SMI501_WriteMode_lcd(pScrn,mode);
    SMI501_CrtcAdjustFrame(crtc, x, y);

    LEAVE();
}

static void
SMI501_CrtcModeSet_crt(xf86CrtcPtr crtc,
		       DisplayModePtr xf86mode,
		       DisplayModePtr adjusted_mode,
		       int x, int y)
{
    ScrnInfoPtr pScrn=crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    MSOCRegPtr mode = pSmi->mode;
    int32_t	x2_select, x2_divider, x2_shift, x2_1xclck;

    ENTER();

    /* Initialize the display controller */

    SMI501_CrtcVideoInit_crt(crtc);

    /* V2CLK have dividers 1 and 3 */
    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, VERBLEV,
		   "Clock request %5.2f (max_divider %d)\n",
		   (double)xf86mode->Clock, 3);
    (void)SMI501_FindClock(xf86mode->Clock, 3,
			   (uint32_t)mode->device_id.f.revision >= 0xc0,
			   &x2_1xclck, &x2_select, &x2_divider, &x2_shift);
    mode->clock.f.v2_select = x2_select;
    mode->clock.f.v2_divider = x2_divider;
    mode->clock.f.v2_shift = x2_shift;
    mode->clock.f.v2_1xclck = x2_1xclck;

    /* 0: select panel - 1: select crt */
    mode->crt_display_ctl.f.select = 1;
    mode->crt_display_ctl.f.enable = 1;
    mode->crt_display_ctl.f.timing = 1;
    /* 0: show pixels - 1: blank */
    mode->crt_display_ctl.f.blank = 0;

    mode->crt_fb_address.f.mextern = 0;	/* local memory */

    /* 0 means pulse high */
    mode->crt_display_ctl.f.hsync = !(xf86mode->Flags & V_PHSYNC);
    mode->crt_display_ctl.f.vsync = !(xf86mode->Flags & V_PVSYNC);

    mode->crt_htotal.f.total = xf86mode->HTotal - 1;
    mode->crt_htotal.f.end = xf86mode->HDisplay - 1;

    mode->crt_hsync.f.start = xf86mode->HSyncStart - 1;
    mode->crt_hsync.f.width = xf86mode->HSyncEnd -
	xf86mode->HSyncStart;

    mode->crt_vtotal.f.total = xf86mode->VTotal - 1;
    mode->crt_vtotal.f.end = xf86mode->VDisplay - 1;

    mode->crt_vsync.f.start = xf86mode->VSyncStart;
    mode->crt_vsync.f.height = xf86mode->VSyncEnd -
	xf86mode->VSyncStart;

    SMI501_WriteMode_crt(pScrn,mode);
    SMI501_CrtcAdjustFrame(crtc, x, y);

    LEAVE();
}

static void
SMI501_CrtcLoadLUT(xf86CrtcPtr crtc)
{
    ScrnInfoPtr pScrn = crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    xf86CrtcConfigPtr crtcConf = XF86_CRTC_CONFIG_PTR(pScrn);
    SMICrtcPrivatePtr crtcPriv = SMICRTC(crtc);
    int i,port;

    ENTER();

    port = crtc == crtcConf->crtc[0] ? PANEL_PALETTE : CRT_PALETTE;
    for (i = 0; i < 256; i++)
	WRITE_SCR(pSmi, port + (i  <<  2),
		  (crtcPriv->lut_r[i] >> 8 << 16) |
		  (crtcPriv->lut_g[i] >> 8 << 8) |
		  (crtcPriv->lut_b[i] >> 8) );

    LEAVE();
}

static xf86CrtcFuncsRec SMI501_Crtc0Funcs;
static SMICrtcPrivateRec SMI501_Crtc0Priv;
static xf86CrtcFuncsRec SMI501_Crtc1Funcs;
static SMICrtcPrivateRec SMI501_Crtc1Priv;

Bool
SMI501_CrtcPreInit(ScrnInfoPtr pScrn)
{
    SMIPtr	pSmi = SMIPTR(pScrn);
    xf86CrtcPtr crtc0, crtc1;

    ENTER();

    /* CRTC0 is LCD */
    SMI_CrtcFuncsInit_base(&SMI501_Crtc0Funcs, &SMI501_Crtc0Priv);
    SMI501_Crtc0Funcs.mode_set		= SMI501_CrtcModeSet_lcd;
    SMI501_Crtc0Priv.adjust_frame	= SMI501_CrtcAdjustFrame;
    SMI501_Crtc0Priv.video_init		= SMI501_CrtcVideoInit_lcd;
    SMI501_Crtc0Priv.load_lut		= SMI501_CrtcLoadLUT;

    crtc0 = xf86CrtcCreate(pScrn, &SMI501_Crtc0Funcs);
    if (!crtc0)
	RETURN(FALSE);
    crtc0->driver_private = &SMI501_Crtc0Priv;

    /* CRTC1 is CRT */
    if (pSmi->Dualhead) {
	SMI_CrtcFuncsInit_base(&SMI501_Crtc1Funcs, &SMI501_Crtc1Priv);
	SMI501_Crtc1Funcs.mode_set	= SMI501_CrtcModeSet_crt;
	SMI501_Crtc1Priv.adjust_frame	= SMI501_CrtcAdjustFrame;
	SMI501_Crtc1Priv.video_init	= SMI501_CrtcVideoInit_crt;
	SMI501_Crtc1Priv.load_lut	= SMI501_CrtcLoadLUT;

	crtc1 = xf86CrtcCreate(pScrn, &SMI501_Crtc1Funcs);
	if (!crtc1)
	    RETURN(FALSE);
	crtc1->driver_private = &SMI501_Crtc1Priv;
    }

    RETURN(TRUE);
}

