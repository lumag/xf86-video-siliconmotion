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

static unsigned int SMILynx_ddc1Read(ScrnInfoPtr pScrn);

Bool
SMILynx_HWInit(ScrnInfoPtr pScrn)
{
    SMIPtr pSmi = SMIPTR(pScrn);
    vgaHWPtr	hwp = VGAHWPTR(pScrn);
    int		vgaIOBase  = hwp->IOBase;
    int		vgaCRIndex = vgaIOBase + VGA_CRTC_INDEX_OFFSET;
    int		vgaCRData  = vgaIOBase + VGA_CRTC_DATA_OFFSET;

    ENTER();

    CARD8 SR17 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x17);
    CARD8 SR20 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x20);
    CARD8 SR21 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x21);
    CARD8 SR22 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x22);
    CARD8 SR24 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x24);
    CARD8 SR30 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x30);
    CARD8 SR31 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x31);
    CARD8 SR32 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x32);
    CARD8 SR34 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x34);
    CARD8 SR66 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x66);
    CARD8 SR68 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x68);
    CARD8 SR69 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x69);
    CARD8 SR6A = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x6A);
    CARD8 SR6B = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x6B);

    if (pSmi->PCIBurst) {
	SR17 |= 0x20;
    } else {
	SR17 &= ~0x20;
    }

    /* Disable DAC and LCD framebuffer r/w operation */
    SR21 |= 0xB0;

    /* Power down mode is standby mode, VCLK and MCLK divided by 4 in standby mode */
    SR20  = (SR20 & ~0xB0) | 0x10;

    /* Set DPMS state to Off */
    SR22 |= 0x30;

    /* VESA compliance power down mode */
    SR24 &= ~0x01;

    if (pSmi->Chipset != SMI_COUGAR3DR) {
	/* Select no displays */
	SR31 &= ~0x07;

	/* Enable virtual refresh */
	if(pSmi->Dualhead){
	    SR31 |= 0x80;
	}else{
	    SR31 &= ~0x80;
	}

        /* Disable expansion */
	SR32 &= ~0x03;
	/* Enable autocentering */
	if (SMI_LYNXM_SERIES(pSmi->Chipset))
	    SR32 |= 0x04;
	else
	    SR32 &= ~0x04;

	if (pSmi->lcd == 2) /* Panel is DSTN */
	    SR21 = 0x00;

	/* Enable HW LCD power sequencing */
	SR34 |= 0x80;
    }

    /* Program MCLK */
    if (pSmi->MCLK > 0)
	SMI_CommonCalcClock(pScrn->scrnIndex, pSmi->MCLK,
			    1, 1, 63, 0, 0,
                            pSmi->clockRange.minClock,
                            pSmi->clockRange.maxClock,
                             &SR6A, &SR6B);

    /* use vclk1 */
    SR68 = 0x54;

    if(pSmi->Dualhead){
	/* set LCD to vclk2 */
	SR69 = 0x04;
    }

    /* Gamma correction */
    if (pSmi->Chipset == SMI_LYNX3DM || pSmi->Chipset == SMI_COUGAR3DR) {
	if(pScrn->bitsPerPixel == 8)
	    SR66 = (SR66 & 0x33) | 0x00; /* Both RAMLUT on, 6 bits-RAM */
	else
	    SR66 = (SR66 & 0x33) | 0x04; /* Both RAMLUT on, Gamma correct ON */
    }

    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x17,SR17);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x20,SR20);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x21,SR21);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x22,SR22);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x24,SR24);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x30,SR30);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x31,SR31);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x32,SR32);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x34,SR34);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x66,SR66);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x68,SR68);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x69,SR69);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x6A,SR6A);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x6B,SR6B);

    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0xA0, 0x00);
    VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x33, 0x00);
    VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x3A, 0x00);

    RETURN(TRUE);
}

/*
 * This function performs the inverse of the restore function: It saves all the
 * standard and extended registers that we are going to modify to set up a video
 * mode.
 */

