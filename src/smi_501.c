/*
Copyright (C) 1994-1999 The XFree86 Project, Inc.  All Rights Reserved.
Copyright (C) 2000 Silicon Motion, Inc.  All Rights Reserved.
Copyright (C) 2008 Mandriva Linux.  All Rights Reserved.

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

#include "xf86Resources.h"
#include "xf86RAC.h"
#include "xf86DDC.h"
#include "xf86int10.h"
#include "vbe.h"
#include "shadowfb.h"

#include "smi.h"
#include "smi_501.h"
#include "regsmi.h"

#include "globals.h"
#define DPMS_SERVER
#include <X11/extensions/dpms.h>

/* Want to see register dumps for now */
#undef VERBLEV
#define VERBLEV		1


/*
 * Prototypes
 */
static void SMI501_ModeSet(ScrnInfoPtr pScrn, MSOCRegPtr mode);
static char *format_integer_base2(int32_t word);

static double SMI501_FindClock(double clock, int max_divider,
				int32_t *x2_1xclck,
				int32_t *x2_select,
				int32_t *x2_divider, int32_t *x2_shift);
static double SMI501_FindPLLClock(double clock, int32_t *m, int32_t *n);
static void SMI501_PrintRegs(ScrnInfoPtr pScrn);
static void SMI501_SetClock(SMIPtr pSmi, int32_t port,
			    int32_t pll, int32_t value);
static void SMI501_WaitVSync(SMIPtr pSmi, int vsync_count);


/*
 * Implemementation
 */
Bool
SMI501_EnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr	pScrn = xf86Screens[scrnIndex];
    SMIPtr	pSmi = SMIPTR(pScrn);
    Bool	result;

    /* Enable MMIO and map memory */
    SMI_MapMem(pScrn);

    pSmi->Save(pScrn);

    /* FBBase may have changed after remapping the memory */
    pScrn->pixmapPrivate.ptr = pSmi->FBBase;
    if(pSmi->useEXA)
	pSmi->EXADriverPtr->memoryBase = pSmi->FBBase;

    /* #670 */
    if (pSmi->shadowFB) {
	pSmi->FBOffset = pSmi->savedFBOffset;
	pSmi->FBReserved = pSmi->savedFBReserved;
    }

    result = pSmi->ModeInit(pScrn, pScrn->currentMode);

    if (result && pSmi->shadowFB) {
	BoxRec box;

	if (pSmi->pSaveBuffer) {
	    memcpy(pSmi->FBBase, pSmi->pSaveBuffer, pSmi->saveBufferSize);
	    xfree(pSmi->pSaveBuffer);
	    pSmi->pSaveBuffer = NULL;
	}

	box.x1 = 0;
	box.y1 = 0;
	box.x2 = pSmi->width;
	box.y2 = pSmi->height;
	SMI_RefreshArea(pScrn, 1, &box);
    }

    /* Reset the grapics engine */
    if (!pSmi->NoAccel)
	SMI_EngineReset(pScrn);

    return (result);
}

void
SMI501_LeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr	pScrn = xf86Screens[scrnIndex];
    SMIPtr	pSmi = SMIPTR(pScrn);

    if (pSmi->shadowFB) {
	pSmi->pSaveBuffer = xnfalloc(pSmi->saveBufferSize);
	if (pSmi->pSaveBuffer)
	    memcpy(pSmi->pSaveBuffer, pSmi->FBBase, pSmi->saveBufferSize);

	pSmi->savedFBOffset = pSmi->FBOffset;
	pSmi->savedFBReserved = pSmi->FBReserved;
    }

    memset(pSmi->FBBase, 0, 256 * 1024);
    SMI_UnmapMem(pScrn);
}

