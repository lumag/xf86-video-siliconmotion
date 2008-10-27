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
SMILynx_CrtcVideoInit_crt(xf86CrtcPtr crtc)
{
    ScrnInfoPtr pScrn=crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    int pitch;

    ENTER();

    switch (pScrn->bitsPerPixel) {
    case 8:
	WRITE_VPR(pSmi, 0x00, 0x00000000);
	break;
    case 16:
	WRITE_VPR(pSmi, 0x00, 0x00020000);
	break;
    case 24:
	WRITE_VPR(pSmi, 0x00, 0x00040000);
	break;
    case 32:
	WRITE_VPR(pSmi, 0x00, 0x00030000);
	break;
    }

    pitch = (crtc->rotatedData? crtc->mode.HDisplay : pScrn->displayWidth) * pSmi->Bpp;
    pitch = (pitch + 15) & ~15;

    WRITE_VPR(pSmi, 0x10, (crtc->mode.HDisplay * pSmi->Bpp) >> 3 << 16 | pitch >> 3);

    LEAVE();
}

static void
SMILynx_CrtcVideoInit_lcd(xf86CrtcPtr crtc)
{
    ScrnInfoPtr pScrn=crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    CARD8 SR31;
    CARD16 fifo_readoffset,fifo_writeoffset;

    ENTER();

    /* Set display depth */
    SR31=VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x31);
    if (pScrn->bitsPerPixel > 8)
	SR31 |= 0x40; /* 16 bpp */
    else
	SR31 &= ~0x40; /* 8 bpp */
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x31,SR31);

    /* FIFO1/2 Read Offset*/
    fifo_readoffset = (crtc->rotatedData? crtc->mode.HDisplay : pScrn->displayWidth) * pSmi->Bpp;
    fifo_readoffset = ((fifo_readoffset + 15) & ~15) >> 3;

    /* FIFO1 Read Offset */
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x44, fifo_readoffset & 0x000000FF);
    /* FIFO2 Read Offset */
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x4B, fifo_readoffset & 0x000000FF);
    /* FIFO1/2 Read Offset overflow */
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x4C, (((fifo_readoffset & 0x00000300) >> 8) << 2) |
		  (((fifo_readoffset & 0x00000300) >> 8) << 6));

    /* FIFO Write Offset */
    fifo_writeoffset = crtc->mode.HDisplay * pSmi->Bpp >> 3;
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x48, fifo_writeoffset & 0x000000FF);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x49, (fifo_writeoffset & 0x00000300) >> 8);

    /* set FIFO levels */
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x4A, 0x41);

    LEAVE();
}

static void
SMI730_CrtcVideoInit(xf86CrtcPtr crtc)
{
    ScrnInfoPtr pScrn=crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    int pitch;

    ENTER();

    switch (pScrn->bitsPerPixel) {
    case 8:
	WRITE_VPR(pSmi, 0x00, 0x00000000);
	WRITE_FPR(pSmi, FPR00, 0x00080000);
	break;
    case 16:
	WRITE_VPR(pSmi, 0x00, 0x00020000);
	WRITE_FPR(pSmi, FPR00, 0x000A0000);
	break;
    case 24:
	WRITE_VPR(pSmi, 0x00, 0x00040000);
	WRITE_FPR(pSmi, FPR00, 0x000C0000);
	break;
    case 32:
	WRITE_VPR(pSmi, 0x00, 0x00030000);
	WRITE_FPR(pSmi, FPR00, 0x000B0000);
	break;
    }

    pitch = (crtc->rotatedData? crtc->mode.HDisplay : pScrn->displayWidth) * pSmi->Bpp;
    pitch = (pitch + 15) & ~15;

    WRITE_VPR(pSmi, 0x10, (crtc->mode.HDisplay * pSmi->Bpp) >> 3 << 16 | pitch >> 3);
    WRITE_FPR(pSmi, FPR10, (crtc->mode.HDisplay * pSmi->Bpp) >> 3 << 16 | pitch >> 3);

    LEAVE();
}