void
SMILynx_Save(ScrnInfoPtr pScrn)
{
    SMIPtr	pSmi = SMIPTR(pScrn);
    int		i;
    CARD32	offset;
    SMIRegPtr	save = pSmi->save;
    vgaHWPtr	hwp = VGAHWPTR(pScrn);
    vgaRegPtr	vgaSavePtr = &hwp->SavedReg;
    int		vgaIOBase  = hwp->IOBase;
    int		vgaCRIndex = vgaIOBase + VGA_CRTC_INDEX_OFFSET;
    int		vgaCRData  = vgaIOBase + VGA_CRTC_DATA_OFFSET;

    ENTER();

    /* Save the standard VGA registers */
    vgaHWSave(pScrn, vgaSavePtr, VGA_SR_ALL);
    save->smiDACMask = VGAIN8(pSmi, VGA_DAC_MASK);
    VGAOUT8(pSmi, VGA_DAC_READ_ADDR, 0);
    for (i = 0; i < 256; i++) {
	save->smiDacRegs[i][0] = VGAIN8(pSmi, VGA_DAC_DATA);
	save->smiDacRegs[i][1] = VGAIN8(pSmi, VGA_DAC_DATA);
	save->smiDacRegs[i][2] = VGAIN8(pSmi, VGA_DAC_DATA);
    }
    for (i = 0, offset = 2; i < 8192; i++, offset += 8)
	save->smiFont[i] = *(pSmi->FBBase + offset);

    /* Now we save all the extended registers we need. */
    save->SR17 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x17);
    save->SR18 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x18);
    save->SR21 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x21);
    save->SR31 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x31);
    save->SR32 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x32);
    save->SR6A = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6A);
    save->SR6B = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6B);
    save->SR81 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x81);
    save->SRA0 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0xA0);

    /* vclk1 */
    save->SR6C = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6C);
    save->SR6D = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6D);
    /* vclk1 control */
    save->SR68 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x68);

    if (pSmi->Dualhead) {
	/* dualhead stuff */
	save->SR22 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x22);
	save->SR40 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x40);
	save->SR41 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x41);
	save->SR42 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x42);
	save->SR43 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x43);
	save->SR44 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x44);
	save->SR45 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x45);
	save->SR48 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x48);
	save->SR49 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x49);
	save->SR4A = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x4A);
	save->SR4B = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x4B);
	save->SR4C = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x4C);
	/* PLL2 stuff */
	save->SR69 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x69);
	save->SR6E = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6E);
	save->SR6F = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6F);
    }

    if (SMI_LYNXM_SERIES(pSmi->Chipset)) {
	/* Save primary registers */
	save->CR90[14] = VGAIN8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x9E);
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x9E, save->CR90[14] & ~0x20);

	for (i = 0; i < 16; i++) {
	    save->CR90[i] = VGAIN8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x90 + i);
	}
	save->CR33 = VGAIN8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x33);
	save->CR3A = VGAIN8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x3A);
	for (i = 0; i < 14; i++) {
	    save->CR40[i] = VGAIN8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x40 + i);
	}

	/* Save secondary registers */
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x9E, save->CR90[14] | 0x20);
	save->CR33_2 = VGAIN8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x33);
	for (i = 0; i < 14; i++) {
	    save->CR40_2[i] = VGAIN8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x40 + i);
	}
	save->CR9F_2 = VGAIN8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x9F);

	/* Save common registers */
	for (i = 0; i < 14; i++) {
	    save->CRA0[i] = VGAIN8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0xA0 + i);
	}

	/* PDR#1069 */
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x9E, save->CR90[14]);
    }
    else {
	save->CR33 = VGAIN8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x33);
	save->CR3A = VGAIN8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x3A);
	for (i = 0; i < 14; i++) {
	    save->CR40[i] = VGAIN8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x40 + i);
	}
    }

    /* CZ 2.11.2001: for gamma correction (TODO: other chipsets?) */
    if ((pSmi->Chipset == SMI_LYNX3DM) || (pSmi->Chipset == SMI_COUGAR3DR)) {
	save->CCR66 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x66);
    }
    /* end CZ */

    save->DPR10 = READ_DPR(pSmi, 0x10);
    save->DPR1C = READ_DPR(pSmi, 0x1C);
    save->DPR20 = READ_DPR(pSmi, 0x20);
    save->DPR24 = READ_DPR(pSmi, 0x24);
    save->DPR28 = READ_DPR(pSmi, 0x28);
    save->DPR2C = READ_DPR(pSmi, 0x2C);
    save->DPR30 = READ_DPR(pSmi, 0x30);
    save->DPR3C = READ_DPR(pSmi, 0x3C);
    save->DPR40 = READ_DPR(pSmi, 0x40);
    save->DPR44 = READ_DPR(pSmi, 0x44);

    save->VPR00 = READ_VPR(pSmi, 0x00);
    save->VPR0C = READ_VPR(pSmi, 0x0C);
    save->VPR10 = READ_VPR(pSmi, 0x10);

    if (pSmi->Chipset == SMI_COUGAR3DR) {
	save->FPR00_ = READ_FPR(pSmi, FPR00);
	save->FPR0C_ = READ_FPR(pSmi, FPR0C);
	save->FPR10_ = READ_FPR(pSmi, FPR10);
    }

    save->CPR00 = READ_CPR(pSmi, 0x00);

    if (!pSmi->ModeStructInit) {
	/* XXX Should check the return value of vgaHWCopyReg() */
	vgaHWCopyReg(&hwp->ModeReg, vgaSavePtr);
	memcpy(pSmi->mode, save, sizeof(SMIRegRec));
	pSmi->ModeStructInit = TRUE;
    }

    if (pSmi->useBIOS && pSmi->pInt10 != NULL) {
	pSmi->pInt10->num = 0x10;
	pSmi->pInt10->ax = 0x0F00;
	xf86ExecX86int10(pSmi->pInt10);
	save->mode = pSmi->pInt10->ax & 0x007F;
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Current mode 0x%02X.\n",
		   save->mode);
    }

    if (xf86GetVerbosity() > 1) {
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, VERBLEV,
		       "Saved current video mode.  Register dump:\n");
	SMI_PrintRegs(pScrn);
    }

    LEAVE();
}

