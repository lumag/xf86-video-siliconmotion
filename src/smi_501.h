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

Except as contained in this notice, the names of the XFree86 Project and
Silicon Motion shall not be used in advertising or otherwise to promote the
sale, use or other dealings in this Software without prior written
authorization from the XFree86 Project and Silicon Motion.
*/

#ifndef _SMI_501_H
#define _SMI_501_H

/*
 * Documentation:
 * ftp://ftp.siliconmotion.com.tw/databooks/SM501MSOCDatabook_VersionB_1.pdf
 *
 * ftp://ftp.siliconmotion.com.tw/databooks/SM502_MMCC_Databook_V1.00.pdf
 */

#include <stdint.h>

#define	bits(lo, hi)			hi + 1 - lo


#define DRAM_CTL			0x000010
#define CMD_STATUS			0x000024

/* contents of either power0_clock or power1_clock */
#define CURRENT_CLOCK			0x00003c

#define POWER0_CLOCK			0x000044
#define POWER1_CLOCK			0x00004c
/*	POWER MODE 0 CLOCK
 *	Read/Write MMIO_base + 0x000044
 *	Power-on Default 0x2A1A0A09
 *
 *	POWER MODE 1 CLOCK
 *	Read/Write MMIO_base + 0x00004C
 *	Power-on Default 0x2A1A0A09
 *
 *	0:3	M1XCLK Frequency Divider
 *		0000 / 1		1000 / 3
 *		0001 / 2		1001 / 6
 *		0010 / 4		1010 / 12
 *		0011 / 8		1011 / 24
 *		0100 / 16		1100 / 48
 *		0101 / 32		1101 / 96
 *		0110 / 64		1110 / 192
 *		0111 / 128		1111 / 384
 *	4:4	M2XCLK Frequency Input Select.
 *		0: 288 MHz.
 *		1: 336 MHz/288 MHz/240 MHz/192 MHz
 *		   (see bits 5:4 in the Miscellaneous Timing register
 *		    at offset 0x68 on page 2-42).
 * 	8:11	MCLK Frequency Divider.
 *		0000 / 1		1000 / 3
 *		0001 / 2		1001 / 6
 *		0010 / 4		1010 / 12
 *		0011 / 8		1011 / 24
 *		0100 / 16		1100 / 48
 *		0101 / 32		1101 / 96
 *		0110 / 64		1110 / 192
 *		0111 / 128		1111 / 384
 *	12:12	MCLK Frequency Input Select.
 *		0: 288 MHz.
 *		1: 336 MHz/288 MHz/240 MHz/192 MHz
 *		   (see bits 5:4 in the Miscellaneous Timing register
 *		    at offset 0x68 on page 2-42).
 *	16:19	V2XCLK DIVIDER
 *		0000 / 1		1000 / 3
 *		0001 / 2		1001 / 6
 *		0010 / 4		1010 / 12
 *		0011 / 8		1011 / 24
 *		0100 / 16		1100 / 48
 *		0101 / 32		1101 / 96
 *		0110 / 64		1110 / 192
 *		0111 / 128		1111 / 384
 *	20:20	V2XCLK SELECT (Crt clock)
 *		0: 288 MHz
 *		1: 336 MHz/288 MHz/240 MHz/192 MHz
 *		   (see bits 5:4 in the Miscellaneous Timing register
 *		    at offset 0x68 on page 2-42).
 *	21:21	Disable 2X V2XCLK.
 *		0: Normal.
 *		1: No need to feed 2X VCLK.
 *	24:28	P2XCLK DIVIDER
 *		00000 / 1	01000 / 3	10000 / 5
 *		00001 / 2	01001 / 6	10001 / 10
 *		00010 / 4	01010 / 12	10010 / 20
 *		00011 / 8	01011 / 24	10011 / 40
 *		00100 / 16	01100 / 48	10100 / 80
 *		00101 / 32	01101 / 96	10101 / 160
 *		00110 / 64	01110 / 192	10110 / 320
 *		00111 / 128	01111 / 384	10111 / 640
 *	29:29	P2XCLK SELECT (Panel clock)
 *		00: 288 MHz
 *		01: 336 MHz/288 MHz/240 MHz/192 MHz
 *		  (see bits 5:4 in the Miscellaneous Timing register
 *		   at offset 0x68 on page 2-42).
 *	30:30	PLL SELECT
 *		0: Use standard p2_xxx clock
 *		1: Use PLL_CONTROL (MMIO 0x074) for clock setting.
 *		   Available only for the panel.
 *	31:31	Disable 2X P2XCLK.
 *		0: Normal.
 *		1: 1X clock for P2CLK.
 */