void
SMI501_Save(ScrnInfoPtr pScrn)
{
    SMIPtr	pSmi = SMIPTR(pScrn);
    MSOCRegPtr	save = pSmi->save;

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, VERBLEV,
		   "Register dump (Before Save)\n");
    SMI501_PrintRegs(pScrn);

    /* Used mainly for DPMS info */
    save->system_ctl.value = READ_SCR(pSmi, SYSTEM_CTL);

    /* Used basically to enable dac */
    save->misc_ctl.value = READ_SCR(pSmi, MISC_CTL);

    /* Read it first to know if current power mode */
    save->power_ctl.value = READ_SCR(pSmi, POWER_CTL);

    switch (save->power_ctl.f.mode) {
	case 0:
	    save->current_gate  = POWER0_GATE;
	    save->current_clock = POWER0_CLOCK;
	    break;
	case 1:
	    save->current_gate  = POWER1_GATE;
	    save->current_clock = POWER1_CLOCK;
	    break;
	default:
	    /* FIXME
	     * Should be in sleep mode
	     * TODO
	     * select mode0 by default
	     */
	    save->current_gate = POWER0_GATE;
	    save->current_clock = POWER0_CLOCK;
	    break;
    }

    save->gate.value  = READ_SCR(pSmi, save->current_gate);
    save->clock.value = READ_SCR(pSmi, save->current_clock);

    /* FIXME Never changed */
    save->timing_ctl.value = READ_SCR(pSmi, TIMING_CTL);

    save->pll_ctl.value = READ_SCR(pSmi, PLL_CTL);
    save->device_id.value = READ_SCR(pSmi, DEVICE_ID);
    save->sleep_gate.value = READ_SCR(pSmi, SLEEP_GATE);

    save->panel_display_ctl.value = READ_SCR(pSmi, PANEL_DISPLAY_CTL);
    save->panel_fb_address.value = READ_SCR(pSmi, PANEL_FB_ADDRESS);
    save->panel_fb_width.value = READ_SCR(pSmi, PANEL_FB_WIDTH);
    save->panel_wwidth.value = READ_SCR(pSmi, PANEL_WWIDTH);
    save->panel_wheight.value = READ_SCR(pSmi, PANEL_WHEIGHT);
    save->panel_plane_tl.value = READ_SCR(pSmi, PANEL_PLANE_TL);
    save->panel_plane_br.value = READ_SCR(pSmi, PANEL_PLANE_BR);
    save->panel_htotal.value = READ_SCR(pSmi, PANEL_HTOTAL);
    save->panel_hsync.value = READ_SCR(pSmi, PANEL_HSYNC);
    save->panel_vtotal.value = READ_SCR(pSmi, PANEL_VTOTAL);
    save->panel_vsync.value = READ_SCR(pSmi, PANEL_VSYNC);

    save->crt_display_ctl.value = READ_SCR(pSmi, CRT_DISPLAY_CTL);
    save->crt_fb_address.value = READ_SCR(pSmi, CRT_FB_ADDRESS);
    save->crt_fb_width.value = READ_SCR(pSmi, CRT_FB_WIDTH);
    save->crt_htotal.value = READ_SCR(pSmi, CRT_HTOTAL);
    save->crt_hsync.value = READ_SCR(pSmi, CRT_HSYNC);
    save->crt_vtotal.value = READ_SCR(pSmi, CRT_VTOTAL);
    save->crt_vsync.value = READ_SCR(pSmi, CRT_VSYNC);
}

void
SMI501_DisplayPowerManagementSet(ScrnInfoPtr pScrn,
				 int PowerManagementMode, int flags)
{
    SMIPtr		pSmi = SMIPTR(pScrn);
    MSOCRegPtr		mode = pSmi->mode;

    if (pSmi->CurrentDPMS != PowerManagementMode) {
	mode->system_ctl.value = READ_SCR(pSmi, SYSTEM_CTL);
	switch (PowerManagementMode) {
	    case DPMSModeOn:
		mode->system_ctl.f.dpmsh = 1;
		mode->system_ctl.f.dpmsv = 1;
		break;
	    case DPMSModeStandby:
		mode->system_ctl.f.dpmsh = 0;
		mode->system_ctl.f.dpmsv = 1;
		break;
	    case DPMSModeSuspend:
		mode->system_ctl.f.dpmsh = 1;
		mode->system_ctl.f.dpmsv = 0;
		break;
	    case DPMSModeOff:
		mode->system_ctl.f.dpmsh = 0;
		mode->system_ctl.f.dpmsv = 0;
		break;
	}
	WRITE_SCR(pSmi, SYSTEM_CTL, mode->system_ctl.value);
	pSmi->CurrentDPMS = PowerManagementMode;
    }
}

