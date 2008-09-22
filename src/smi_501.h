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
 */

#include <stdint.h>

#define field(record, name)		record.detail.name
#define	bitfield(lo, hi)		hi + 1 - lo


#define DRAM_CONTROL			0x000010
#define CMD_STATUS			0x000024

/* contents of either power0_clock or power1_clock */
#define CURRENT_CLOCK			0x00003c

#define POWER0_CLOCK			0x000044
#define POWER1_CLOCK			0x00004c
/*  POWER MODE 0 CLOCK
 *  Read/Write MMIO_base + 0x000044
 *  Power-on Default 0x2A1A0A09
 *
 *  POWER MODE 1 CLOCK
 *  Read/Write MMIO_base + 0x00004C
 *  Power-on Default 0x2A1A0A09
 *
 *  0:3     M2XCLK Frequency Divider
 *	    0000 / 1	    1000 / 3
 *	    0001 / 2	    1001 / 6
 *	    0010 / 4	    1010 / 12
 *	    0011 / 8	    1011 / 24
 *	    0100 / 16	    1100 / 48
 *	    0101 / 32	    1101 / 96
 *	    0110 / 64	    1110 / 192
 *	    0111 / 128	    1111 / 384
 *  4:4     M2XCLK Frequency Input Select.
 *	    0: 288 MHz.
 *	    1: 336 MHz/288 MHz/240 MHz/192 MHz
 *	       (see bits 5:4 in the Miscellaneous Timing register
 *		at offset 0x68 on page 2-42).
 *  8:11    MCLK Frequency Divider.
 *	    0000 / 1	    1000 / 3
 *	    0001 / 2	    1001 / 6
 *	    0010 / 4	    1010 / 12
 *	    0011 / 8	    1011 / 24
 *	    0100 / 16	    1100 / 48
 *	    0101 / 32	    1101 / 96
 *	    0110 / 64	    1110 / 192
 *	    0111 / 128	    1111 / 384
 *  12:12   MCLK Frequency Input Select.
 *	    0: 288 MHz.
 *	    1: 336 MHz/288 MHz/240 MHz/192 MHz
 *	       (see bits 5:4 in the Miscellaneous Timing register
 *		at offset 0x68 on page 2-42).
 * 16:19    V2XCLK DIVIDER
 *	    0000 / 1	    1000 / 3
 *	    0001 / 2	    1001 / 6
 *	    0010 / 4	    1010 / 12
 *	    0011 / 8	    1011 / 24
 *	    0100 / 16	    1100 / 48
 *	    0101 / 32	    1101 / 96
 *	    0110 / 64	    1110 / 192
 *	    0111 / 128	    1111 / 384
 * 20:20    V2XCLK SELECT (Crt clock)
 *	    0: 288 MHz
 *	    1: 336 MHz/288 MHz/240 MHz/192 MHz
 *	       (see bits 5:4 in the Miscellaneous Timing register
 *		at offset 0x68 on page 2-42).
 * 24:28    P2XCLK DIVIDER
 *	    00000 / 1	    01000 / 3	    10000 / 5
 *	    00001 / 2	    01001 / 6	    10001 / 10
 *	    00010 / 4	    01010 / 12	    10010 / 20
 *	    00011 / 8	    01011 / 24	    10011 / 40
 *	    00100 / 16	    01100 / 48	    10100 / 80
 *	    00101 / 32	    01101 / 96	    10101 / 160
 *	    00110 / 64	    01110 / 192     10110 / 320
 *	    00111 / 128     01111 / 384     10111 / 640
 *  29:29   P2XCLK SELECT (Panel clock)
 *	    0: 288 MHz
 *	    1: 336 MHz/288 MHz/240 MHz/192 MHz
 *	      (see bits 5:4 in the Miscellaneous Timing register
 *	       at offset 0x68 on page 2-42).
 *
 * Remarks:
 *  Table 2-2: Programmable Clock Branches
 *  Clock Description
 *  P2XCLK  2X clock source for the Panel interface timing.
 *	    The actual rate at which the pixels are shifted
 *	    out is P2XCLK divided by two.
 *  V2XCLK  2X clock source for the CRT interface timing.
 *	    The actual rate at which the pixels are shifted
 *	    out is V2XCLK divided by two
 */