typedef union _MSOCClockRec {
    struct {
	/* Clock source for the local SDRAM controller. */
	int32_t 	m1_shift	: bits( 0,  2);
	int32_t 	m1_divider	: bits( 3,  3);
	int32_t		m1_select	: bits( 4,  4);
	int32_t 	u0		: bits( 5,  7);
	/* Main clock source for all functional blocks,
	 * such as the 2D engine, GPIO, Video Engine, DMA Engine. */
	int32_t 	m_shift		: bits( 8, 10);
	int32_t 	m_divider	: bits(11, 11);
	int32_t 	m_select	: bits(12, 12);
	int32_t 	u1		: bits(13, 15);
	/* 2X clock source for the CRT interface timing.
	 * The actual rate at which the pixels are shifted
	 * out is V2XCLK divided by two. */
	int32_t 	v2_shift	: bits(16, 18);
	int32_t 	v2_divider	: bits(19, 19);
	int32_t 	v2_select	: bits(20, 20);
	int32_t 	v2_1xclck	: bits(21, 21);
	int32_t 	u2		: bits(22, 23);
	/* 2X clock source for the Panel interface timing.
	 * The actual rate at which the pixels are shifted
	 * out is P2XCLK divided by two. */
	int32_t 	p2_shift	: bits(24, 26);
	int32_t 	p2_divider	: bits(27, 28);
	int32_t 	p2_select	: bits(29, 29);
	/* If pll_select is set, an alternate clock selection, available
	 * only in the 502 (using PLL_CTL, MMIO 0x074), will be used,
	 * and p2_* values will be ignored. */
	int32_t		pll_select	: bits(30, 30);
	/* If p2_1xclck is set, it means use 1x clock, otherwise
	 * 2x clocks must be specified in p2_{shift,divider,select}. */
	int32_t 	p2_1xclck	: bits(31, 31);
    } f;
    int32_t		value;
} MSOCClockRec, *MSOCClockPtr;

