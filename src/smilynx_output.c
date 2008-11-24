/*
Copyright (C) 1994-1999 The XFree86 Project, Inc.  All Rights Reserved.
Copyright (C) 2000 Silicon Motion, Inc.  All Rights Reserved.
Copyright (C) 2008 Francisco Jerez. All Rights Reserved.

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
#include "smilynx.h"

static void
SMILynx_OutputDPMS_crt(xf86OutputPtr output, int mode)
{
    ScrnInfoPtr pScrn = output->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    vgaHWPtr	hwp = VGAHWPTR(pScrn);
    CARD8 SR21, SR22, SR31;

    ENTER();

    SR21=VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x21);
    SR22=VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x22);
    SR31=VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x31);

    switch (mode) {
    case DPMSModeOn:
	SR21 &= ~0x88; /* Enable DAC and color palette RAM */
	SR31 |= 0x02; /* Enable CRT display*/
	SR22 = (SR22 & ~0x30) | 0x00; /* Set DPMS state*/
	break;
    case DPMSModeStandby:
	SR21 |= 0x88; /* Disable DAC and color palette RAM */
	SR31 |= 0x02; /* Enable CRT display*/
	SR22 = (SR22 & ~0x30) | 0x10; /* Set DPMS state*/
	break;
    case DPMSModeSuspend:
	SR21 |= 0x88; /* Disable DAC and color palette RAM */
	SR31 |= 0x02; /* Enable CRT display*/
	SR22 = (SR22 & ~0x30) | 0x20; /* Set DPMS state*/
	break;
    case DPMSModeOff:
	SR21 |= 0x88; /* Disable DAC and color palette RAM */
	SR31 &= ~0x02; /* Disable CRT display*/
	SR22 = (SR22 & ~0x30) | 0x30; /* Set DPMS state*/
	break;
    }

    /* Wait for vertical retrace */

    while (hwp->readST01(hwp) & 0x8) ;
    while (!(hwp->readST01(hwp) & 0x8)) ;

    /* Write the registers */
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x21, SR21);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x22, SR22);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x31, SR31);

    LEAVE();

}


static void
SMILynx_OutputDPMS_lcd(xf86OutputPtr output, int mode)
{
    ScrnInfoPtr pScrn = output->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    CARD8 SR21, SR31;

    ENTER();

    SR21=VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x21);
    SR31=VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x31);

    switch (mode) {
    case DPMSModeOn:
	if(pSmi->lcd == 2 /* LCD is DSTN */
	   || pSmi->Dualhead /* Virtual Refresh is enabled */)
	    SR21 &= ~0x10; /* Enable LCD framebuffer read operation and DSTN dithering engine */
	if(pSmi->lcd == 2 /* LCD is DSTN */
	   && !pSmi->Dualhead /* Virtual Refresh is disabled */)
	    SR21 &= ~0x20; /* Enable LCD framebuffer write operation */

	SR31 |= 0x01; /* Enable LCD display*/
	break;
    case DPMSModeStandby:
    case DPMSModeSuspend:
    case DPMSModeOff:
	SR21 |= 0x30; /* Disable LCD framebuffer r/w operation */
	SR31 &= ~0x01; /* Disable LCD display*/
	break;
    }

    /* Write the registers */
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x21, SR21);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x31, SR31);

    LEAVE();


}


static DisplayModePtr
SMILynx_OutputGetModes_crt(xf86OutputPtr output)
{
    ScrnInfoPtr pScrn = output->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    xf86MonPtr pMon = NULL;

    ENTER();

    if(xf86LoaderCheckSymbol("xf86PrintEDID")){ /* Ensure the DDC module is loaded*/
	/* Try VBE */
	if(pSmi->pVbe){
	    pMon = vbeDoEDID(pSmi->pVbe, NULL);
	    if ( pMon != NULL &&
		 (pMon->rawData[0] == 0x00) &&
		 (pMon->rawData[1] == 0xFF) &&
		 (pMon->rawData[2] == 0xFF) &&
		 (pMon->rawData[3] == 0xFF) &&
		 (pMon->rawData[4] == 0xFF) &&
		 (pMon->rawData[5] == 0xFF) &&
		 (pMon->rawData[6] == 0xFF) &&
		 (pMon->rawData[7] == 0x00)) {
		xf86OutputSetEDID(output,pMon);
		LEAVE(xf86OutputGetEDIDModes(output));
	    }
	}

	/* Try DDC1 */
	pMon=SMILynx_ddc1(pScrn);
	if(pMon){
	    xf86OutputSetEDID(output,pMon);
	    LEAVE(xf86OutputGetEDIDModes(output));
	}

	/* Try DDC2 */
	if(pSmi->I2C){
	    pMon=xf86OutputGetEDID(output,pSmi->I2C);
	    if(pMon){
		xf86OutputSetEDID(output,pMon);
		LEAVE(xf86OutputGetEDIDModes(output));
	    }
	}
    }

    LEAVE(NULL);
}