Bool
SMI501_ModeInit(ScrnInfoPtr pScrn, DisplayModePtr xf86mode)
{
    MSOCRegPtr	save;
    MSOCRegPtr	mode;
    SMIPtr	pSmi = SMIPTR(pScrn);
    double	p2_diff, pll_diff;
    int32_t	x2_select, x2_divider, x2_shift, x2_1xclck, p2_pll;

    save = pSmi->save;
    mode = pSmi->mode;

    pSmi->Bpp = pScrn->bitsPerPixel / 8;
    if (pSmi->rotate) {
	pSmi->width  = pScrn->virtualY;
	pSmi->height = pScrn->virtualX;
	pSmi->Stride = (pSmi->height * pSmi->Bpp + 15) & ~15;
    } 
    else {
	pSmi->width  = pScrn->virtualX;
	pSmi->height = pScrn->virtualY;
	pSmi->Stride = (pSmi->width * pSmi->Bpp + 15) & ~15;
    }

    /* Start with a fresh copy of registers before any mode change */
    memcpy(mode, save, sizeof(MSOCRegRec));

    if (pSmi->UseFBDev)
	return (TRUE);

    /* Enable DAC -- 0: enable - 1: disable */
    mode->misc_ctl.f.dac = 0;

    /* Enable 2D engine */
    mode->gate.f.engine = 1;
    /* Color space conversion */
    mode->gate.f.csc = 1;
    /* ZV port */
    mode->gate.f.zv = 1;
    /* Gpio, Pwm, and I2c */
    mode->gate.f.gpio = 1;

    /* FIXME fixed at power mode 0 as in the smi sources */
    mode->power_ctl.f.status = 0;
    mode->power_ctl.f.mode = 0;

    /* FIXME fixed at 336/3/0 as in the smi sources */
    mode->clock.f.m_select = 1;
    mode->clock.f.m_divider = 1;
    mode->clock.f.m_shift = 0;

    /* FIXME probably should not "touch" m1clk. A value other then 112Mhz
     * will instant lock on my test prototype, "or" maybe it just means
     * that m1clk value must be equal to mclk value? (and mclk must be
     * set first!?) */
    switch (pSmi->MCLK) {
	case 168000:	    /* 336/1/1 */
	    mode->clock.f.m1_select = 1;
	    mode->clock.f.m1_divider = 0;
	    mode->clock.f.m1_shift = 1;
	    break;
	case 96000:	    /* 288/3/0 */
	    mode->clock.f.m1_select = 0;
	    mode->clock.f.m1_divider = 1;
	    mode->clock.f.m1_shift = 0;
	    break;
	case 144000:	    /* 288/1/1 */
	    mode->clock.f.m1_select = 0;
	    mode->clock.f.m1_divider = 0;
	    mode->clock.f.m1_shift = 1;
	    break;
	case 112000:	    /* 336/3/0 */
	default:
	    mode->clock.f.m1_select = 1;
	    mode->clock.f.m1_divider = 1;
	    mode->clock.f.m1_shift = 0;
	    break;
    }

    if (pSmi->lcd) {
	/* P2CLK can have dividers 1, 3 and 5 */
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, VERBLEV,
		       "Clock request %5.2f (max_divider %d)\n",
		       (double)xf86mode->Clock, 5);
	p2_diff = SMI501_FindClock(xf86mode->Clock, 5, &x2_1xclck,
				   &x2_select, &x2_divider, &x2_shift);
	mode->clock.f.p2_select = x2_select;
	mode->clock.f.p2_divider = x2_divider;
	mode->clock.f.p2_shift = x2_shift;
	mode->clock.f.p2_1xclck = x2_1xclck;

	/* Check if it is a SMI 502 */
	/* FIXME May need to add a Boolean option here, (or use extra
	 * xorg.conf options?) to force it to not use 502 mode set. */
	if ((uint32_t)mode->device_id.f.revision >= 0xc0) {
	    int32_t	m, n;

	    pll_diff = SMI501_FindPLLClock(xf86mode->Clock, &m, &n);
	    if (pll_diff < p2_diff) {

		/* FIXME Need to clarify. This is required for the GDIUM,
		 * that should have set it to 0 earlier, but is there some
		 * rule to choose the value? */
		mode->clock.f.p2_1xclck = 1;

		mode->clock.f.pll_select = 1;
		mode->pll_ctl.f.m = m;
		mode->pll_ctl.f.n = n;

		/* 0: Crystal input	1: Test clock input */
		mode->pll_ctl.f.select = 0;

		/* FIXME Divider means (redundant) enable p2xxx or does
		 * it mean it can also calculate with 12MHz output, or
		 * something else? */
		mode->pll_ctl.f.divider = 0;
		mode->pll_ctl.f.power = 1;
	    }
	    else
		mode->clock.f.pll_select = 0;
	}
	else
	    mode->clock.f.pll_select = 0;

	mode->panel_display_ctl.f.format =
	    pScrn->bitsPerPixel == 8 ? 0 :
	    pScrn->bitsPerPixel == 16 ? 1 : 2;

	mode->panel_display_ctl.f.enable = 1;
	mode->panel_display_ctl.f.timing = 1;

	/* FIXME if non clone dual head, and secondary, need to
	 * properly set panel fb address properly ... */
	mode->panel_fb_address.f.address = 0;
	mode->panel_fb_address.f.mextern = 0;	/* local memory */
	mode->panel_fb_address.f.pending = 0;	/* FIXME required? */

	/* >> 4 because of the "unused bits" that should be set to 0 */
	/* FIXME this should be used for virtual size? */
	mode->panel_fb_width.f.offset = pSmi->Stride >> 4;
	mode->panel_fb_width.f.width = pSmi->Stride >> 4;

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

	mode->panel_hsync.f.start = xf86mode->HSyncStart;
	mode->panel_hsync.f.width = xf86mode->HSyncEnd -
	    xf86mode->HSyncStart;

	mode->panel_vtotal.f.total = xf86mode->VTotal - 1;
	mode->panel_vtotal.f.end = xf86mode->VDisplay - 1;

	mode->panel_vsync.f.start = xf86mode->VSyncStart;
	mode->panel_vsync.f.height = xf86mode->VSyncEnd -
	    xf86mode->VSyncStart;
    }
    else {
	/* V2CLK can have dividers 1 and 3 */
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, VERBLEV,
		       "Clock request %5.2f (max_divider %d)\n",
		       (double)xf86mode->Clock, 3);
	(void)SMI501_FindClock(xf86mode->Clock, 3, &x2_1xclck,
			       &x2_select, &x2_divider, &x2_shift);
	mode->clock.f.v2_select = x2_select;
	mode->clock.f.v2_divider = x2_divider;
	mode->clock.f.v2_shift = x2_shift;
	mode->clock.f.v2_1xclck = x2_1xclck;

	mode->crt_display_ctl.f.format =
	    pScrn->bitsPerPixel == 8 ? 0 :
	    pScrn->bitsPerPixel == 16 ? 1 : 2;

	/* 0: select panel - 1: select crt */
	mode->crt_display_ctl.f.select = 1;
	mode->crt_display_ctl.f.enable = 1;

	/* FIXME if non clone dual head, and secondary, need to
	 * properly set crt fb address ... */
	mode->crt_fb_address.f.address = 0;
	mode->crt_fb_address.f.mextern = 0;	/* local memory */
	mode->crt_fb_address.f.pending = 0;	/* FIXME required? */

	/* >> 4 because of the "unused bits" that should be set to 0 */
	/* FIXME this should be used for virtual size? */
	mode->crt_fb_width.f.offset = pSmi->Stride >> 4;
	mode->crt_fb_width.f.width = pSmi->Stride >> 4;

	/* 0 means pulse high */
	mode->crt_display_ctl.f.hsync = !(xf86mode->Flags & V_PHSYNC);
	mode->crt_display_ctl.f.vsync = !(xf86mode->Flags & V_PVSYNC);

	mode->crt_htotal.f.total = xf86mode->HTotal - 1;
	mode->crt_htotal.f.end = xf86mode->HDisplay - 1;

	mode->crt_hsync.f.start = xf86mode->HSyncStart;
	mode->crt_hsync.f.width = xf86mode->HSyncEnd -
	    xf86mode->HSyncStart;

	mode->crt_vtotal.f.total = xf86mode->VTotal - 1;
	mode->crt_vtotal.f.end = xf86mode->VDisplay - 1;

	mode->crt_vsync.f.start = xf86mode->HSyncStart;
	mode->crt_vsync.f.height = xf86mode->HSyncEnd -
	    xf86mode->HSyncStart;
    }

    SMI501_ModeSet(pScrn, mode);

    SMI_AdjustFrame(pScrn->scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, VERBLEV,
		   "Register dump (After Mode Init)\n");
    SMI501_PrintRegs(pScrn);

    return (TRUE);
}