/*
 * This function is used to restore a video mode. It writes out all of the
 * standard VGA and extended registers needed to setup a video mode.
 */

void
SMILynx_WriteMode(ScrnInfoPtr pScrn, vgaRegPtr vgaSavePtr, SMIRegPtr restore)
{
    SMIPtr	pSmi = SMIPTR(pScrn);

    ENTER();

    int		i;
    CARD8		tmp;
    CARD32		offset;
    vgaHWPtr	hwp = VGAHWPTR(pScrn);
    int		vgaIOBase  = hwp->IOBase;
    int		vgaCRIndex = vgaIOBase + VGA_CRTC_INDEX_OFFSET;
    int		vgaCRData  = vgaIOBase + VGA_CRTC_DATA_OFFSET;

    /* Wait for engine to become idle */
    if (pSmi->IsSwitching)
	WaitIdle();

    if (pSmi->useBIOS && pSmi->pInt10 != NULL && restore->mode != 0) {
	pSmi->pInt10->num = 0x10;
	pSmi->pInt10->ax = restore->mode | 0x80;
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Setting mode 0x%02X\n",
		   restore->mode);
	xf86ExecX86int10(pSmi->pInt10);

	/* Enable linear mode. */
	outb(pSmi->PIOBase + VGA_SEQ_INDEX, 0x18);
	tmp = inb(pSmi->PIOBase + VGA_SEQ_DATA);
	outb(pSmi->PIOBase + VGA_SEQ_DATA, tmp | 0x01);

	/* Enable DPR/VPR registers. */
	tmp = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x21);
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x21, tmp & ~0x03);
    } else {
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x17, restore->SR17);
	tmp = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x18) & ~0x1F;
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x18, tmp |
		      (restore->SR18 & 0x1F));
	tmp = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x21);
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x21, tmp & ~0x03);
	tmp = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x31) & ~0xC0;
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x31, tmp |
		      (restore->SR31 & 0xC0));
	tmp = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x32) & ~0x07;
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x32, tmp |
		      (restore->SR32 & 0x07));
	if (restore->SR6B != 0xFF) {
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6A, restore->SR6A);
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6B, restore->SR6B);
	}
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x81, restore->SR81);
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0xA0, restore->SRA0);

	/* Restore the standard VGA registers */
	vgaHWRestore(pScrn, vgaSavePtr, VGA_SR_ALL);
	if (restore->smiDACMask) {
	    VGAOUT8(pSmi, VGA_DAC_MASK, restore->smiDACMask);
	} else {
	    VGAOUT8(pSmi, VGA_DAC_MASK, 0xFF);
	}
	VGAOUT8(pSmi, VGA_DAC_WRITE_ADDR, 0);
	for (i = 0; i < 256; i++) {
	    VGAOUT8(pSmi, VGA_DAC_DATA, restore->smiDacRegs[i][0]);
	    VGAOUT8(pSmi, VGA_DAC_DATA, restore->smiDacRegs[i][1]);
	    VGAOUT8(pSmi, VGA_DAC_DATA, restore->smiDacRegs[i][2]);
	}
	for (i = 0, offset = 2; i < 8192; i++, offset += 8) {
	    *(pSmi->FBBase + offset) = restore->smiFont[i];
	}

	if (SMI_LYNXM_SERIES(pSmi->Chipset)) {
	    /* Restore secondary registers */
	    VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x9E,
			  restore->CR90[14] | 0x20);

	    VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x33, restore->CR33_2);
	    for (i = 0; i < 14; i++) {
		VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x40 + i,
			      restore->CR40_2[i]);
	    }
	    VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x9F, restore->CR9F_2);

	    /* Restore primary registers */
	    VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x9E,
			  restore->CR90[14] & ~0x20);

	    VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x33, restore->CR33);
	    VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x3A, restore->CR3A);
	    for (i = 0; i < 14; i++) {
		VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x40 + i,
			      restore->CR40[i]);
	    }
	    for (i = 0; i < 16; i++) {
		if (i != 14) {
		    VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x90 + i,
				  restore->CR90[i]);
		}
	    }
	    VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x9E, restore->CR90[14]);

	    /* Restore common registers */
	    for (i = 0; i < 14; i++) {
		VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0xA0 + i,
			      restore->CRA0[i]);
	    }
	}

	/* Restore the standard VGA registers */
	if (xf86IsPrimaryPci(pSmi->PciInfo)) {
	    vgaHWRestore(pScrn, vgaSavePtr, VGA_SR_CMAP | VGA_SR_FONTS);
	}

	if (restore->modeInit)
	    vgaHWRestore(pScrn, vgaSavePtr, VGA_SR_ALL);

	if (!SMI_LYNXM_SERIES(pSmi->Chipset)) {
	    VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x33, restore->CR33);
	    VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x3A, restore->CR3A);
	    for (i = 0; i < 14; i++) {
		VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x40 + i,
			      restore->CR40[i]);
	    }
	}

	/* vclk1 */
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x68, restore->SR68);
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6C, restore->SR6C);
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6D, restore->SR6D);

	if (pSmi->Dualhead) {

	    /* TFT panel uses FIFO1, DSTN panel uses FIFO1 for upper panel and
	     * FIFO2 for lower panel.  I don't have a DSTN panel, so it's untested.
	     * -- AGD
	     */

	    /* PLL2 regs */

	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x69, restore->SR69);

	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6E, restore->SR6E);
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6F, restore->SR6F);

	    /* setting SR21 bit 2 disables ZV circuitry,
	     * if ZV is needed, SR21 = 0x20
	     */
	    /* enable DAC, PLL, etc. */
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x21, restore->SR21);

	    /* clear DPMS state */
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x22, restore->SR22);

	    /* enable virtual refresh and LCD and CRT outputs */
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x31, restore->SR31);

	    /* FIFO1 Read Offset */
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x44, restore->SR44);
	    /* FIFO2 Read Offset */
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x4B, restore->SR4B);
	    /* FIFO1/2 Read Offset overflow */
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x4C, restore->SR4C);

	    /* FIFO Write Offset */
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x48, restore->SR48);
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x49, restore->SR49);

	    /* set FIFO levels */
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x4A, restore->SR4A);

	    VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x33, restore->CR33);

	}
    }

    /* CZ 2.11.2001: for gamma correction (TODO: other chipsets?) */
    if ((pSmi->Chipset == SMI_LYNX3DM) || (pSmi->Chipset == SMI_COUGAR3DR)) {
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x66, restore->CCR66);
    }
    /* end CZ */

    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x81, 0x00);

    /* Reset the graphics engine */
    WRITE_DPR(pSmi, 0x10, restore->DPR10);
    WRITE_DPR(pSmi, 0x1C, restore->DPR1C);
    WRITE_DPR(pSmi, 0x20, restore->DPR20);
    WRITE_DPR(pSmi, 0x24, restore->DPR24);
    WRITE_DPR(pSmi, 0x28, restore->DPR28);
    WRITE_DPR(pSmi, 0x2C, restore->DPR2C);
    WRITE_DPR(pSmi, 0x30, restore->DPR30);
    WRITE_DPR(pSmi, 0x3C, restore->DPR3C);
    WRITE_DPR(pSmi, 0x40, restore->DPR40);
    WRITE_DPR(pSmi, 0x44, restore->DPR44);

    /* write video controller regs */
    WRITE_VPR(pSmi, 0x00, restore->VPR00);
    WRITE_VPR(pSmi, 0x0C, restore->VPR0C);
    WRITE_VPR(pSmi, 0x10, restore->VPR10);

    if(pSmi->Chipset == SMI_COUGAR3DR) {
	WRITE_FPR(pSmi, FPR00, restore->FPR00_);
	WRITE_FPR(pSmi, FPR0C, restore->FPR0C_);
	WRITE_FPR(pSmi, FPR10, restore->FPR10_);
    }

    WRITE_CPR(pSmi, 0x00, restore->CPR00);

    if (xf86GetVerbosity() > 1) {
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, VERBLEV,
		       "Done restoring mode.  Register dump:\n");
	SMI_PrintRegs(pScrn);
    }

    vgaHWProtect(pScrn, FALSE);

    LEAVE();
}