static xf86OutputStatus
SMILynx_OutputDetect_crt(xf86OutputPtr output)
{
    SMIPtr pSmi = SMIPTR(output->scrn);
    vgaHWPtr	hwp = VGAHWPTR(output->scrn);
    CARD8 SR21, SR7D;
    Bool status;

    ENTER();

    SR21 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x21);
    SR7D = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x7D);

    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x21, SR21 & ~0x80); /* Enable DAC */
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x7B, 0x40); /* "TV and RAMDAC Testing Power", Green component */
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x7D, SR7D | 0x10); /* Enable monitor detect */

    /* Wait for vertical retrace */
    while (hwp->readST01(hwp) & 0x8) ;
    while (!(hwp->readST01(hwp) & 0x8)) ;

    status = MMIO_IN8(pSmi->IOBase, 0x3C2) & 0x10;

    /* Restore previous state */
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x21, SR21);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x7D, SR7D);

    if(status)
	LEAVE(XF86OutputStatusConnected);
    else
	LEAVE(XF86OutputStatusDisconnected);
}

static xf86OutputFuncsRec SMILynx_Output0Funcs;
static xf86OutputFuncsRec SMILynx_Output1Funcs;

Bool
SMILynx_OutputPreInit(ScrnInfoPtr pScrn)
{
    SMIPtr pSmi = SMIPTR(pScrn);
    xf86OutputPtr output0=NULL;
    xf86OutputPtr output1=NULL;

    ENTER();

    if(pSmi->Chipset == SMI_COUGAR3DR){
	    /* CRTC0 is LCD */
	    SMI_OutputFuncsInit_base(&SMILynx_Output0Funcs);
	    SMILynx_Output0Funcs.dpms=SMILynx_OutputDPMS_lcd;
	    SMILynx_Output0Funcs.get_modes=SMI_OutputGetModes_native;
	    SMILynx_Output0Funcs.detect=SMI_OutputDetect_lcd;

	    output0=xf86OutputCreate(pScrn,&SMILynx_Output0Funcs,"LVDS");
	    if(!output0)
		LEAVE(FALSE);

	    output0->possible_crtcs = 1 << 0;
	    output0->possible_clones = 0;
	    output0->interlaceAllowed = FALSE;
	    output0->doubleScanAllowed = FALSE;
    }else{
	if(pSmi->Dualhead){
	    /* CRTC0 is LCD*/
	    SMI_OutputFuncsInit_base(&SMILynx_Output0Funcs);
	    SMILynx_Output0Funcs.dpms=SMILynx_OutputDPMS_lcd;
	    SMILynx_Output0Funcs.get_modes=SMI_OutputGetModes_native;
	    SMILynx_Output0Funcs.detect=SMI_OutputDetect_lcd;

	    output0=xf86OutputCreate(pScrn,&SMILynx_Output0Funcs,"LVDS");
	    if(!output0)
		LEAVE(FALSE);

	    output0->possible_crtcs = 1 << 0;
	    output0->possible_clones = 0;
	    output0->interlaceAllowed = FALSE;
	    output0->doubleScanAllowed = FALSE;

	    /* CRTC1 is CRT*/
	    SMI_OutputFuncsInit_base(&SMILynx_Output1Funcs);
	    SMILynx_Output1Funcs.dpms=SMILynx_OutputDPMS_crt;
	    SMILynx_Output1Funcs.get_modes=SMILynx_OutputGetModes_crt;
	    SMILynx_Output1Funcs.detect=SMILynx_OutputDetect_crt;

	    output1=xf86OutputCreate(pScrn,&SMILynx_Output1Funcs,"VGA");
	    if(!output1)
		LEAVE(FALSE);

	    output1->possible_crtcs = 1 << 1;
	    output1->possible_clones = 0;
	    output1->interlaceAllowed = FALSE;
	    output1->doubleScanAllowed = FALSE;
	}else{
	    /* CRTC0 is LCD */
	    SMI_OutputFuncsInit_base(&SMILynx_Output0Funcs);
	    SMILynx_Output0Funcs.dpms=SMILynx_OutputDPMS_lcd;
	    SMILynx_Output0Funcs.get_modes=SMI_OutputGetModes_native;
	    SMILynx_Output0Funcs.detect=SMI_OutputDetect_lcd;

	    output0=xf86OutputCreate(pScrn,&SMILynx_Output0Funcs,"LVDS");
	    if(!output0)
		LEAVE(FALSE);

	    output0->possible_crtcs = 1 << 0;
	    output0->possible_clones = 0;
	    output0->interlaceAllowed = FALSE;
	    output0->doubleScanAllowed = FALSE;
	}
    }

    LEAVE(TRUE);
}