typedef union _MSOCClockRec {
    struct {
	int32_t 	m2_shift	: bitfield( 0,  2);
	int32_t 	m2_divider	: bitfield( 3,  3);
	int32_t		m2_select	: bitfield( 4,  4);
	int32_t 	u0		: bitfield( 5,  7);
	int32_t 	m_shift		: bitfield( 8, 10);
	int32_t 	m_divider	: bitfield(11, 11);
	int32_t 	m_select	: bitfield(12, 12);
	int32_t 	u1		: bitfield(13, 15);
	int32_t 	v2_shift	: bitfield(16, 18);
	int32_t 	v2_divider	: bitfield(19, 19);
	int32_t 	v2_select	: bitfield(20, 20);
	int32_t 	u2		: bitfield(21, 23);
	int32_t 	p2_shift	: bitfield(24, 26);
	int32_t 	p2_divider	: bitfield(27, 28);
	int32_t 	p2_select	: bitfield(29, 29);
    } detail;
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
	    int32_t	u0		: bitfield( 0,  6);
	    int32_t	retry		: bitfield( 7,  7);
	    int32_t	u1		: bitfield( 8, 14);
	    int32_t	burst_read	: bitfield(15, 15);
	    int32_t	u2		: bitfield(16, 28);
	    int32_t	burst		: bitfield(29, 29);
	    int32_t	dpmsh		: bitfield(30, 30);
	    int32_t	dpmsv		: bitfield(31, 31);
	} detail;
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
     */
    union {
	struct {
	    int32_t	u0		: bitfield( 0, 11);
	    int32_t	dac		: bitfield(12, 12);
	} detail;
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
	    int32_t	u0		: bitfield(0, 2);
	    int32_t	engine		: bitfield(3, 3);
	    int32_t	csc		: bitfield(4, 4);
	    int32_t	zv		: bitfield(5, 5);
	    int32_t	gpio		: bitfield(6, 6);
	} detail;
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
	    int32_t	u0		: bitfield( 0, 12);
	    int32_t	recovery	: bitfield(13, 14);
	    int32_t	u1		: bitfield(15, 18);
	    int32_t	divider		: bitfield(19, 22);
	} detail;
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
	    int32_t	mode		: bitfield(0, 1);
	    int32_t	status		: bitfield(2, 2);
	} detail;
	int32_t		value;
    } power_ctl;