static void
SMILynx_CrtcAdjustFrame(xf86CrtcPtr crtc, int x, int y)
{
    ScrnInfoPtr pScrn=crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    xf86CrtcConfigPtr crtcConf = XF86_CRTC_CONFIG_PTR(pScrn);
    CARD32 Base;

    ENTER();

    if(crtc->rotatedData)
	Base = (char*)crtc->rotatedData - (char*)pSmi->FBBase;
    else
	Base = pSmi->FBOffset + (x + y * pScrn->displayWidth) * pSmi->Bpp;


    if (SMI_LYNX3D_SERIES(pSmi->Chipset) ||
	     SMI_COUGAR_SERIES(pSmi->Chipset)) {
	Base = (Base + 15) & ~15;
	while ((Base % pSmi->Bpp) > 0) {
	    Base -= 16;
	}
    } else {
	Base = (Base + 7) & ~7;
	while ((Base % pSmi->Bpp) > 0)
	    Base -= 8;
    }

    Base >>= 3;

    if(SMI_COUGAR_SERIES(pSmi->Chipset)){
	WRITE_VPR(pSmi, 0x0C, Base);
	WRITE_FPR(pSmi, FPR0C, Base);
    }else{
	if(pSmi->Dualhead && crtc == crtcConf->crtc[0]){
	    /* LCD */

	    /* FIFO1 read start address */
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x40,
			  (Base & 0x000000FF));
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x41,
			  ((Base & 0x0000FF00) >> 8));

	    /* FIFO2 read start address */
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x42,
			  (Base & 0x000000FF));
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x43,
			  ((Base & 0x0000FF00) >> 8));

	    /* FIFO1/2 read start address overflow */
	    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x45,
			  ((Base & 0x000F0000) >> 16) | (((Base & 0x000F0000) >> 16) << 4));
	}else{
	    /* CRT or single head */
	    WRITE_VPR(pSmi, 0x0C, Base);
	}
    }

    LEAVE();
}

static void
SMILynx_CrtcModeSet_vga(xf86CrtcPtr crtc,
	    DisplayModePtr mode,
	    DisplayModePtr adjusted_mode,
	    int x, int y)
{
    ScrnInfoPtr pScrn=crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr vganew = &hwp->ModeReg;
    CARD8 SR6C, SR6D;

    ENTER();

    /* Initialize Video Processor Registers */

    SMICRTC(crtc)->video_init(crtc);
    SMILynx_CrtcAdjustFrame(crtc, x,y);


    /* Program the PLL */

    /* calculate vclk1 */
    if (SMI_LYNX_SERIES(pSmi->Chipset)) {
        SMI_CommonCalcClock(pScrn->scrnIndex, mode->Clock,
			1, 1, 63, 0, 3,
                        pSmi->clockRange.minClock,
                        pSmi->clockRange.maxClock,
                        &SR6C, &SR6D);
    } else {
        SMI_CommonCalcClock(pScrn->scrnIndex, mode->Clock,
			1, 1, 63, 0, 1,
                        pSmi->clockRange.minClock,
                        pSmi->clockRange.maxClock,
                        &SR6C, &SR6D);
    }

    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6C, SR6C);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6D, SR6D);


    /* Adjust mode timings */

    if (!vgaHWInit(pScrn, mode)) {
	LEAVE();
	return;
    }

    if ((mode->HDisplay == 640) && SMI_LYNXM_SERIES(pSmi->Chipset)) {
	vganew->MiscOutReg &= ~0x0C;
    } else {
	vganew->MiscOutReg |= 0x0C;
    }
    vganew->MiscOutReg |= 0x20;

    if(pSmi->Chipset != SMI_COUGAR3DR){
	/* Enable LCD */
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x31,
		      VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x31) | 0x01);
    }

    vgaHWRestore(pScrn, vganew, VGA_SR_MODE);

    LEAVE();
}