static void
SMI501_ModeSet(ScrnInfoPtr pScrn, MSOCRegPtr mode)
{
    int32_t		pll;
    MSOCClockRec	clock;
    SMIPtr		pSmi = SMIPTR(pScrn);

    /* Update gate first */
    WRITE_SCR(pSmi, mode->current_gate, mode->gate.value);

    clock.value = READ_SCR(pSmi, mode->current_clock);

    clock.f.m_select = mode->clock.f.m_select;
    pll = clock.value;
    clock.f.m_divider = mode->clock.f.m_divider;
    clock.f.m_shift = mode->clock.f.m_shift;
    SMI501_SetClock(pSmi, mode->current_clock, pll, clock.value);

    clock.f.m1_select = mode->clock.f.m1_select;
    pll = clock.value;
    clock.f.m1_divider = mode->clock.f.m1_divider;
    clock.f.m1_shift = mode->clock.f.m1_shift;
    SMI501_SetClock(pSmi, mode->current_clock, pll, clock.value);

    if (pSmi->lcd) {
	/* Alternate pll_select is only available for the SMI 502,
	 * and the bit should be only set in that case. */
	if (mode->clock.f.pll_select)
	    WRITE_SCR(pSmi, PLL_CTL, mode->pll_ctl.value);
	clock.f.p2_select = mode->clock.f.p2_select;
	pll = clock.value;
	clock.f.p2_divider = mode->clock.f.p2_divider;
	clock.f.p2_shift = mode->clock.f.p2_shift;
	clock.f.pll_select = mode->clock.f.pll_select;
	clock.f.p2_1xclck = mode->clock.f.p2_1xclck;
	SMI501_SetClock(pSmi, mode->current_clock, pll, clock.value);
    }
    else {
	clock.f.v2_select = mode->clock.f.v2_select;
	pll = clock.value;
	clock.f.v2_divider = mode->clock.f.v2_divider;
	clock.f.v2_shift = mode->clock.f.v2_shift;
	clock.f.v2_1xclck = mode->clock.f.v2_1xclck;
	SMI501_SetClock(pSmi, mode->current_clock, pll, clock.value);
    }

    WRITE_SCR(pSmi, MISC_CTL, mode->misc_ctl.value);

    if (pSmi->lcd) {
	WRITE_SCR(pSmi, PANEL_FB_ADDRESS, mode->panel_fb_address.value);
	WRITE_SCR(pSmi, PANEL_FB_WIDTH, mode->panel_fb_width.value);

	WRITE_SCR(pSmi, PANEL_WWIDTH, mode->panel_wwidth.value);
	WRITE_SCR(pSmi, PANEL_WHEIGHT, mode->panel_wheight.value);

	WRITE_SCR(pSmi, PANEL_PLANE_TL, mode->panel_plane_tl.value);
	WRITE_SCR(pSmi, PANEL_PLANE_BR, mode->panel_plane_br.value);

	WRITE_SCR(pSmi, PANEL_HTOTAL, mode->panel_htotal.value);
	WRITE_SCR(pSmi, PANEL_HSYNC, mode->panel_hsync.value);
	WRITE_SCR(pSmi, PANEL_VTOTAL, mode->panel_vtotal.value);
	WRITE_SCR(pSmi, PANEL_VSYNC, mode->panel_vsync.value);
	WRITE_SCR(pSmi, PANEL_DISPLAY_CTL, mode->panel_display_ctl.value);

	/* Power up sequence for panel */
	mode->panel_display_ctl.f.vdd = 1;
	WRITE_SCR(pSmi, PANEL_DISPLAY_CTL, mode->panel_display_ctl.value);
	SMI501_WaitVSync(pSmi, 4);

	mode->panel_display_ctl.f.signal = 1;
	WRITE_SCR(pSmi, PANEL_DISPLAY_CTL, mode->panel_display_ctl.value);
	SMI501_WaitVSync(pSmi, 4);

	mode->panel_display_ctl.f.bias = 1;
	WRITE_SCR(pSmi, PANEL_DISPLAY_CTL, mode->panel_display_ctl.value);
	SMI501_WaitVSync(pSmi, 4);

	mode->panel_display_ctl.f.fp = 1;
	WRITE_SCR(pSmi, PANEL_DISPLAY_CTL, mode->panel_display_ctl.value);
	SMI501_WaitVSync(pSmi, 4);

	/* FIXME: No dual head setup, and in this case, crt may
	 * just be another panel */
	/* crt clones panel */
	mode->crt_display_ctl.f.enable = 1;
	/* 0: select panel - 1: select crt */
	mode->crt_display_ctl.f.select = 0;
	WRITE_SCR(pSmi, CRT_DISPLAY_CTL, mode->crt_display_ctl.value);
    }
    else {
	WRITE_SCR(pSmi, CRT_FB_ADDRESS, mode->crt_fb_address.value);
	WRITE_SCR(pSmi, CRT_FB_WIDTH, mode->crt_fb_width.value);
	WRITE_SCR(pSmi, CRT_HTOTAL, mode->crt_htotal.value);
	WRITE_SCR(pSmi, CRT_HSYNC, mode->crt_hsync.value);
	WRITE_SCR(pSmi, CRT_VTOTAL, mode->crt_vtotal.value);
	WRITE_SCR(pSmi, CRT_VSYNC, mode->crt_vsync.value);
	WRITE_SCR(pSmi, CRT_DISPLAY_CTL, mode->crt_display_ctl.value);

	/* Turn CRT on */
	SMI501_DisplayPowerManagementSet(pScrn, DPMSModeOn, 0);
    }

    WRITE_SCR(pSmi, POWER_CTL, mode->power_ctl.value);

    /* Match configuration */
    /* FIXME some other fields should also be set, otherwise, since
     * neither kernel nor driver change it, a reboot is required to
     * modify or reset to default */
    mode->system_ctl.f.burst = mode->system_ctl.f.burst_read =
	pSmi->PCIBurst != FALSE;
    mode->system_ctl.f.retry = pSmi->PCIRetry != FALSE;
    WRITE_SCR(pSmi, SYSTEM_CTL, mode->system_ctl.value);
}