#define TIMING_CONTROL			0x000068
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
	    int32_t	u0		: bitfield( 0,  3);
	    int32_t	pll		: bitfield( 4,  5);
	} detail;
	int32_t	value;
    } timing_control;

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
	    int32_t	format		: bitfield( 0,  1);
	    int32_t	enable		: bitfield( 2,  2);
	    int32_t	u0		: bitfield( 3,  7);
	    int32_t	timing		: bitfield( 8,  8);
	    int32_t	u1		: bitfield( 9, 11);
	    int32_t	hsync		: bitfield(12, 12);
	    int32_t	vsync		: bitfield(13, 13);
	    int32_t	u2		: bitfield(14, 23);
	    int32_t	vdd		: bitfield(24, 24);
	    int32_t	signal		: bitfield(25, 25);
	    int32_t	bias		: bitfield(26, 26);
	    int32_t	fp		: bitfield(27, 27);
	} detail;
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
	    int32_t	u0		: bitfield( 0,  3);
	    int32_t	address		: bitfield( 4, 25);
	    int32_t	mextern		: bitfield(26, 26);
	    int32_t	mselect		: bitfield(27, 27);
	    int32_t	u1		: bitfield(28, 30);
	    int32_t	pending		: bitfield(31, 31);
	} detail;
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
	    int32_t	u0		: bitfield( 0,  3);
	    int32_t	offset		: bitfield( 4, 13);
	    int32_t	u1		: bitfield(14, 19);
	    int32_t	width		: bitfield(20, 29);
	} detail;
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
	    int32_t	x		: bitfield( 0, 11);
	    int32_t	u0		: bitfield(12, 15);
	    int32_t	width		: bitfield(16, 27);
	} detail;
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
	    int32_t	y		: bitfield( 0, 11);
	    int32_t	u0		: bitfield(12, 15);
	    int32_t	height		: bitfield(16, 27);
	} detail;
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
	    int32_t	left		: bitfield( 0, 10);
	    int32_t	u0		: bitfield(11, 15);
	    int32_t	top		: bitfield(16, 26);
	} detail;
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
	    int32_t	right		: bitfield( 0, 10);
	    int32_t	u0		: bitfield(11, 15);
	    int32_t	bottom		: bitfield(16, 26);
	} detail;
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
	    int32_t	end		: bitfield( 0, 11);
	    int32_t	u0		: bitfield(12, 15);
	    int32_t	total		: bitfield(16, 27);
	} detail;
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
	    int32_t	start		: bitfield( 0, 11);
	    int32_t	u0		: bitfield(12, 15);
	    int32_t	width		: bitfield(16, 23);
	} detail;
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
	    int32_t	end		: bitfield( 0, 11);
	    int32_t	u0		: bitfield(12, 15);
	    int32_t	total		: bitfield(16, 27);
	} detail;
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
	    int32_t	start		: bitfield( 0, 11);
	    int32_t	u0		: bitfield(12, 15);
	    int32_t	height		: bitfield(16, 23);
	} detail;
	int32_t		value;
    } panel_vsync;

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
     *	9:9:	CRT Data Select.
     *		0: CRT will display panel data.
     *		1: CRT will display CRT data.
     *	12:12	Horizontal Sync Pulse Phase Select.
     *		0: Horizontal sync pulse active high.
     *		1: Horizontal sync pulse active low.
     *	13:13	Vertical Sync Pulse Phase Select.
     *		0: Vertical sync pulse active high.
     *		1: Vertical sync pulse active low.
     */
    union {
	struct {
	    int32_t	format		: bitfield( 0,  1);
	    int32_t	enable		: bitfield( 2,  2);
	    int32_t	u0		: bitfield( 3,  8);
	    int32_t	select		: bitfield( 9,  9);
	    int32_t	u1		: bitfield(10, 11);
	    int32_t	hsync		: bitfield(12, 12);
	    int32_t	vsync		: bitfield(13, 13);
	} detail;
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
	    int32_t	u0		: bitfield( 0,  3);
	    int32_t	address		: bitfield( 4, 25);
	    int32_t	mextern		: bitfield(26, 26);
	    int32_t	mselect		: bitfield(27, 27);
	    int32_t	u1		: bitfield(28, 30);
	    int32_t	pending		: bitfield(31, 31);
	} detail;
	int32_t		value;
    } crt_fb_address;

#define CRT_FB_WIDTH			0x080208
    /*	CRT FB WIDTH
     *	Read/Write MMIO_base + 0x080208
     *	Power-on Default Undefined
     *
     *	4:13	Number of 128-bit aligned bytes per line of the FB
     *		graphics plane
     *	20:29	Number of bytes per line of the panel graphics window
     *		specified in 128-bit aligned bytes.
     */
    union {
	struct {
	    int32_t	u0		: bitfield( 0,  3);
	    int32_t	offset		: bitfield( 4, 13);
	    int32_t	u1		: bitfield(14, 19);
	    int32_t	width		: bitfield(20, 29);
	} detail;
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
	    int32_t	end		: bitfield( 0, 11);
	    int32_t	u0		: bitfield(12, 15);
	    int32_t	total		: bitfield(16, 27);
	} detail;
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
	    int32_t	start		: bitfield( 0, 11);
	    int32_t	u0		: bitfield(12, 15);
	    int32_t	width		: bitfield(16, 23);
	} detail;
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
	    int32_t	end		: bitfield( 0, 10);
	    int32_t	u0		: bitfield(11, 15);
	    int32_t	total		: bitfield(16, 26);
	} detail;
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
	    int32_t	start		: bitfield( 0, 11);
	    int32_t	u0		: bitfield(12, 15);
	    int32_t	height		: bitfield(16, 21);
	} detail;
	int32_t		value;
    } crt_vsync;
} MSOCRegRec, *MSOCRegPtr;

#define PANEL_PALETTE			0x080400
#define CRT_PALETTE			0x080c00

/* In Kb - documentation says it is 64Kb... */
#define FB_RESERVE4USB			512


Bool SMI501_EnterVT(int scrnIndex, int flags);
void SMI501_LeaveVT(int scrnIndex, int flags);
void SMI501_Save(ScrnInfoPtr pScrn);
void SMI501_DisplayPowerManagementSet(ScrnInfoPtr pScrn,
				      int PowerManagementMode, int flags);
Bool SMI501_ModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
void SMI501_LoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
			LOCO *colors, VisualPtr pVisual);

#endif  /*_SMI_501_H*/
