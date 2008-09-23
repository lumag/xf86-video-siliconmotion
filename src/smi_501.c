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

static void SMI501_ModeSet(ScrnInfoPtr pScrn, MSOCRegPtr mode);
static char *format_integer_base2(int32_t word);
static void SMI501_PrintRegs(ScrnInfoPtr pScrn);
static void SMI501_SetClock(SMIPtr pSmi, int32_t port,
			    int32_t pll, int32_t value);
static void SMI501_WaitVSync(SMIPtr pSmi, int vsync_count);

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

    switch (field(save->power_ctl, mode)) {
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
    save->power_ctl.value = READ_SCR(pSmi, TIMING_CONTROL);

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
		field(mode->system_ctl, dpmsh) = 1;
		field(mode->system_ctl, dpmsv) = 1;
		break;
	    case DPMSModeStandby:
		field(mode->system_ctl, dpmsh) = 0;
		field(mode->system_ctl, dpmsv) = 1;
		break;
	    case DPMSModeSuspend:
		field(mode->system_ctl, dpmsh) = 1;
		field(mode->system_ctl, dpmsv) = 0;
		break;
	    case DPMSModeOff:
		field(mode->system_ctl, dpmsh) = 0;
		field(mode->system_ctl, dpmsv) = 0;
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
    double	mclk;
    int		diff, best, divider, shift, x2_divider, x2_shift;

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

    /* Enable DAC -- 0: enable - 1: disable */
    field(mode->misc_ctl, dac) = 0;

    /* Enable 2D engine */
    field(mode->gate, engine) = 1;
    /* Color space conversion */
    field(mode->gate, csc) = 1;
    /* ZV port */
    field(mode->gate, zv) = 1;
    /* Gpio, Pwm, and I2c */
    field(mode->gate, gpio) = 1;

    /* FIXME fixed at power mode 0 as in the smi sources */
    field(mode->power_ctl, status) = 0;
    field(mode->power_ctl, mode) = 0;

    /* FIXME fixed at 336/3/0 as in the smi sources */
    field(mode->clock, m_select) = 1;
    field(mode->clock, m_divider) = 1;
    field(mode->clock, m_shift) = 0;

    switch (pSmi->MCLK) {
	case 168000:	    /* 336/1/1 */
	    field(mode->clock, m2_select) = 1;
	    field(mode->clock, m2_divider) = 0;
	    field(mode->clock, m2_shift) = 1;
	    break;
	case 96000:	    /* 288/3/0 */
	    field(mode->clock, m2_select) = 0;
	    field(mode->clock, m2_divider) = 1;
	    field(mode->clock, m2_shift) = 0;
	    break;
	case 144000:	    /* 288/1/1 */
	    field(mode->clock, m2_select) = 0;
	    field(mode->clock, m2_divider) = 0;
	    field(mode->clock, m2_shift) = 1;
	    break;
	case 112000:	    /* 336/3/0 */
	default:
	    field(mode->clock, m2_select) = 1;
	    field(mode->clock, m2_divider) = 1;
	    field(mode->clock, m2_shift) = 0;
	    break;
    }

    /* Find clock best matching mode */
    best = 0x7fffffff;
    for (mclk = 288000.0; mclk <= 336000.0; mclk += 48000.0) {
	for (divider = 1; divider <= (pSmi->lcd ? 5 : 3); divider += 2) {
	    /* Start at 1 to match division by 2 */
	    for (shift = 1; shift <= 8; shift++) {
		/* Shift starts at 1 to add a division by two, matching
		 * description of P2XCLK and V2XCLK. */
		diff = (mclk / (divider << shift)) - xf86mode->Clock;
		if (diff < 0)
		    diff = -diff;
		if (diff < best) {
		    x2_shift = shift - 1;
		    x2_divider = divider == 1 ? 0 : divider == 3 ? 1 : 2;

		    /* Remember best diff */
		    best = diff;
		}
	    }
	}
    }

    if (pSmi->lcd) {
	field(mode->clock, p2_select) = mclk == 288000.0 ? 0 : 1;
	field(mode->clock, p2_divider) = x2_divider;
	field(mode->clock, p2_shift) = x2_shift;

	field(mode->panel_display_ctl, format) =
	    pScrn->bitsPerPixel == 8 ? 0 :
	    pScrn->bitsPerPixel == 16 ? 1 : 2;

	field(mode->panel_display_ctl, enable) = 1;
	field(mode->panel_display_ctl, timing) = 1;

	/* FIXME if non clone dual head, and secondary, need to
	 * properly set panel fb address properly ... */
	field(mode->panel_fb_address, address) = 0;
	field(mode->panel_fb_address, mextern) = 0;	/* local memory */
	field(mode->panel_fb_address, pending) = 0;	/* FIXME required? */

	/* >> 4 because of the "unused bits" that should be set to 0 */
	/* FIXME this should be used for virtual size? */
	field(mode->panel_fb_width, offset) = pSmi->Stride >> 4;
	field(mode->panel_fb_width, width) = pSmi->Stride >> 4;

	field(mode->panel_wwidth, x) = 0;
	field(mode->panel_wwidth, width) = xf86mode->HDisplay;

	field(mode->panel_wheight, y) = 0;
	field(mode->panel_wheight, height) = xf86mode->VDisplay;

	field(mode->panel_plane_tl, top) = 0;
	field(mode->panel_plane_tl, left) = 0;

	field(mode->panel_plane_br, right) = xf86mode->HDisplay - 1;
	field(mode->panel_plane_br, bottom) = xf86mode->VDisplay - 1;

	/* 0 means pulse high */
	field(mode->panel_display_ctl, hsync) = !(xf86mode->Flags & V_PHSYNC);
	field(mode->panel_display_ctl, vsync) = !(xf86mode->Flags & V_PVSYNC);

	field(mode->panel_htotal, total) = xf86mode->HTotal - 1;
	field(mode->panel_htotal, end) = xf86mode->HDisplay - 1;

	field(mode->panel_hsync, start) = xf86mode->HSyncStart;
	field(mode->panel_hsync, width) = xf86mode->HSyncEnd -
	    xf86mode->HSyncStart;

	field(mode->panel_vtotal, total) = xf86mode->VTotal - 1;
	field(mode->panel_vtotal, end) = xf86mode->VDisplay - 1;

	field(mode->panel_vsync, start) = xf86mode->VSyncStart;
	field(mode->panel_vsync, height) = xf86mode->VSyncEnd -
	    xf86mode->VSyncStart;
    }
    else {
	field(mode->clock, v2_select) = mclk == 288000.0 ? 0 : 1;
	field(mode->clock, v2_divider) = x2_divider;
	field(mode->clock, v2_shift) = x2_shift;

	field(mode->crt_display_ctl, format) =
	    pScrn->bitsPerPixel == 8 ? 0 :
	    pScrn->bitsPerPixel == 16 ? 1 : 2;

	/* 0: select panel - 1: select crt */
	field(mode->crt_display_ctl, select) = 1;
	field(mode->crt_display_ctl, enable) = 1;

	/* FIXME if non clone dual head, and secondary, need to
	 * properly set crt fb address properly ... */
	field(mode->crt_fb_address, address) = 0;
	field(mode->crt_fb_address, mextern) = 0;	/* local memory */
	field(mode->crt_fb_address, pending) = 0;	/* FIXME required? */

	/* >> 4 because of the "unused fields" that should be set to 0 */
	/* FIXME this should be used for virtual size? */
	field(mode->crt_fb_width, offset) = pSmi->Stride >> 4;
	field(mode->crt_fb_width, width) = pSmi->Stride >> 4;

	/* 0 means pulse high */
	field(mode->crt_display_ctl, hsync) = !(xf86mode->Flags & V_PHSYNC);
	field(mode->crt_display_ctl, vsync) = !(xf86mode->Flags & V_PVSYNC);

	field(mode->crt_htotal, total) = xf86mode->HTotal - 1;
	field(mode->crt_htotal, end) = xf86mode->HDisplay - 1;

	field(mode->crt_hsync, start) = xf86mode->HSyncStart;
	field(mode->crt_hsync, width) = xf86mode->HSyncEnd -
	    xf86mode->HSyncStart;

	field(mode->crt_vtotal, total) = xf86mode->VTotal - 1;
	field(mode->crt_vtotal, end) = xf86mode->VDisplay - 1;

	field(mode->crt_vsync, start) = xf86mode->HSyncStart;
	field(mode->crt_vsync, height) = xf86mode->HSyncEnd -
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

    /* Start with current value */
    clock.value = READ_SCR(pSmi, mode->current_clock);

    field(clock, m_select) = field(mode->clock, m_select);
    pll = clock.value;
    field(clock, m_divider) = field(mode->clock, m_divider);
    field(clock, m_shift) = field(mode->clock, m_shift);
    SMI501_SetClock(pSmi, mode->current_clock, pll, clock.value);

    field(clock, m2_select) = field(mode->clock, m2_select);
    pll = clock.value;
    field(clock, m2_divider) = field(mode->clock, m2_divider);
    field(clock, m2_shift) = field(mode->clock, m2_shift);
    SMI501_SetClock(pSmi, mode->current_clock, pll, clock.value);

    if (pSmi->lcd)
	field(clock, p2_select) = field(mode->clock, p2_select);
    else
	field(clock, v2_select) = field(mode->clock, v2_select);
    pll = clock.value;
    clock.value = mode->clock.value;
    SMI501_SetClock(pSmi, mode->current_clock, pll, clock.value);

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
	field(mode->panel_display_ctl, vdd) = 1;
	WRITE_SCR(pSmi, PANEL_DISPLAY_CTL, mode->panel_display_ctl.value);
	SMI501_WaitVSync(pSmi, 4);

	field(mode->panel_display_ctl, signal) = 1;
	WRITE_SCR(pSmi, PANEL_DISPLAY_CTL, mode->panel_display_ctl.value);
	SMI501_WaitVSync(pSmi, 4);

	field(mode->panel_display_ctl, bias) = 1;
	WRITE_SCR(pSmi, PANEL_DISPLAY_CTL, mode->panel_display_ctl.value);
	SMI501_WaitVSync(pSmi, 4);

	field(mode->panel_display_ctl, fp) = 1;
	WRITE_SCR(pSmi, PANEL_DISPLAY_CTL, mode->panel_display_ctl.value);
	SMI501_WaitVSync(pSmi, 4);

	/* FIXME: No dual head setup, and in this case, crt may
	 * just be another panel */
	/* crt clones panel */
	field(mode->crt_display_ctl, enable) = 1;
	/* 0: select panel - 1: select crt */
	field(mode->crt_display_ctl, select) = 0;
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
    field(mode->system_ctl, burst) = field(mode->system_ctl, burst_read) =
	pSmi->PCIBurst != FALSE;
    field(mode->system_ctl, retry) = pSmi->PCIRetry != FALSE;
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
    WRITE_SCR(pSmi, port, pll);
    SMI501_WaitVSync(pSmi, 1);
}
