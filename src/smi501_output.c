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


static void
SMI501_OutputDPMS_lcd(xf86OutputPtr output, int dpms)
{
    ENTER();

    /* Nothing - FIXME */

    LEAVE();
}

static void
SMI501_OutputDPMS_panel(xf86OutputPtr output, int dpms)
{
    ScrnInfoPtr		pScrn = output->scrn;
    SMIPtr		pSmi = SMIPTR(pScrn);
    MSOCRegPtr		mode = pSmi->mode;

    ENTER();

    mode->system_ctl.value = READ_SCR(pSmi, SYSTEM_CTL);
    switch (dpms) {
    case DPMSModeOn:
	SMI501_PowerPanel(pScrn, mode, TRUE);
    case DPMSModeStandby:
	break;
    case DPMSModeSuspend:
	break;
    case DPMSModeOff:
	SMI501_PowerPanel(pScrn, mode, FALSE);
	break;
    }

    LEAVE();
}

static void
SMI501_OutputDPMS_crt(xf86OutputPtr output, int dpms)
{
    ScrnInfoPtr pScrn = output->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    MSOCRegPtr		mode = pSmi->mode;

    ENTER();

    mode->system_ctl.value = READ_SCR(pSmi, SYSTEM_CTL);
    switch (dpms) {
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

    LEAVE();
}

static xf86OutputStatus
SMI501_OutputDetect_crt(xf86OutputPtr output)
{
    ENTER();

    RETURN(XF86OutputStatusDisconnected);
}

static xf86OutputFuncsRec SMI501_Output0Funcs;
static xf86OutputFuncsRec SMI501_Output1Funcs;

Bool
SMI501_OutputPreInit(ScrnInfoPtr pScrn)
{
    SMIPtr		pSmi = SMIPTR(pScrn);
    xf86OutputPtr	output0, output1;

    ENTER();

    /* CRTC0 is LCD */
    SMI_OutputFuncsInit_base(&SMI501_Output0Funcs);
    SMI501_Output0Funcs.dpms		= SMI501_OutputDPMS_lcd;
    SMI501_Output0Funcs.get_modes	= SMI_OutputGetModes_native;
    SMI501_Output0Funcs.detect		= SMI_OutputDetect_lcd;

    output0 = xf86OutputCreate(pScrn, &SMI501_Output0Funcs, "LVDS");
    if (!output0)
	RETURN(FALSE);

    output0->possible_crtcs = 1 << 0;
    output0->possible_clones = 0;
    output0->interlaceAllowed = FALSE;
    output0->doubleScanAllowed = FALSE;

    /* CRTC1 is CRT */
    if (pSmi->Dualhead) {
	SMI_OutputFuncsInit_base(&SMI501_Output1Funcs);
	SMI501_Output1Funcs.dpms	= SMI501_OutputDPMS_crt;
	SMI501_Output1Funcs.get_modes	= SMI_OutputGetModes_native;

	output1 = xf86OutputCreate(pScrn, &SMI501_Output1Funcs, "VGA");
	if (!output1)
	    RETURN(FALSE);

	output1->possible_crtcs = 1 << 1;
	output1->possible_clones = 0;
	output1->interlaceAllowed = FALSE;
	output1->doubleScanAllowed = FALSE;
    }

    RETURN(TRUE);
}