/*
 * SMI_DisplayPowerManagementSet -- Sets VESA Display Power Management
 * Signaling (DPMS) Mode.
 */
void
SMILynx_DisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode,
							  int flags)
{
    SMIPtr	pSmi = SMIPTR(pScrn);
    vgaHWPtr	hwp = VGAHWPTR(pScrn);

    ENTER();

    /* If we already are in the requested DPMS mode, just return */
    if (pSmi->CurrentDPMS != PowerManagementMode) {
	/* Read the required SR registers for the DPMS handler */
	CARD8 SR01 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x01);
	CARD8 SR23 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x23);

	switch (PowerManagementMode) {
	    case DPMSModeOn:
		SR01 &= ~0x20; /* Screen on */
		SR23 &= ~0xC0; /* Disable chip activity detection */
		break;
	    case DPMSModeStandby:
	    case DPMSModeSuspend:
	    case DPMSModeOff:
		SR01 |= 0x20; /*Screen off*/
		SR23  = (SR23 & ~0x07) | 0xD8; /*Enable chip activity detection
						 Enable internal auto-standby mode
					         Enable both IO Write and Host Memory write detect
					         0 minutes timeout */
		break;
	}

	/* Wait for vertical retrace */
	while (hwp->readST01(hwp) & 0x8) ;
	while (!(hwp->readST01(hwp) & 0x8)) ;

	/* Write the registers */
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x01, SR01);
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x23, SR23);

	/* Set the DPMS mode to every output and CRTC */
	xf86DPMSSet(pScrn, PowerManagementMode, flags);

	/* Save the current power state */
	pSmi->CurrentDPMS = PowerManagementMode;
    }

    LEAVE();
}