void
SMI501_LoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
		   LOCO *colors, VisualPtr pVisual)
{
    SMIPtr	pSmi = SMIPTR(pScrn);
    int		i, port;

    port = pSmi->IsSecondary ? CRT_PALETTE : PANEL_PALETTE;
    for (i = 0; i < numColors; i++)
	WRITE_SCR(pSmi, port + (indices[i]  <<  2),
		  (colors[indices[i]].red   << 16) |
		  (colors[indices[i]].green <<  8) |
		   colors[indices[i]].blue);
}

static char *
format_integer_base2(int32_t word)
{
    int		i;
    static char	buffer[33];

    for (i = 0; i < 32; i++) {
	if (word & (1 << i))
	    buffer[31 - i] = '1';
	else
	    buffer[31 - i] = '0';
    }

    return (buffer);
}

static double
SMI501_FindClock(double clock, int32_t max_divider, int32_t *x2_1xclck,
		 int32_t *x2_select, int32_t *x2_divider, int32_t *x2_shift)
{
    double	diff, best, mclk;
    int32_t	multiplier, divider, shift, xclck;

    /* The Crystal input frequency is 24Mhz, and can have be multiplied
     * by 12 or 14 (actually, there are other values, see TIMING_CTL,
     * MMIO 0x068) */

    /* Find clock best matching mode */
    best = 0x7fffffff;
    for (multiplier = 12, mclk  = multiplier * 24 * 1000.0;
	 mclk <= 14 * 24 * 1000.0;
	 multiplier += 2, mclk  = multiplier * 24 * 1000.0) {
	for (divider = 1; divider <= max_divider; divider += 2) {
	    for (shift = 0; shift < 8; shift++) {
		for (xclck = 0; xclck <= 1; xclck++) {
		    diff = (mclk / (divider << shift << xclck)) - clock;
		    if (fabs(diff) < best) {
			*x2_shift = shift;
			*x2_divider = divider == 1 ? 0 : divider == 3 ? 1 : 2;
			*x2_1xclck = xclck == 0;

			/* Remember best diff */
			best = fabs(diff);
		    }
		}
	    }
	}
    }

    *x2_select = mclk == 12 * 24 * 1000.0 ? 0 : 1;

    xf86ErrorFVerb(VERBLEV,
		   "\tMatching clock %5.2f, diff %5.2f (%d/%d/%d/%d)\n",
		   ((*x2_select ? 14 : 12) * 24 * 1000.0) /
		   ((*x2_divider == 0 ? 1 : *x2_divider == 1 ? 3 : 5) <<
		    *x2_shift << (*x2_1xclck ? 0 : 1)),
		   best, *x2_shift, *x2_divider, *x2_select, *x2_1xclck);

    return (best);
}