static void
SMILynx_CrtcModeSet_crt(xf86CrtcPtr crtc,
	    DisplayModePtr mode,
	    DisplayModePtr adjusted_mode,
	    int x, int y)
{
    ScrnInfoPtr pScrn=crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    int vgaIOBase  = hwp->IOBase;
    int	vgaCRIndex = vgaIOBase + VGA_CRTC_INDEX_OFFSET;
    int	vgaCRData  = vgaIOBase + VGA_CRTC_DATA_OFFSET;
    CARD8 SR6C, SR6D;

    ENTER();

    /* Initialize Video Processor Registers */

    SMILynx_CrtcVideoInit_crt(crtc);
    SMILynx_CrtcAdjustFrame(crtc, x,y);


    /* Program the PLL */

    /* calculate vclk1 */
    if (SMI_LYNX_SERIES(pSmi->Chipset)) {
        SMI_CommonCalcClock(pScrn->scrnIndex, mode->Clock,
			1, 1, 63, 0, 3,
                        pSmi->clockRange.minClock,
                        pSmi->clockRange.maxClock,
                        &SR6C, &SR6D);
    } else {
        SMI_CommonCalcClock(pScrn->scrnIndex, mode->Clock,
			1, 1, 63, 0, 1,
                        pSmi->clockRange.minClock,
                        pSmi->clockRange.maxClock,
                        &SR6C, &SR6D);
    }

    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6C, SR6C);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6D, SR6D);


    /* Adjust mode timings */
    /* In virtual refresh mode, the CRT timings are controlled through
       the shadow VGA registers */

    /* Select primary set of shadow registers */
    VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x9E,
		  VGAIN8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x9E) & ~0x20);

    {
	unsigned long HTotal=(mode->CrtcHTotal>>3)-5;
	unsigned long HDisplay=(mode->CrtcHDisplay>>3)-1;
	unsigned long HBlankStart=(mode->CrtcHBlankStart>>3)-1;
	unsigned long HBlankEnd=(mode->CrtcHBlankEnd>>3)-1;
	unsigned long HSyncStart=mode->CrtcHSyncStart>>3;
	unsigned long HSyncEnd=mode->CrtcHSyncEnd>>3;
	unsigned long VTotal=mode->CrtcVTotal-2;
	unsigned long VDisplay=mode->CrtcVDisplay-1;
	unsigned long VBlankStart=mode->CrtcVBlankStart-1;
	unsigned long VBlankEnd=mode->CrtcVBlankEnd-1;
	unsigned long VSyncStart=mode->CrtcVSyncStart;
	unsigned long VSyncEnd=mode->CrtcVSyncEnd;

	if((mode->CrtcHBlankEnd >> 3) == (mode->CrtcHTotal >> 3)) HBlankEnd=0;
	if(mode->CrtcVBlankEnd == mode->CrtcVTotal) VBlankEnd=0;

	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x40, HTotal & 0xFF );
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x41, HBlankStart & 0xFF);
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x42, HBlankEnd & 0x1F);
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x43, HSyncStart & 0xFF);
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x44,
		      (HBlankEnd & 0x20) >> 5 << 7 |
		      (HSyncEnd & 0x1F) );
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x45, VTotal & 0xFF );
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x46, VBlankStart & 0xFF );
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x47, VBlankEnd & 0xFF );
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x48, VSyncStart & 0xFF );
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x49, VSyncEnd & 0x0F );
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x4A,
		      (VSyncStart & 0x200) >> 9 << 7 |
		      (VDisplay & 0x200) >> 9 << 6 |
		      (VTotal & 0x200) >> 9 << 5 |
		      (VBlankStart & 0x100) >> 8 << 3 |
		      (VSyncStart & 0x100) >> 8 << 2 |
		      (VDisplay & 0x100) >> 8 << 1 |
		      (VTotal & 0x100) >> 8 << 0 );
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x4B,
		      ((mode->Flags & V_NVSYNC)?1:0) << 7 |
		      ((mode->Flags & V_NHSYNC)?1:0) << 6 |
		      (VBlankStart & 0x200) >> 9 << 5 );
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x4C, HDisplay & 0xFF );
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x4D, VDisplay & 0xFF );
	VGAOUT8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x33,
		      (HBlankEnd & 0xC0) >> 6 << 5 |
		      (VBlankEnd & 0x300) >> 8 << 3);

    }

    LEAVE();
}