static unsigned int
SMILynx_ddc1Read(ScrnInfoPtr pScrn)
{
    register vgaHWPtr hwp = VGAHWPTR(pScrn);
    SMIPtr pSmi = SMIPTR(pScrn);
    unsigned int ret;

    ENTER();

    while (hwp->readST01(hwp) & 0x8) ;
    while (!(hwp->readST01(hwp) & 0x8)) ;

    ret = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x72) & 0x08;

    RETURN(ret);
}

xf86MonPtr
SMILynx_ddc1(ScrnInfoPtr pScrn)
{
    SMIPtr pSmi = SMIPTR(pScrn);
    xf86MonPtr pMon;
    unsigned char tmp;

    ENTER();

    tmp = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x72);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x72, tmp | 0x20);

    pMon = xf86PrintEDID(xf86DoEDID_DDC1(pScrn->scrnIndex,
					 vgaHWddc1SetSpeedWeak(),
					 SMILynx_ddc1Read));
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x72, tmp);

    RETURN(pMon);
}


/* This function is used to debug, it prints out the contents of Lynx regs */
void
SMILynx_PrintRegs(ScrnInfoPtr pScrn)
{
    unsigned char i;
    SMIPtr pSmi = SMIPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    int vgaCRIndex = hwp->IOBase + VGA_CRTC_INDEX_OFFSET;
    int vgaCRReg   = hwp->IOBase + VGA_CRTC_DATA_OFFSET;
    int vgaStatus  = hwp->IOBase + VGA_IN_STAT_1_OFFSET;

    xf86ErrorFVerb(VERBLEV, "MISCELLANEOUS OUTPUT\n    %02X\n",
		   VGAIN8(pSmi, VGA_MISC_OUT_R));

    xf86ErrorFVerb(VERBLEV, "\nSEQUENCER\n"
		   "    x0 x1 x2 x3  x4 x5 x6 x7  x8 x9 xA xB  xC xD xE xF");
    for (i = 0x00; i <= 0xAF; i++) {
	if ((i & 0xF) == 0x0) xf86ErrorFVerb(VERBLEV, "\n%02X|", i);
	if ((i & 0x3) == 0x0) xf86ErrorFVerb(VERBLEV, " ");
	xf86ErrorFVerb(VERBLEV, "%02X ",
		       VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, i));
    }

    xf86ErrorFVerb(VERBLEV, "\n\nCRT CONTROLLER\n"
		   "    x0 x1 x2 x3  x4 x5 x6 x7  x8 x9 xA xB  xC xD xE xF");
    for (i = 0x00; i <= 0xAD; i++) {
	if (i == 0x20) i = 0x30;
	if (i == 0x50) i = 0x90;
	if ((i & 0xF) == 0x0) xf86ErrorFVerb(VERBLEV, "\n%02X|", i);
	if ((i & 0x3) == 0x0) xf86ErrorFVerb(VERBLEV, " ");
	xf86ErrorFVerb(VERBLEV, "%02X ",
		       VGAIN8_INDEX(pSmi, vgaCRIndex, vgaCRReg, i));
    }

    xf86ErrorFVerb(VERBLEV, "\n\nGRAPHICS CONTROLLER\n"
		   "    x0 x1 x2 x3  x4 x5 x6 x7  x8 x9 xA xB  xC xD xE xF");
    for (i = 0x00; i <= 0x08; i++) {
	if ((i & 0xF) == 0x0) xf86ErrorFVerb(VERBLEV, "\n%02X|", i);
	if ((i & 0x3) == 0x0) xf86ErrorFVerb(VERBLEV, " ");
	xf86ErrorFVerb(VERBLEV, "%02X ",
		       VGAIN8_INDEX(pSmi, VGA_GRAPH_INDEX, VGA_GRAPH_DATA, i));
    }

    xf86ErrorFVerb(VERBLEV, "\n\nATTRIBUTE 0CONTROLLER\n"
		   "    x0 x1 x2 x3  x4 x5 x6 x7  x8 x9 xA xB  xC xD xE xF");
    for (i = 0x00; i <= 0x14; i++) {
	(void) VGAIN8(pSmi, vgaStatus);
	if ((i & 0xF) == 0x0) xf86ErrorFVerb(VERBLEV, "\n%02X|", i);
	if ((i & 0x3) == 0x0) xf86ErrorFVerb(VERBLEV, " ");
	xf86ErrorFVerb(VERBLEV, "%02X ",
		       VGAIN8_INDEX(pSmi, VGA_ATTR_INDEX, VGA_ATTR_DATA_R, i));
    }
    (void) VGAIN8(pSmi, vgaStatus);
    VGAOUT8(pSmi, VGA_ATTR_INDEX, 0x20);
}