static double
SMI501_FindPLLClock(double clock, int32_t *m, int32_t *n)
{
    int32_t	M, N;
    double	diff, best;

    best = 0x7fffffff;
    for (N = 2; N <= 24; N++) {
	M = clock * N / 24 / 1000.0;
	diff = clock - (24 * 1000.0 * M / N);
	/* Paranoia check for 7 bits range */
	if (M >= 0 && M < 128 && fabs(diff) < best) {
	    *m = M;
	    *n = N;

	    /* Remember best diff */
	    best = fabs(diff);
	}
    }

    xf86ErrorFVerb(VERBLEV,
		   "\tMatching alternate clock %5.2f, diff %5.2f (%d/%d)\n",
		   24 * 1000.0 * *m / *n, best, *m, *n);

    return (best);
}

static void
SMI501_PrintRegs(ScrnInfoPtr pScrn)
{
    int		i;
    SMIPtr	pSmi = SMIPTR(pScrn);

    xf86ErrorFVerb(VERBLEV, "    SMI501 System Setup:\n");
    for (i = 0x00; i < 0x6c; i += 4)
	xf86ErrorFVerb(VERBLEV, "\t%08x: %s\n", i,
		       format_integer_base2(READ_SCR(pSmi, i)));
    xf86ErrorFVerb(VERBLEV, "    SMI501 Display Setup:\n");
    for (i = 0x80000; i < 0x80400; i += 4)
	xf86ErrorFVerb(VERBLEV, "\t%08x: %s\n", i,
		       format_integer_base2(READ_SCR(pSmi, i)));
}

