/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
 * Copyright (c) 2009 Dmitry Eremin-Solenikov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#include "xf86.h"
/* FIXME: this should go away */
#include "smi.h"

/* FIXME: This should go tinto xf86Helper.c */

int xf86MatchVirtualInstances(const char *driverName, SymTabPtr chipsets,
		      IsaChipsets *ISAchipsets, DriverPtr drvp,
		      FindIsaDevProc FindDevice, GDevPtr *devList,
		      int numDevs, int **foundEntities)
{
    SymTabRec *c;
    IsaChipsets *Chips;
    int i;
    int numFound = 0;
    int foundChip = -1;
    int *retEntities = NULL;

    *foundEntities = NULL;


    for (i = 0; i < numDevs; i++) {
	MessageType from = X_CONFIG;
	GDevPtr dev = NULL;

	if (dev) xf86MsgVerb(X_WARNING,0,
			     "%s: More than one matching "
			     "Device section found: %s\n",
			     driverName,devList[i]->identifier);
	else dev = devList[i];

	if (dev) {
	    if (dev->chipset) {
		for (c = chipsets; c->token >= 0; c++) {
		    if (xf86NameCmp(c->name, dev->chipset) == 0)
			break;
		}
		if (c->token == -1) {
		    xf86MsgVerb(X_WARNING, 0, "%s: Chipset \"%s\" in Device "
				"section \"%s\" isn't valid for this driver\n",
				driverName, dev->chipset,
				dev->identifier);
		} else
		    foundChip = c->token;
	    } else {
		if (FindDevice) foundChip = (*FindDevice)(dev);
                                                        /* Probe it */
		from = X_PROBED;
	    }
	}

	/* Check if the chip type is listed in the chipset table - for sanity*/

	if (foundChip >= 0){
	    for (Chips = ISAchipsets; Chips->numChipset >= 0; Chips++) {
		if (Chips->numChipset == foundChip)
		    break;
	    }
	    if (Chips->numChipset == -1){
		foundChip = -1;
		xf86MsgVerb(X_WARNING,0,
			    "%s: Driver detected unknown Platform Bus Chipset\n",
			    driverName);
	    }
	}
	if (foundChip != -1) {
	    numFound++;
	    retEntities = xnfrealloc(retEntities,numFound * sizeof(int));
	    retEntities[numFound - 1] =
	    xf86ClaimNoSlot(drvp,foundChip,dev, dev->active ? TRUE : FALSE);
	    for (c = chipsets; c->token >= 0; c++) {
		if (c->token == foundChip)
		    break;
	    }
	    xf86Msg(from, "Chipset %s found\n", c->name);
	}
    }
    *foundEntities = retEntities;

    return numFound;
}