typedef struct _MSOCRegRec {
#define SYSTEM_CTL			0x000000
    /*	SYSTEM CONTROL
     *	Read/Write MMIO_base + 0x000000
     *	Power-on Default 0b0000.0000.XX0X.X0XX.0000.0000.0000.0000
     *
     *	7:7	PCI Retry
     *		0: Enable
     *		1: Disable
     *	15:15	PCI Burst Read Enable.
     *		The BE bit must be enabled as well for this bit to take effect.
     *		(BE bit is bit 29, bit 15 is BrE)
     *		0: Disable.
     *		1: Enable.
     *	29:29	PCI Burst Enable.
     *		0: Disable.
     *		1: Enable.
     *	30:31	Vertical Sync	Horizontal Sync
     *	   00	Pulsing		Pulsing
     *	   01	Pulsing		Not pulsing
     *	   10	Not pulsing	Pulsing
     *	   11	Not pulsing	Not pulsing
     */
    union {
	struct {
	    int32_t	u0		: bits( 0,  6);
	    int32_t	retry		: bits( 7,  7);
	    int32_t	u1		: bits( 8, 14);
	    int32_t	burst_read	: bits(15, 15);
	    int32_t	u2		: bits(16, 28);
	    int32_t	burst		: bits(29, 29);
	    int32_t	dpmsh		: bits(30, 30);
	    int32_t	dpmsv		: bits(31, 31);
	} f;
	int32_t	value;
    } system_ctl;

#define MISC_CTL			0x000004
    /*	Miscellaneous Control
     *	Read/Write MMIO_base + 0x000004
     *	Power-on Default 0b0000.0000.0000.00X0.0001.0000.XXX0.0XXX
     *
     *	12:12	DAC Power Control.
     *		0: Enable.
     *		1: Disable.
     *	24:24	Crystal Frequency Select.
     *		0: 24MHz.
     *		1: 12MHz
     */
    union {
	struct {
	    int32_t	u0		: bits( 0, 11);
	    int32_t	dac		: bits(12, 12);
	    int32_t	u1		: bits(13, 23);
	    int32_t	frequency	: bits(24, 24);
	} f;
	int32_t	value;
    } misc_ctl;

#define POWER0_GATE			0x000040
#define POWER1_GATE			0x000048
    /*	POWER MODE 0 GATE
     *	Read/Write MMIO_base + 0x000040
     *	Power-on Default 0x00021807
     *
     *	POWER MODE 1 GATE
     *	Read/Write MMIO_base + 0x000048
     *	Power-on Default 0x00021807
     *
     *	3:3	2D Engine Clock Control.
     *		0: Disable.
     *		1: Enable.
     *	4:4	Color Space Conversion Clock Control.
     *		0: Disable.
     *		1: Enable.
     *	5:5	ZV-Port Clock Control.
     *		0: Disable.
     *		1: Enable.
     *	6:6	GPIO, PWM, and I2C Clock Control.
     *		0: Disable.
     *		1: Enable.
     */
    union {
	struct {
	    int32_t	u0		: bits(0, 2);
	    int32_t	engine		: bits(3, 3);
	    int32_t	csc		: bits(4, 4);
	    int32_t	zv		: bits(5, 5);
	    int32_t	gpio		: bits(6, 6);
	} f;
	int32_t	value;
    } gate;
    int32_t	current_gate;

    MSOCClockRec	clock;
    int32_t		current_clock;

#define SLEEP_GATE			0x000050
    /*	SLEEP MODE GATE
     *	Read/Write MMIO_base + 0x000050
     *	Power-on Default 0x00018000
     *
     *	13:14	PLL Recovery.
     *		00: 1ms (32 counts).
     *		01: 2ms (64 counts).
     *		10: 3ms (96 counts).
     *		11: 4ms (128 counts).
     *	19:22	PLL Recovery Clock Divider.
     *		0000 / 4096	0100 / 256	1000 / 16
     *		0001 / 2048	0101 / 128	1001 / 8
     *		0010 / 1024	0110 / 64	1010 / 4
     *		0011 / 512	0111 / 32	1011 / 2
     *	    Internally, the PLL recovery time counters are based on a 32 us
     *	    clock. So you have to program the D field (19:22) to make the
     *	    host clock come as close to 33 us as possible.
     */
    union {
	struct {
	    int32_t	u0		: bits( 0, 12);
	    int32_t	recovery	: bits(13, 14);
	    int32_t	u1		: bits(15, 18);
	    int32_t	divider		: bits(19, 22);
	} f;
	int32_t		value;
    } sleep_gate;

#define POWER_CTL			0x000054
    /*	POWER MODE CONTROL
     *	Read/Write MMIO_base + 0x000054
     *	Power-on Default 0x00000000
     *
     *	1:0	Power Mode Select.
     *		00: Power Mode 0.
     *		01: Power Mode 1.
     *		10: Sleep Mode.
     *	2:2	Current Sleep Status.
     *		0: Not in sleep mode.
     *		1: In sleep mode.
     *	    When the SM501 is transitioning back from sleep mode to a normal
     *	    power mode (Modes 0 or 1), the software needs to poll this bit
     *	    until it becomes "0" before writing any other commands to the chip.
     */
    union {
	struct {
	    int32_t	mode		: bits(0, 1);
	    int32_t	status		: bits(2, 2);
	} f;
	int32_t		value;
    } power_ctl;


#define DEVICE_ID			0x000060
    /*	DEVICE ID
     *	Read/Write MMIO_base + 0x000060
     *	Power-on Default 0x050100A0
     *
     *	0:7	Revision Identification: (0xC0 for the 502).
     *	16:31	DeviceId Device Identification: 0x0501.
     */
    union {
	struct {
	    int32_t	revision	: bits( 0,  7);
	    int32_t	u0		: bits( 8, 15);
	    int32_t	ident		: bits(16, 31);
	} f;
	int32_t		value;
    } device_id;

#define TIMING_CTL			0x000068
    /*	Miscellaneous Control
     *	Read/Write MMIO_base + 0x000068
     *	Power-on Default 0x00000000
     *
     *  4:5	PLL Input frequency
     *		00: the output of PLL2 = 48 MHz x 7 = 336 MHz, power on default
     *		01: the output of PLL2 = 48 MHz x 6 = 288 MHz
     *		10: the output of PLL2 = 48 MHz x 5 = 240 MHz
     *		11: the output of PLL2 = 48 MHz x 4 = 192 MHz
     */
    union {
	struct {
	    int32_t	u0		: bits( 0,  3);
	    int32_t	pll		: bits( 4,  5);
	} f;
	int32_t	value;
    } timing_ctl;

#define PLL_CTL				0x000074
    /*	Programmable PLL Control
     *	Read/Write MMIO_base + 0x000074
     *	Power-on Default 0x000000FF
     *	0:7	PLL M Value
     *	8:14	PLL N Value
     *	15:15	PLL Output Divided by 2.
     *		0: Disable.
     *		1: Enable.
     *	16:16	PLL Clock Select.
     *		0: Crystal input.
     *		1: Test clock input.
     *	17:17	PLL Power Down.
     *		0: Power down.
     *		1: Power on.
     *
     *	The formula is:
     *		Requested Pixel Clock = Input Frequency * M / N
     *	Input Frequency is the input crystal frequency value (24 MHz in
     *	the SMI VGX Demo Board). N must be a value between 2 and 24.
     *	M can be any (8 bits) value, and a loop testing all possible N
     *	values should be the best approach to calculate it's value.
     */
    union {
	struct {
	    int32_t	m		: bits( 0,  7);
	    int32_t	n		: bits( 8, 14);
	    int32_t	divider		: bits(15, 15);
	    int32_t	select		: bits(16, 16);
	    int32_t	power		: bits(17, 17);
	} f;
	int32_t	value;
    } pll_ctl;

#define PANEL_DISPLAY_CTL		0x080000
    /*	PANEL DISPLAY CONTROL
     *	Read MMIO_base + 0x080000
     *	Power-on Default 0x00010000
     *
     *	1:0	Format Panel Graphics Plane Format.
     *		00: 8-bit indexed mode.
     *		01: 16-bit RGB 5:6:5 mode.
     *		10: 32-bit RGB 8:8:8 mode.
     *	2:2	Panel Graphics Plane Enable.
     *		0: Disable panel graphics plane.
     *		1: Enable panel graphics plane.
     *	3:3	Enable Gamma Control. Gamma control can only
     *		be enabled in RGB 5:6:5 and RGB 8:8:8 modes.
     *		0: Disable.
     *		1: Enable.
     *	8:8	Enable Panel Timing.
     *		0: Disable panel timing.
     *		1: Enable panel timing.
     *	12:12	Horizontal Sync Pulse Phase Select.
     *		0: Horizontal sync pulse active high.
     *		1: Horizontal sync pulse active low.
     *	13:13	Vertical Sync Pulse Phase Select.
     *		0: Vertical sync pulse active high.
     *		1: Vertical sync pulse active low.
     *	24:24	Control FPVDDEN Output Pin.
     *		0: Driven low.
     *		1: Driven high.
     *	25:25	Panel Control Signals and Data Lines Enable.
     *		0: Disable panel control signals and data lines.
     *		1: Enable panel control signals and data lines.
     *	26:26	Control VBIASEN Output Pin.
     *		0: Driven low.
     *		1: Driven high.
     *	27:27	Control FPEN Output Pin.
     *		0: Driven low.
     *		1: Driven high.
     */
    union {
	struct {
	    int32_t	format		: bits( 0,  1);
	    int32_t	enable		: bits( 2,  2);
	    int32_t	gamma		: bits( 3,  3);
	    int32_t	u0		: bits( 4,  7);
	    int32_t	timing		: bits( 8,  8);
	    int32_t	u1		: bits( 9, 11);
	    int32_t	hsync		: bits(12, 12);
	    int32_t	vsync		: bits(13, 13);
	    int32_t	u2		: bits(14, 23);
	    int32_t	vdd		: bits(24, 24);
	    int32_t	signal		: bits(25, 25);
	    int32_t	bias		: bits(26, 26);
	    int32_t	fp		: bits(27, 27);
	} f;
	int32_t		value;
    } panel_display_ctl;

#define PANEL_FB_ADDRESS		0x08000c
    /*	PANEL FB ADDRESS
     *	Read/Write MMIO_base + 0x08000C
     *	Power-on Default Undefined
     *
     *	4:25	Address Memory address of frame buffer for the
     *		panel graphics plane with 128-bit alignment.
     *	26:26	Chip Select for External Memory.
     *		0: CS0 of external memory.
     *		1: CS1 of external memory.
     *	27:27	Ext Memory Selection.
     *		0: Local memory.
     *		1: External memory.
     *	31:31	Status Bit.
     *		0: No flip pending.
     *		1: Flip pending.
     */
    union {
	struct {
	    int32_t	u0		: bits( 0,  3);
	    int32_t	address		: bits( 4, 25);
	    int32_t	mextern		: bits(26, 26);
	    int32_t	mselect		: bits(27, 27);
	    int32_t	u1		: bits(28, 30);
	    int32_t	pending		: bits(31, 31);
	} f;
	int32_t		value;
    } panel_fb_address;

#define PANEL_FB_WIDTH			0x080010
    /*	PANEL FB WIDTH
     *	Read/Write MMIO_base + 0x080010
     *	Power-on Default Undefined
     *
     *	4:13	Number of 128-bit aligned bytes per line of the FB
     *		graphics plane
     *	20:29	Number of bytes per line of the panel graphics window
     *		specified in 128-bit aligned bytes.
     */
    union {
	struct {
	    int32_t	u0		: bits( 0,  3);
	    int32_t	offset		: bits( 4, 13);
	    int32_t	u1		: bits(14, 19);
	    int32_t	width		: bits(20, 29);
	} f;
	int32_t		value;
    } panel_fb_width;

#define PANEL_WWIDTH			0x080014
    /*	PANEL WINDOW WIDTH
     *	Read/Write MMIO_base + 0x080014
     *	Power-on Default Undefined
     *
     *	0:11	Starting x-coordinate of panel graphics window
     *		specified in pixels.
     *	16:27	Width of FB graphics window specified in pixels.
     */
    union {
	struct {
	    int32_t	x		: bits( 0, 11);
	    int32_t	u0		: bits(12, 15);
	    int32_t	width		: bits(16, 27);
	} f;
	int32_t		value;
    } panel_wwidth;

#define PANEL_WHEIGHT			0x080018
    /*	PANEL WINDOW HEIGHT
     *	Read/Write MMIO_base + 0x080018
     *	Power-on Default Undefined
     *
     *	0:11	Starting y-coordinate of panel graphics window
     *		specified in pixels.
     *	16:27	Height of FB graphics window specified in pixels.
     */
    union {
	struct {
	    int32_t	y		: bits( 0, 11);
	    int32_t	u0		: bits(12, 15);
	    int32_t	height		: bits(16, 27);
	} f;
	int32_t		value;
    } panel_wheight;

#define PANEL_PLANE_TL			0x08001c
    /*	PANEL PLANE TL
     *	Read/Write MMIO_base + 0x08001c
     *	Power-on Default Undefined
     *
     *	0:10	Left location of the panel graphics plane specified in pixels.
     *	16:26	Top location of the panel graphics plane specified in lines.
     */
    union {
	struct {
	    int32_t	left		: bits( 0, 10);
	    int32_t	u0		: bits(11, 15);
	    int32_t	top		: bits(16, 26);
	} f;
	int32_t		value;
    } panel_plane_tl;

#define PANEL_PLANE_BR			0x080020
    /*	PANEL PLANE BR
     *	Read/Write MMIO_base + 0x080020
     *	Power-on Default Undefined
     *
     *	0:10	Right location of the panel graphics plane specified in pixels.
     *	16:26	Bottom location of the panel graphics plane specified in lines.
     */
    union {
	struct {
	    int32_t	right		: bits( 0, 10);
	    int32_t	u0		: bits(11, 15);
	    int32_t	bottom		: bits(16, 26);
	} f;
	int32_t		value;
    } panel_plane_br;

#define PANEL_HTOTAL			0x080024
    /*	PANEL HORIZONTAL TOTAL
     *	Read/Write MMIO_base + 0x080024
     *	Power-on Default Undefined
     *
     *	0:11	Panel horizontal display end specified as number of pixels - 1.
     *	16:27	Panel horizontal total specified as number of pixels - 1.
     */
    union {
	struct {
	    int32_t	end		: bits( 0, 11);
	    int32_t	u0		: bits(12, 15);
	    int32_t	total		: bits(16, 27);
	} f;
	int32_t		value;
    } panel_htotal;

#define PANEL_HSYNC			0x080028
    /*	PANEL HORIZONTAL SYNC
     *	Read/Write MMIO_base + 0x080028
     *	Power-on Default Undefined
     *
     *	0:11 HS Panel horizontal sync start specified as pixel number - 1.
     *	16:23 HSW Panel horizontal sync width specified in pixels.
     */
    union {
	struct {
	    int32_t	start		: bits( 0, 11);
	    int32_t	u0		: bits(12, 15);
	    int32_t	width		: bits(16, 23);
	} f;
	int32_t		value;
    } panel_hsync;

#define PANEL_VTOTAL			0x08002c
    /*	PANEL VERTICAL TOTAL
     *	Read/Write MMIO_base + 0x08002C
     *	Power-on Default Undefined
     *
     *	0:11 VDE Panel vertical display end specified as number of pixels - 1.
     *	16:27 VT Panel vertical total specified as number of pixels - 1.
     */
    union {
	struct {
	    int32_t	end		: bits( 0, 11);
	    int32_t	u0		: bits(12, 15);
	    int32_t	total		: bits(16, 27);
	} f;
	int32_t		value;
    } panel_vtotal;

#define PANEL_VSYNC			0x080030
    /*	PANEL VERTICAL SYNC
     *	Read/Write MMIO_base + 0x080030
     *	Power-on Default Undefined
     *
     *	0:11 VS Panel vertical sync start specified as pixel number - 1.
     *	16:23 VSH Panel vertical sync height specified in pixels.
     */
    union {
	struct {
	    int32_t	start		: bits( 0, 11);
	    int32_t	u0		: bits(12, 15);
	    int32_t	height		: bits(16, 23);
	} f;
	int32_t		value;
    } panel_vsync;

#define ALPHA_DISPLAY_CTL		0x080100
    /*	ALPHA DISPLAY CONTROL
     *	Read MMIO base + 0x080100
     *	Power-on Default 0x00010000
     *
     *	0:1	Alpha Plane Format
     *		01: 16-bit RGB 5:6:5 mode.
     *		10: 8-bit indexed a|4:4 mode.
     *		11: 16-bit aRGB 4:4:4:4 mode.
     *	2:2	Alpha Plane Enable
     *		0: Disable Alpha Plane
     *		1: Enable Alpha Plane
     *	3:3	Enable Chroma Key.
     *		0: Disable chroma key.
     *		1: Enable chroma key.
     *	24:27	Alpha Plane Alpha Value.
     *		This field is only valid when bit 28 is set.
     *	28:28	Alpha Select.
     *		0: 0: Use per-pixel alpha values.
     *		1: Use alpha value specified in Alpha.
     */
    union {
	struct {
	    int32_t	format		: bits( 0,  1);
	    int32_t	enable		: bits( 2,  2);
	    int32_t	chromakey	: bits( 3,  3);
	    int32_t	u0		: bits( 4, 23);
	    int32_t	alpha		: bits(24, 27);
	    int32_t	select		: bits(28, 28);
	} f;
	int32_t		value;
    } alpha_display_ctl;

#define ALPHA_FB_ADDRESS		0x080104
    /*	ALPHA FB ADDRESS
     *	Read/Write MMIO_base + 0x080104
     *	Power-on Default Undefined
     *
     *	4:25	Address Memory address of frame buffer for the
     *		CRT graphics plane with 128-bit alignment.
     *	26:26	Chip Select for External Memory.
     *		0: CS0 of external memory.
     *		1: CS1 of external memory.
     *	27:27	Ext Memory Selection.
     *		0: Local memory.
     *		1: External memory.
     *	31:31	Status Bit.
     *		0: No flip pending.
     *		1: Flip pending.
     */
    union {
	struct {
	    int32_t	u0		: bits( 0,  3);
	    int32_t	address		: bits( 4, 25);
	    int32_t	mextern		: bits(26, 26);
	    int32_t	mselect		: bits(27, 27);
	    int32_t	u1		: bits(28, 30);
	    int32_t	pending		: bits(31, 31);
	} f;
	int32_t		value;
    } alpha_fb_address;

#define ALPHA_FB_WIDTH			0x080108
    /*	ALPHA FB WIDTH
     *	Read/Write MMIO_base + 0x080108
     *	Power-on Default Undefined
     *
     *	4:13	Number of 128-bit aligned bytes per line of the FB
     *		graphics plane
     *	20:29	Number of bytes per line of the alpha window
     *		specified in 128-bit aligned bytes.
     */
    union {
	struct {
	    int32_t	u0		: bits( 0,  3);
	    int32_t	offset		: bits( 4, 13);
	    int32_t	u1		: bits(14, 19);
	    int32_t	width		: bits(20, 29);
	} f;
	int32_t		value;
    } alpha_fb_width;

#define ALPHA_PLANE_TL			0x08010c
    /*	ALPHA PLANE TL
     *	Read/Write MMIO_base + 0x08010c
     *	Power-on Default Undefined
     *
     *	0:10	Left location of the panel graphics plane specified in pixels.
     *	16:26	Top location of the panel graphics plane specified in lines.
     */
    union {
	struct {
	    int32_t	left		: bits( 0, 10);
	    int32_t	u0		: bits(11, 15);
	    int32_t	top		: bits(16, 26);
	} f;
	int32_t		value;
    } alpha_plane_tl;

#define ALPHA_PLANE_BR			0x080110
    /*	ALPHA PLANE BR
     *	Read/Write MMIO_base + 0x080110
     *	Power-on Default Undefined
     *
     *	0:10	Right location of the panel graphics plane specified in pixels.
     *	16:26	Bottom location of the panel graphics plane specified in lines.
     */
    union {
	struct {
	    int32_t	right		: bits( 0, 10);
	    int32_t	u0		: bits(11, 15);
	    int32_t	bottom		: bits(16, 26);
	} f;
	int32_t		value;
    } alpha_plane_br;

#define ALPHA_CHROMA_KEY		0x080114
    /*	ALPHA PLANE BR
     *	Read/Write MMIO_base + 0x080114
     *	Power-on Default Undefined
     *
     *	0:15	Chroma Key Value for Alpha Plane.
     *	16:31	Chroma Key Mask for Alpha Plane.
     *		0: Compare respective bit.
     *		1: Do not compare respective bit.
     *		Note that the 0 and 1 values are mean't for every one of
     *		the 16 5:6:5 bits
     */
    union {
	struct {
	    int32_t	value		: bits( 0, 15);
	    int32_t	mask		: bits(16, 31);
	} f;
	int32_t		value;
    } alpha_chroma_key;

    /* 0x080118 to 0x80134 16 4 bit indexed rgb 5:6:5 table */
#define ALPHA_COLOR_LOOKUP		0x080118

#define CRT_DISPLAY_CTL			0x080200
    /*	CRT DISPLAY CONTROL
     *	Read MMIO_base + 0x080200
     *	Power-on Default 0x00010000
     *
     *	0:1	Format Panel Graphics Plane Format.
     *		00: 8-bit indexed mode.
     *		01: 16-bit RGB 5:6:5 mode.
     *		10: 32-bit RGB 8:8:8 mode.
     *	2:2	CRT Graphics Plane Enable.
     *		0: Disable CRT Graphics plane.
     *		1: Enable CRT Graphics plane.
     *	3:3	Enable Gamma Control. Gamma control can be enabled
     *		only in RGB 5:6:5 and RGB 8:8:8 modes.
     *		0: Disable gamma control.
     *		1: Enable gamma control.
     *	4:7	Starting Pixel Number for Smooth Pixel Panning.
     *	8:8	Enable CRT Timing.
     *		0: Disable CRT timing.
     *		1: Enable CRT timing.
     *	9:9:	CRT Data Select.
     *		0: CRT will display panel data.
     *		1: CRT will display CRT data.
     *	10:10	CRT Data Blanking.
     *		0: CRT will show pixels.
     *		1: CRT will be blank.
     *	11:11	Vertical Sync. This bit is read only.
     *	12:12	Horizontal Sync Pulse Phase Select.
     *		0: Horizontal sync pulse active high.
     *		1: Horizontal sync pulse active low.
     *	13:13	Vertical Sync Pulse Phase Select.
     *		0: Vertical sync pulse active high.
     *		1: Vertical sync pulse active low.
     */
    union {
	struct {
	    int32_t	format		: bits( 0,  1);
	    int32_t	enable		: bits( 2,  2);
	    int32_t	gamma		: bits( 3,  3);
	    int32_t	pixel		: bits( 4,  7);
	    int32_t	timing		: bits( 8,  8);
	    int32_t	select		: bits( 9,  9);
	    int32_t	blank		: bits(10, 10);
	    int32_t	sync		: bits(11, 11);
	    int32_t	hsync		: bits(12, 12);
	    int32_t	vsync		: bits(13, 13);
	} f;
	int32_t		value;
    } crt_display_ctl;

#define CRT_FB_ADDRESS			0x080204
    /*	CRT FB ADDRESS
     *	Read/Write MMIO_base + 0x080204
     *	Power-on Default Undefined
     *
     *	4:25	Address Memory address of frame buffer for the
     *		CRT graphics plane with 128-bit alignment.
     *	26:26	Chip Select for External Memory.
     *		0: CS0 of external memory.
     *		1: CS1 of external memory.
     *	27:27	Ext Memory Selection.
     *		0: Local memory.
     *		1: External memory.
     *	31:31	Status Bit.
     *		0: No flip pending.
     *		1: Flip pending.
     */
    union {
	struct {
	    int32_t	u0		: bits( 0,  3);
	    int32_t	address		: bits( 4, 25);
	    int32_t	mextern		: bits(26, 26);
	    int32_t	mselect		: bits(27, 27);
	    int32_t	u1		: bits(28, 30);
	    int32_t	pending		: bits(31, 31);
	} f;
	int32_t		value;
    } crt_fb_address;

#define CRT_FB_WIDTH			0x080208
    /*	CRT FB WIDTH
     *	Read/Write MMIO_base + 0x080208
     *	Power-on Default Undefined
     *
     *	4:13	Number of 128-bit aligned bytes per line of the FB
     *		graphics plane
     *	20:29	Number of bytes per line of the crt graphics window
     *		specified in 128-bit aligned bytes.
     */
    union {
	struct {
	    int32_t	u0		: bits( 0,  3);
	    int32_t	offset		: bits( 4, 13);
	    int32_t	u1		: bits(14, 19);
	    int32_t	width		: bits(20, 29);
	} f;
	int32_t		value;
    } crt_fb_width;

#define CRT_HTOTAL			0x08020c
    /*	CRT HORIZONTAL TOTAL
     *	Read/Write MMIO_base + 0x08020C
     *	Power-on Default Undefined
     *
     *	0:11	Crt horizontal display end specified as number of pixels - 1.
     *	16:27	Crt horizontal total specified as number of pixels - 1.
     */
    union {
	struct {
	    int32_t	end		: bits( 0, 11);
	    int32_t	u0		: bits(12, 15);
	    int32_t	total		: bits(16, 27);
	} f;
	int32_t		value;
    } crt_htotal;

#define CRT_HSYNC			0x080210
    /*	CRT HORIZONTAL SYNC
     *	Read/Write MMIO_base + 0x080210
     *	Power-on Default Undefined
     *
     *	0:11	Crt horizontal sync start specified as pixel number - 1.
     *	16:23	Crt horizontal sync width specified in pixels.
     */
    union {
	struct {
	    int32_t	start		: bits( 0, 11);
	    int32_t	u0		: bits(12, 15);
	    int32_t	width		: bits(16, 23);
	} f;
	int32_t		value;
    } crt_hsync;

#define CRT_VTOTAL			0x080214
    /*	CRT VERTICAL TOTAL
     *	Read/Write MMIO_base + 0x080214
     *	Power-on Default Undefined
     *
     *	0:10	Crt vertical display end specified as number of pixels - 1.
     *	16:26	Crt vertical total specified as number of pixels - 1.
     */
    union {
	struct {
	    int32_t	end		: bits( 0, 10);
	    int32_t	u0		: bits(11, 15);
	    int32_t	total		: bits(16, 26);
	} f;
	int32_t		value;
    } crt_vtotal;

#define CRT_VSYNC			0x080218
    /*	CRT VERTICAL SYNC
     *	Read/Write MMIO_base + 0x080218
     *	Power-on Default Undefined
     *
     *	0:11	Crt vertical sync start specified as pixel number - 1.
     *	16:21	Crt vertical sync height specified in pixels.
     */
    union {
	struct {
	    int32_t	start		: bits( 0, 11);
	    int32_t	u0		: bits(12, 15);
	    int32_t	height		: bits(16, 21);
	} f;
	int32_t		value;
    } crt_vsync;

#define CRT_DETECT			0x080224
    /*	CRT MONITOR DETECT
     *	Read/Write MMIO_base + 0x080224
     *	Power-on Default Undefined
     *
     *	0:23	Monitor Detect Data in RGB 8:8:8. This field is read-only.
     *	24:24	Monitor Detect Enable.
     *		0: Disable.
     *		1: Enable.
     *	25:25	Monitor Detect Read Back.
     *		1: All R, G, and B voltages are greater than 0.325 V.
     *		0: All R, G, and B voltages are less than or equal to 0.325 V.
     */
    union {
	struct {
	    int32_t	data		: bits( 0, 23);
	    int32_t	enable		: bits(24, 24);
	    int32_t	voltage		: bits(25, 25);
	} f;
	int32_t		value;
    } crt_detect;

#define PANEL_PALETTE			0x080400
#define CRT_PALETTE			0x080c00

#define ACCEL_SRC			0x100000
    int32_t	accel_src;

#define ACCEL_DST			0x100004
    int32_t	accel_dst;

#define ACCEL_DIM			0x100008
    int32_t	accel_dim;

#define ACCEL_CTL			0x10000c
    int32_t	accel_ctl;

#define ACCEL_PITCH			0x100010
    int32_t	accel_pitch;

#define ACCEL_FMT			0x10001c
    int32_t	accel_fmt;

#define ACCEL_CLIP_TL			0x10002c
    int32_t	accel_clip_tl;

#define ACCEL_CLIP_BR			0x100030
    int32_t	accel_clip_br;

#define ACCEL_PAT_LO			0x100034
    int32_t	accel_pat_lo;

#define ACCEL_PAT_HI			0x100038
    int32_t	accel_pat_hi;

#define ACCEL_WWIDTH			0x10003c
    int32_t	accel_wwidth;

#define ACCEL_SRC_BASE			0x100040
    int32_t	accel_src_base;

#define ACCEL_DST_BASE			0x100044
    int32_t	accel_dst_base;

} MSOCRegRec, *MSOCRegPtr;