static void
SMILynx_CrtcModeSet_lcd(xf86CrtcPtr crtc,
	    DisplayModePtr mode,
	    DisplayModePtr adjusted_mode,
	    int x, int y)
{
    ScrnInfoPtr pScrn=crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    CARD8 SR32, SR6E, SR6F;

    ENTER();

    /* Initialize the flat panel video processor */

    SMILynx_CrtcVideoInit_lcd(crtc);
    SMILynx_CrtcAdjustFrame(crtc,x,y);


    /* Program the PLL */

    /* calculate vclk2 */
    if (SMI_LYNX_SERIES(pSmi->Chipset)) {
        SMI_CommonCalcClock(pScrn->scrnIndex, mode->Clock,
			1, 1, 63, 0, 3,
                        pSmi->clockRange.minClock,
                        pSmi->clockRange.maxClock,
                        &SR6E, &SR6F);
    } else {
        SMI_CommonCalcClock(pScrn->scrnIndex, mode->Clock,
			1, 1, 63, 0, 1,
                        pSmi->clockRange.minClock,
                        pSmi->clockRange.maxClock,
                        &SR6E, &SR6F);
    }
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6E, SR6E);
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x6F, SR6F);


    /* Adjust mode timings */
    {
	unsigned long HTotal=(mode->CrtcHTotal>>3)-1;
	unsigned long HDisplay=(mode->CrtcHDisplay>>3)-1;
	unsigned long HSyncStart=(mode->CrtcHSyncStart>>3);
	unsigned long HSyncWidth=((mode->CrtcHSyncEnd - mode->CrtcHSyncStart) >> 3) - 1;
	unsigned long VTotal=mode->CrtcVTotal-1;
	unsigned long VDisplay=mode->CrtcVDisplay-1;
	unsigned long VSyncStart=mode->CrtcVSyncStart-1;
	unsigned long VSyncWidth=mode->CrtcVSyncEnd - mode->CrtcVSyncStart - 1;

	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x50,
		      (VTotal & 0x700) >> 8 << 1 |
		      (HSyncStart & 0x100) >> 8 << 0);
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x51,
		      (VSyncStart & 0x700) >> 8 << 5 |
		      (VDisplay & 0x700) >> 8 << 2 |
		      (HDisplay & 0x100) >> 8 << 1 |
		      (HTotal & 0x100) >> 8 << 0);
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x52, HTotal & 0xFF);
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x53, HDisplay & 0xFF);
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x54, HSyncStart & 0xFF);
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x55, VTotal & 0xFF);
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x56, VDisplay & 0xFF);
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x57, VSyncStart & 0xFF);
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x5A,
		      (HSyncWidth & 0x1F) << 3 |
		      (VSyncWidth & 0x07) << 0);

	/* XXX - Why is the polarity hardcoded here? */
	SR32=VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x32);
	SR32 &= ~0x18;
	if (mode->HDisplay == 800) {
	    SR32 |= 0x18;
	}
	if ((mode->HDisplay == 1024) && SMI_LYNXM_SERIES(pSmi->Chipset)) {
	    SR32 |= 0x18;
	}
	VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x32,SR32);
    }

    LEAVE();
}

static void
SMILynx_CrtcLoadLUT_crt(xf86CrtcPtr crtc)
{
    ScrnInfoPtr pScrn = crtc->scrn;
    SMIPtr pSmi = SMIPTR(pScrn);
    SMICrtcPrivatePtr crtcPriv = SMICRTC(crtc);
    CARD8 SR66;
    int i;

    ENTER();

    SR66 = VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x66);

    /* Write CRT RAM only */
    VGAOUT8_INDEX(pSmi, VGA_SEQ_INDEX,VGA_SEQ_DATA,0x66,(SR66 & ~0x30) | 0x20);

    for(i=0;i<256;i++){
	VGAOUT8(pSmi, VGA_DAC_WRITE_ADDR, i);
	VGAOUT8(pSmi, VGA_DAC_DATA, crtcPriv->lut_r[i] >> 8);
	VGAOUT8(pSmi, VGA_DAC_DATA, crtcPriv->lut_g[i] >> 8);
	VGAOUT8(pSmi, VGA_DAC_DATA, crtcPriv->lut_b[i] >> 8);
    }


    LEAVE();
}

static void
SMILynx_CrtcLoadLUT_lcd(xf86CrtcPtr crtc)
{
    ENTER();

    /* XXX - Is it possible to load LCD LUT in Virtual Refresh mode? */

    LEAVE();
}