static void
SMI501_WaitVSync(SMIPtr pSmi, int vsync_count)
{
    int32_t	status, timeout;

    while (vsync_count-- > 0) {
	/* Wait for end of vsync */
	timeout = 0;
	do {
	    /* bit 11: vsync active *if set* */
	    status = READ_SCR(pSmi, CMD_STATUS);
	    if (++timeout == 10000)
		break;
	} while (status & (1 << 11));

	/* Wait for start of vsync */
	timeout = 0;
	do {
	    status = READ_SCR(pSmi, CMD_STATUS);
	    if (++timeout == 10000)
		break;
	} while (!(status & (1 << 11)));
    }
}

static void
SMI501_SetClock(SMIPtr pSmi, int32_t port, int32_t pll, int32_t value)
{
    /*
     *	Rules to Program the Power Mode Clock Registers for Clock Selection
     *
     *	1. There should be only one clock source changed at a time.
     *	   To change clock source for P2XCLK, V2XCLK, MCLK, M2XCLK
     *	   simultaneously may cause the internal logic normal operation
     *	   to be disrupted. There should be a minimum of 16mS wait from
     *	   change one clock source to another.
     *	2. When adjusting the clock rate, the PLL selection bit should
     *	   be programmed first before changing the divider value for each
     *	   clock source. For example, to change the P2XCLK clock rate:
     *		. bit 29 should be set first
     *		. wait for a minimum of 16ms (about one Vsync time)
     *		. adjust bits [28:24].
     *	   The minimum 16 ms wait is necessary for logic to settle down
     *	   before the clock rate is changed.
     *	3. There should be a minimum 16 ms wait after a clock source is
     *	   changed before any operation that could result in a bus
     *	   transaction.
     */

    /* register contents selecting clock */
    WRITE_SCR(pSmi, port, pll);
    SMI501_WaitVSync(pSmi, 1);

    /* full register contents */
    WRITE_SCR(pSmi, port, value);
    SMI501_WaitVSync(pSmi, 1);
}