/* In Kb - documentation says it is 64Kb... */
#define FB_RESERVE4USB			512


void SMI501_Save(ScrnInfoPtr pScrn);
void SMI501_DisplayPowerManagementSet(ScrnInfoPtr pScrn,
				      int PowerManagementMode, int flags);
void SMI501_PrintRegs(ScrnInfoPtr pScrn);
double SMI501_FindClock(double clock, int max_divider, Bool has1xclck,
			       int32_t *x2_1xclck, int32_t *x2_select,
			       int32_t *x2_divider, int32_t *x2_shift);
double SMI501_FindPLLClock(double clock, int32_t *m, int32_t *n,
				  int32_t *xclck);
void SMI501_WaitVSync(SMIPtr pSmi, int vsync_count);

/* Initialize the CRTC-independent hardware registers */
Bool SMI501_HWInit(ScrnInfoPtr pScrn);
/* Load to hardware the specified register set */
void SMI501_WriteMode_common(ScrnInfoPtr pScrn, MSOCRegPtr mode);
void SMI501_WriteMode_lcd(ScrnInfoPtr pScrn, MSOCRegPtr mode);
void SMI501_WriteMode_crt(ScrnInfoPtr pScrn, MSOCRegPtr mode);
void SMI501_WriteMode_alpha(ScrnInfoPtr pScrn, MSOCRegPtr mode);
void SMI501_WriteMode(ScrnInfoPtr pScrn, MSOCRegPtr restore);
void SMI501_PowerPanel(ScrnInfoPtr pScrn, MSOCRegPtr mode, Bool on);

/* smi501_crtc.c */
Bool SMI501_CrtcPreInit(ScrnInfoPtr pScrn);

/* smi501_output.c */
Bool SMI501_OutputPreInit(ScrnInfoPtr pScrn);

#endif  /*_SMI_501_H*/