static xf86CrtcFuncsRec SMILynx_Crtc0Funcs;
static SMICrtcPrivateRec SMILynx_Crtc0Priv;
static xf86CrtcFuncsRec SMILynx_Crtc1Funcs;
static SMICrtcPrivateRec SMILynx_Crtc1Priv;

Bool
SMILynx_CrtcPreInit(ScrnInfoPtr pScrn)
{
    SMIPtr pSmi = SMIPTR(pScrn);
    xf86CrtcPtr crtc0=NULL;
    xf86CrtcPtr crtc1=NULL;

    ENTER();

    if(pSmi->Chipset == SMI_COUGAR3DR){
	/* XXX - Looking at the datasheet, it seems trivial to add
	   dualhead support for this chip... Little more than
	   splitting the WRITE_FPR/WRITE_VPR calls in separate
	   functions. Has someone access to this hardware? */

	SMI_CrtcFuncsInit_base(&SMILynx_Crtc0Funcs, &SMILynx_Crtc0Priv);
	SMILynx_Crtc0Funcs.mode_set = SMILynx_CrtcModeSet_vga;
	SMILynx_Crtc0Priv.adjust_frame = SMILynx_CrtcAdjustFrame;
	SMILynx_Crtc0Priv.video_init = SMI730_CrtcVideoInit;
	SMILynx_Crtc0Priv.load_lut = SMILynx_CrtcLoadLUT_crt;

	crtc0=xf86CrtcCreate(pScrn,&SMILynx_Crtc0Funcs);
	if(!crtc0)
	    RETURN(FALSE);
	crtc0->driver_private = &SMILynx_Crtc0Priv;
    }else{
	if(pSmi->Dualhead){
	    /* CRTC0 is LCD*/
	    SMI_CrtcFuncsInit_base(&SMILynx_Crtc0Funcs, &SMILynx_Crtc0Priv);
	    SMILynx_Crtc0Funcs.mode_set = SMILynx_CrtcModeSet_lcd;
	    SMILynx_Crtc0Priv.adjust_frame = SMILynx_CrtcAdjustFrame;
	    SMILynx_Crtc0Priv.video_init = SMILynx_CrtcVideoInit_lcd;
	    SMILynx_Crtc0Priv.load_lut = SMILynx_CrtcLoadLUT_lcd;

	    crtc0=xf86CrtcCreate(pScrn,&SMILynx_Crtc0Funcs);
	    if(!crtc0)
		RETURN(FALSE);
	    crtc0->driver_private = &SMILynx_Crtc0Priv;

	    /* CRTC1 is CRT */
	    SMI_CrtcFuncsInit_base(&SMILynx_Crtc1Funcs, &SMILynx_Crtc1Priv);
	    SMILynx_Crtc1Funcs.mode_set = SMILynx_CrtcModeSet_crt;
	    SMILynx_Crtc1Priv.adjust_frame = SMILynx_CrtcAdjustFrame;
	    SMILynx_Crtc1Priv.video_init = SMILynx_CrtcVideoInit_crt;
	    SMILynx_Crtc1Priv.load_lut = SMILynx_CrtcLoadLUT_crt;

	    crtc1=xf86CrtcCreate(pScrn,&SMILynx_Crtc1Funcs);
	    if(!crtc1)
		RETURN(FALSE);
	    crtc1->driver_private = &SMILynx_Crtc1Priv;

	}else{
	    /* CRTC0 is LCD, but in standard refresh mode
	       it is controlled through the primary VGA registers */
	    SMI_CrtcFuncsInit_base(&SMILynx_Crtc0Funcs, &SMILynx_Crtc0Priv);
	    SMILynx_Crtc0Funcs.mode_set = SMILynx_CrtcModeSet_vga;
	    SMILynx_Crtc0Priv.adjust_frame = SMILynx_CrtcAdjustFrame;
	    SMILynx_Crtc0Priv.video_init = SMILynx_CrtcVideoInit_crt;
	    SMILynx_Crtc0Priv.load_lut = SMILynx_CrtcLoadLUT_crt;

	    crtc0=xf86CrtcCreate(pScrn,&SMILynx_Crtc0Funcs);
	    if(!crtc0)
		RETURN(FALSE);
	    crtc0->driver_private = &SMILynx_Crtc0Priv;
	}
    }

    RETURN(TRUE);
}

