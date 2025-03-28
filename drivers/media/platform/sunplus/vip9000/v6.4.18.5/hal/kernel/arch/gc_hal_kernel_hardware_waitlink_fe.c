/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2014 - 2024 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************
*
*    The GPL License (GPL)
*
*    Copyright (C) 2014 - 2024 Vivante Corporation
*
*    This program is free software; you can redistribute it and/or
*    modify it under the terms of the GNU General Public License
*    as published by the Free Software Foundation; either version 2
*    of the License, or (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*****************************************************************************
*
*    Note: This software is released under dual MIT and GPL licenses. A
*    recipient may use this file under the terms of either the MIT license or
*    GPL License. If you wish to use only one license not the other, you can
*    indicate your decision by deleting one of the above license notices in your
*    version of this file.
*
*****************************************************************************/

#include "gc_hal.h"
#include "gc_hal_kernel.h"
#include "gc_hal_kernel_context.h"

#define _GC_OBJ_ZONE gcvZONE_HARDWARE

gceSTATUS
gckWLFE_Construct(gckHARDWARE Hardware, gckWLFE *FE)
{
    /* Just a non-null value. */
    *FE = (gckWLFE)(gctUINTPTR_T)1;
    return gcvSTATUS_OK;
}

void
gckWLFE_Destroy(gckHARDWARE Hardware, gckWLFE FE)
{
    gcmkASSERT(FE);
}

gceSTATUS
gckWLFE_Initialize(gckHARDWARE Hardware, gckWLFE FE)
{
    gcmkASSERT(FE);
    return gcvSTATUS_OK;
}

/*******************************************************************************
 *
 *  gckWLFE_WaitLink
 *
 *  Append a WAIT/LINK command sequence at the specified location in the command
 *  queue.
 *
 *  INPUT:
 *
 *      gckHARDWARE Hardware
 *          Pointer to an gckHARDWARE object.
 *
 *      gctPOINTER Logical
 *          Pointer to the current location inside the command queue to append
 *          WAIT/LINK command sequence at or gcvNULL just to query the size of
 *          the WAIT/LINK command sequence.
 *
 *      gctADDRESS Address
 *          GPU address of current location inside the command queue.
 *
 *      gctUINT32 Offset
 *          The Offset into command buffer required for alignment.
 *
 *      gctSIZE_T *Bytes
 *          Pointer to the number of bytes available for the WAIT/LINK command
 *          sequence.  If 'Logical' is gcvNULL, this argument will be ignored.
 *
 *  OUTPUT:
 *
 *      gctSIZE_T *Bytes
 *          Pointer to a variable that will receive the number of bytes
 *          required by the WAIT/LINK command sequence.  If 'Bytes' is gcvNULL,
 *          nothing will be returned.
 *
 *      gctUINT32 *WaitOffset
 *          Pointer to a variable that will receive the offset of the WAIT
 *          command from the specified logcial pointer. If 'WaitOffset' is
 *          gcvNULL nothing will be returned.
 *
 *      gctSIZE_T *WaitSize
 *          Pointer to a variable that will receive the number of bytes used by
 *          the WAIT command.  If 'LinkSize' is gcvNULL nothing will be
 *          returned.
 */
gceSTATUS
gckWLFE_WaitLink(gckHARDWARE Hardware,
                 gctPOINTER Logical,
                 gctADDRESS Address,
                 gctUINT32 Offset,
                 gctUINT32 *Bytes,
                 gctUINT32 *WaitOffset,
                 gctUINT32 *WaitSize)
{
    gceSTATUS status;
    gctUINT32_PTR logical;
    gctUINT32 bytes;
    gctBOOL useL2;
    gctUINT32 address;

    gcmkHEADER_ARG("Hardware=0x%x Logical=0x%x Offset=0x%08x *Bytes=0x%x",
                   Hardware, Logical, Offset, gcmOPT_VALUE(Bytes));

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmkVERIFY_ARGUMENT((Logical != gcvNULL) || (Bytes != gcvNULL));

    gcmkASSERT(Hardware->wlFE);
    useL2 = gckHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_64K_L2_CACHE);

    /* Compute number of bytes required. */
    if (useL2)
        bytes = gcmALIGN(Offset + 24, 8) - Offset;
    else
        bytes = gcmALIGN(Offset + 16, 8) - Offset;

    /* Cast the input pointer. */
    logical = (gctUINT32_PTR)Logical;

    if (logical != gcvNULL) {
        /* Not enough space? */
        if (*Bytes < bytes) {
            /* Command queue too small. */
            gcmkONERROR(gcvSTATUS_BUFFER_TOO_SMALL);
        }

        gcmkASSERT(Address != gcvINVALID_ADDRESS);

        /* Store the WAIT/LINK address. */
        Hardware->lastWaitLink = Address;

        /* Append WAIT(count). */
        *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (Hardware->waitCount) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

        logical++;

        if (useL2) {
            /* LoadState(AQFlush, 1), flush. */
            *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) |
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) |
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));

            *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
        }

        /* Append LINK(2, address). */
        *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x08 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (bytes >> 3) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

        /* Command buffer address won't beyond 4GB. */
        gcmkSAFECASTVA(address, Address);

        *logical = address;

        gcmkONERROR(gckOS_MemoryBarrier(Hardware->os, logical));

        gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE, "0x%x: WAIT %u",
                       address, Hardware->waitCount);

        gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE, "0x%x: LINK 0x%x, #0x%x",
                       address + 8, address, bytes);

        if (WaitOffset != gcvNULL) {
            /* Return the offset pointer to WAIT command. */
            *WaitOffset = 0;
        }

        if (WaitSize != gcvNULL) {
            /* Return number of bytes used by the WAIT command. */
            if (useL2)
                *WaitSize = 16;
            else
                *WaitSize = 8;
        }
    }

    if (Bytes != gcvNULL) {
        /* Return number of bytes required by the WAIT/LINK command
         * sequence.
         */
        *Bytes = bytes;
    }

    /* Success. */
    gcmkFOOTER_ARG("*Bytes=0x%x *WaitOffset=0x%x *WaitSize=0x%x",
                   gcmOPT_VALUE(Bytes), gcmOPT_VALUE(WaitOffset), gcmOPT_VALUE(WaitSize));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmkFOOTER();
    return status;
}

gceSTATUS
gckWLFE_InvalidatePipe(gckHARDWARE Hardware,
                       gctPOINTER Logical,
                       gctADDRESS Address,
                       gctUINT32 *Bytes)
{
    gctUINT size;
    gctUINT32_PTR logical = (gctUINT32_PTR)Logical;
    gceSTATUS status;
    gctBOOL blt = gcvFALSE;
    gctBOOL multiCluster = gcvFALSE;

    gcmkHEADER_ARG("Hardware=0x%x Logical=0x%x *Bytes=0x%x",
                   Hardware, Logical, gcmOPT_VALUE(Bytes));

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmkVERIFY_ARGUMENT((Logical == gcvNULL) || (Bytes != gcvNULL));

    gcmkASSERT(Hardware->wlFE);

    if (gckHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_BLT_ENGINE)) {
        /* Send all event from blt. */
        blt = gcvTRUE;
        multiCluster = gckHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_MULTI_CLUSTER);
    }

    /* Determine the size of the command. */
    size = Hardware->extraEventStates ?
           gcmALIGN(8 + (1 + 5) * 4, 8) /* EVENT + 5 STATES */
           :
           8;

    if (blt) {
        size += 16;
        if (multiCluster)
            size += 8;
    }

    /* END. */
    size += 8;

    if (Logical != gcvNULL) {
        if (*Bytes < size) {
            /* Command queue too small. */
            gcmkONERROR(gcvSTATUS_BUFFER_TOO_SMALL);
        }

        if (blt) {
            *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) |
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) |
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x502E) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

            *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)));

            if (multiCluster) {
                *logical++ =
                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) |
                         ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) |
                         ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x50CE) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

                *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0))) | (((gctUINT32) ((gctUINT32) (Hardware->identity.clusterAvailMask & Hardware->options.userClusterMask) & ((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0)));
            }
        }

        /* Append EVENT(Event, PE_SRC). */
        *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E01) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));

        *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (EVENT_ID_INVALIDATE_PIPE) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)));

        if (blt) {
            *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) |
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) |
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x502E) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

            *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)));
        }

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
        {
            gctPHYS_ADDR_T phys;

            gckOS_GetPhysicalAddress(Hardware->os, Logical, &phys);
            gckOS_CPUPhysicalToGPUPhysical(Hardware->os, phys, &phys);
            gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE,
                           "0x%08llx: EVENT %d", phys, EVENT_ID_INVALIDATE_PIPE);
        }
#endif

        /* Append the extra states. These are needed for the chips that do not
         * support back-to-back events due to the async interface. The extra
         * states add the necessary delay to ensure that event IDs do not
         * collide.
         */
        if (Hardware->extraEventStates) {
            *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) |
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0100) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) |
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (5) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));
            *logical++ = 0;
            *logical++ = 0;
            *logical++ = 0;
            *logical++ = 0;
            *logical++ = 0;
        }

#if gcdINTERRUPT_STATISTIC
        if (Hardware->kernel->eventObj->totalQueueCount > EVENT_ID_INVALIDATE_PIPE) {
            gckOS_AtomSetMask(Hardware->pendingEvent,
                              1 << EVENT_ID_INVALIDATE_PIPE);
        }
#endif

        /* Append END. */
        *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x02 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

        /* Record the count of execution which is finised by this END. */
        *logical++ = Hardware->executeCount;

        gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE, "%p: END", Logical);

        /* Make sure the CPU writes out the data to memory. */
        gcmkONERROR(gckOS_MemoryBarrier(Hardware->os, Logical));

        Hardware->lastEnd = Address + size - 8;
    }

    if (Bytes != gcvNULL) {
        /* Return number of bytes required by the EVENT command. */
        *Bytes = size;
    }

    /* Success. */
    gcmkFOOTER_ARG("*Bytes=0x%x", gcmOPT_VALUE(Bytes));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmkFOOTER();
    return status;
}

void
gckWLFE_DoneInvalidatePipe(gckHARDWARE Hardware)
{
    gctUINT32 resume;
    gctUINT32 bytes;
    gctUINT32 idle;
    gctUINT32 pageSize = Hardware->kernel->command->pageSize;

    gcmkASSERT(Hardware->wlFE);

    /* Make sure FE is idle. */
    do {
        gcmkVERIFY_OK(gckOS_ReadRegisterEx(Hardware->os, Hardware->kernel,
                                           0x00004, &idle));
    } while (idle != 0x7FFFFFFF);

    gcmkVERIFY_OK(gckOS_ReadRegisterEx(Hardware->os, Hardware->kernel,
                                       0x00664,
                                       &resume));

    gcmkVERIFY_OK(gckOS_ReadRegisterEx(Hardware->os, Hardware->kernel,
                                       0x00664,
                                       &resume));

    gcmkVERIFY_OK(gckWLFE_WaitLink(Hardware, gcvNULL, gcvINVALID_ADDRESS,
                                   resume & (pageSize - 1),
                                   &bytes, gcvNULL, gcvNULL));

    /* Start Command Parser. */
    gcmkVERIFY_OK(gckWLFE_Execute(Hardware, resume, bytes));
}

/******************************************************************************
 *
 *  gckWLFE_Link
 *
 *  Append a LINK command at the specified location in the command queue.
 *
 *  INPUT:
 *
 *      gckHARDWARE Hardware
 *          Pointer to an gckHARDWARE object.
 *
 *      gctPOINTER Logical
 *          Pointer to the current location inside the command queue to append
 *          the LINK command at or gcvNULL just to query the size of the LINK
 *          command.
 *
 *      gctADDRESS FetchAddress
 *          Hardware address of destination of LINK.
 *
 *      gctSIZE_T FetchSize
 *          Number of bytes in destination of LINK.
 *
 *      gctSIZE_T *Bytes
 *          Pointer to the number of bytes available for the LINK command.  If
 *          'Logical' is gcvNULL, this argument will be ignored.
 *
 *  OUTPUT:
 *
 *      gctSIZE_T *Bytes
 *          Pointer to a variable that will receive the number of bytes required
 *          for the LINK command.  If 'Bytes' is gcvNULL, nothing will be
 *          returned.
 */
gceSTATUS
gckWLFE_Link(gckHARDWARE Hardware,
             gctPOINTER Logical,
             gctADDRESS FetchAddress,
             gctUINT32 FetchSize,
             gctUINT32 *Bytes,
             gctUINT32 *Low,
             gctUINT32 *High)
{
    gceSTATUS status;
    gctSIZE_T bytes;
    gctUINT32 link, address;
    gctUINT32_PTR logical = (gctUINT32_PTR)Logical;

    gcmkHEADER_ARG("Hardware=0x%x Logical=0x%x FetchAddress=0x%x FetchSize=0x%x *Bytes=0x%x",
                   Hardware, Logical, FetchAddress, FetchSize, gcmOPT_VALUE(Bytes));

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmkVERIFY_ARGUMENT((Logical == gcvNULL) || (Bytes != gcvNULL));

    gcmkASSERT(Hardware->wlFE);

    if (Logical != gcvNULL) {
        if (*Bytes < 8) {
            /* Command queue too small. */
            gcmkONERROR(gcvSTATUS_BUFFER_TOO_SMALL);
        }

        gcmkSAFECASTVA(address, FetchAddress);

        gcmkONERROR(gckOS_WriteMemory(Hardware->os, logical + 1, address));

        if (High)
            *High = address;

        /* Make sure the address got written before the LINK command. */
        gcmkONERROR(gckOS_MemoryBarrier(Hardware->os, logical + 1));

        /* Compute number of 64-byte aligned bytes to fetch. */
        bytes = gcmALIGN(address + FetchSize, 64) - address;

        /* Append LINK(bytes / 8), FetchAddress. */
        link = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x08 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) |
                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (bytes >> 3) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

        gcmkONERROR(gckOS_WriteMemory(Hardware->os, logical, link));

        if (Low)
            *Low = link;

        /* Memory barrier. */
        gcmkONERROR(gckOS_MemoryBarrier(Hardware->os, logical));
    }

    if (Bytes != gcvNULL) {
        /* Return number of bytes required by the LINK command. */
        *Bytes = 8;
    }

    /* Success. */
    gcmkFOOTER_ARG("*Bytes=0x%x", gcmOPT_VALUE(Bytes));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmkFOOTER();
    return status;
}

/******************************************************************************
 *
 *  gckWLFE_End
 *
 *  Append an END command at the specified location in the command queue.
 *
 *  INPUT:
 *
 *      gckHARDWARE Hardware
 *          Pointer to an gckHARDWARE object.
 *
 *      gctPOINTER Logical
 *          Pointer to the current location inside the command queue to append
 *          END command at or gcvNULL just to query the size of the END
 *          command.
 *
 *      gctADDRESS Address
 *          GPU address of current location inside the command queue.
 *
 *      gctUINT32 User
 *          If we write the user logical or not.
 *
 *      gctSIZE_T *Bytes
 *          Pointer to the number of bytes available for the END command.  If
 *          'Logical' is gcvNULL, this argument will be ignored.
 *
 *  OUTPUT:
 *
 *      gctSIZE_T *Bytes
 *          Pointer to a variable that will receive the number of bytes
 *          required for the END command.  If 'Bytes' is gcvNULL, nothing
 *          will be returned.
 */
gceSTATUS
gckWLFE_EndEx(gckHARDWARE Hardware,
              gctPOINTER Logical,
              gctADDRESS Address,
              gctBOOL User,
              gctUINT32 *Bytes)
{
    gctUINT32_PTR logical = (gctUINT32_PTR)Logical;
    gctUINT32 end;
    gceSTATUS status;

    gcmkHEADER_ARG("Hardware=0x%x Logical=0x%x *Bytes=0x%x",
                   Hardware, Logical, gcmOPT_VALUE(Bytes));

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmkVERIFY_ARGUMENT((Logical == gcvNULL) || (Bytes != gcvNULL));

    gcmkASSERT(Hardware->wlFE);

    if (Logical != gcvNULL) {
        if (*Bytes < 8) {
            /* Command queue too small. */
            gcmkONERROR(gcvSTATUS_BUFFER_TOO_SMALL);
        }

        end = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x02 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

        if (User) {
            /* Append END. */
            gcmkONERROR(gckOS_WriteMemory(Hardware->os, logical, end));

            /* Record the count of execution which is finised by this END. */
            gcmkONERROR(gckOS_WriteMemory(Hardware->os, logical + 1, Hardware->executeCount));
        } else {
            /* Append END. */
            logical[0] = end;
            /* Record the count of execution which is finised by this END. */
            logical[1] = Hardware->executeCount;
        }

        gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE, "%p: END", Logical);

        /* Make sure the CPU writes out the data to memory. */
        gcmkONERROR(gckOS_MemoryBarrier(Hardware->os, Logical));

        gcmkASSERT(Address != ~0U);

        Hardware->lastEnd = Address;
    }

    if (Bytes != gcvNULL) {
        /* Return number of bytes required by the END command. */
        *Bytes = 8;
    }

    /* Success. */
    gcmkFOOTER_ARG("*Bytes=0x%x", gcmOPT_VALUE(Bytes));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmkFOOTER();
    return status;
}

gceSTATUS
gckWLFE_End(gckHARDWARE Hardware,
            gctPOINTER Logical,
            gctADDRESS Address,
            gctUINT32 *Bytes)
{
    return gckWLFE_EndEx(Hardware, Logical, Address, gcvFALSE, Bytes);
}

/******************************************************************************
 *
 *  gckWLFE_Nop
 *
 *  Append a NOP command at the specified location in the command queue.
 *
 *  INPUT:
 *
 *      gckHARDWARE Hardware
 *          Pointer to an gckHARDWARE object.
 *
 *      gctPOINTER Logical
 *          Pointer to the current location inside the command queue to append
 *          NOP command at or gcvNULL just to query the size of the NOP
 *          command.
 *
 *      gctSIZE_T *Bytes
 *          Pointer to the number of bytes available for the NOP command.  If
 *          'Logical' is gcvNULL, this argument will be ignored.
 *
 *  OUTPUT:
 *
 *      gctSIZE_T *Bytes
 *          Pointer to a variable that will receive the number of bytes
 *          required for the NOP command.  If 'Bytes' is gcvNULL, nothing
 *          will be returned.
 */
gceSTATUS
gckWLFE_Nop(gckHARDWARE Hardware, gctPOINTER Logical, gctSIZE_T *Bytes)
{
    gctUINT32_PTR logical = (gctUINT32_PTR)Logical;
    gceSTATUS status;

    gcmkHEADER_ARG("Hardware=0x%x Logical=0x%x *Bytes=0x%zx",
                   Hardware, Logical, gcmOPT_VALUE(Bytes));

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmkVERIFY_ARGUMENT((Logical == gcvNULL) || (Bytes != gcvNULL));

    gcmkASSERT(Hardware->wlFE);

    if (Logical != gcvNULL) {
        if (*Bytes < 8) {
            /* Command queue too small. */
            gcmkONERROR(gcvSTATUS_BUFFER_TOO_SMALL);
        }

        /* Append NOP. */
        logical[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x03 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

        gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE, "%p: NOP", Logical);
    }

    if (Bytes != gcvNULL) {
        /* Return number of bytes required by the NOP command. */
        *Bytes = 8;
    }

    /* Success. */
    gcmkFOOTER_ARG("*Bytes=0x%zx", gcmOPT_VALUE(Bytes));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmkFOOTER();
    return status;
}

/******************************************************************************
 *
 *  gckWLFE_Event
 *
 *  Append an EVENT command at the specified location in the command queue.
 *
 *  INPUT:
 *
 *      gckHARDWARE Hardware
 *          Pointer to an gckHARDWARE object.
 *
 *      gctPOINTER Logical
 *          Pointer to the current location inside the command queue to append
 *          the EVENT command at or gcvNULL just to query the size of the EVENT
 *          command.
 *
 *      gctUINT8 Event
 *          The Event ID to program.
 *
 *      gceKERNEL_WHERE FromWhere
 *          Location of the pipe to send the event.
 *
 *      gctSIZE_T *Bytes
 *          Pointer to the number of bytes available for the EVENT command.  If
 *          'Logical' is gcvNULL, this argument will be ignored.
 *
 *  OUTPUT:
 *
 *      gctSIZE_T *Bytes
 *          Pointer to a variable that will receive the number of bytes
 *          required for the EVENT command.  If 'Bytes' is gcvNULL, nothing
 *          will be returned.
 */
gceSTATUS
gckWLFE_Event(gckHARDWARE Hardware,
              gctPOINTER Logical,
              gctUINT8 Event,
              gceKERNEL_WHERE FromWhere,
              gctUINT32 *Bytes)
{
    gctUINT size;
    gctUINT32 destination = 0;
    gctUINT32_PTR logical = (gctUINT32_PTR)Logical;
    gceSTATUS status;
    gctBOOL blt;
    gctBOOL extraEventStates;
    gctBOOL multiCluster;

    gcmkHEADER_ARG("Hardware=0x%x Logical=0x%x Event=%u FromWhere=%d *Bytes=0x%x",
                   Hardware, Logical, Event, FromWhere, gcmOPT_VALUE(Bytes));

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmkVERIFY_ARGUMENT((Logical == gcvNULL) || (Bytes != gcvNULL));
    gcmkVERIFY_ARGUMENT(Event < 32);

    gcmkASSERT(Hardware->wlFE);

    if (gckHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_BLT_ENGINE)) {
        /* Send all event from blt. */
        if (FromWhere == gcvKERNEL_PIXEL)
            FromWhere = gcvKERNEL_BLT;
    }

    if (Hardware->identity.chipModel == 0x8400 &&
        Hardware->identity.customerID == 0x54)
        FromWhere = gcvKERNEL_PIXEL;

    blt = FromWhere == gcvKERNEL_BLT ? gcvTRUE : gcvFALSE;

    multiCluster = gckHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_MULTI_CLUSTER);

    /* Determine the size of the command. */

    extraEventStates = Hardware->extraEventStates && (FromWhere == gcvKERNEL_PIXEL);

    size = extraEventStates ?
           gcmALIGN(8 + (1 + 5) * 4, 8) /* EVENT + 5 STATES */
           :
           8;

    if (blt) {
        size += 16;
        if (multiCluster)
            size += 8;
    }

    if (Logical != gcvNULL) {
        if (*Bytes < size) {
            /* Command queue too small. */
            gcmkONERROR(gcvSTATUS_BUFFER_TOO_SMALL);
        }

        switch (FromWhere) {
        case gcvKERNEL_COMMAND:
            /* From command processor. */
            destination = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5)));
            break;

        case gcvKERNEL_PIXEL:
            /* From pixel engine. */
            destination = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
            break;

        case gcvKERNEL_BLT:
            destination = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)));
            break;

        default:
            gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        if (blt) {
            *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) |
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) |
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x502E) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

            *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)));

            if (multiCluster) {
                *logical++ =
                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) |
                         ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) |
                         ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x50CE) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

                *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0))) | (((gctUINT32) ((gctUINT32) (Hardware->identity.clusterAvailMask & Hardware->options.userClusterMask) & ((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0)));
            }
        }

        /* Append EVENT(Event, destination). */
        *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E01) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));

        *logical++ = ((((gctUINT32) (destination)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (Event) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)));

        if (blt) {
            *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) |
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) |
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x502E) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

            *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)));
        }

        /* Make sure the event ID gets written out before GPU can access it. */
        gcmkONERROR(gckOS_MemoryBarrier(Hardware->os, logical + 1));

        /* Append the extra states. These are needed for the chips that do not
         * support back-to-back events due to the async interface. The extra
         * states add the necessary delay to ensure that event IDs do not
         * collide.
         */
        if (extraEventStates) {
            *logical++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) |
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0100) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) |
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (5) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));
            *logical++ = 0;
            *logical++ = 0;
            *logical++ = 0;
            *logical++ = 0;
            *logical++ = 0;
        }

#if gcdINTERRUPT_STATISTIC
        if (Event < (gctUINT8)Hardware->kernel->eventObj->totalQueueCount)
            gckOS_AtomSetMask(Hardware->pendingEvent, 1 << Event);
#endif
    }

    if (Bytes != gcvNULL) {
        /* Return number of bytes required by the EVENT command. */
        *Bytes = size;
    }

    /* Success. */
    gcmkFOOTER_ARG("*Bytes=0x%x", gcmOPT_VALUE(Bytes));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmkFOOTER();
    return status;
}

gceSTATUS
gckWLFE_ChipEnable(gckHARDWARE Hardware,
                   gctPOINTER Logical,
                   gceCORE_3D_MASK ChipEnable,
                   gctSIZE_T *Bytes)
{
    gckOS os = Hardware->os;
    gctUINT32_PTR logical = (gctUINT32_PTR)Logical;
    gceSTATUS status;

    gcmkHEADER_ARG("Hardware=0x%x Logical=0x%x ChipEnable=0x%x *Bytes=0x%zx",
                   Hardware, Logical, ChipEnable, gcmOPT_VALUE(Bytes));

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmkVERIFY_ARGUMENT((Logical == gcvNULL) || (Bytes != gcvNULL));

    gcmkASSERT(Hardware->wlFE);

    if (Logical != gcvNULL) {
        if (*Bytes < 8) {
            /* Command queue too small. */
            gcmkONERROR(gcvSTATUS_BUFFER_TOO_SMALL);
        }

        /* Append CHIPENABLE. */
        gcmkWRITE_MEMORY(logical,
                         ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) ==
 32) ? ~
0U : (~
(~0U << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ChipEnable);

        gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE,
                       "%p: CHIPENABLE 0x%x", Logical, ChipEnable);
    }

    if (Bytes != gcvNULL) {
        /* Return number of bytes required by the CHIPENABLE command. */
        *Bytes = 8;
    }

    /* Success. */
    gcmkFOOTER_ARG("*Bytes=0x%zx", gcmOPT_VALUE(Bytes));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmkFOOTER();
    return status;
}

/******************************************************************************
 *
 *  gckWLFE_Execute
 *
 *  Kickstart the hardware's command processor with an initialized command
 *  buffer.
 *
 *  INPUT:
 *
 *      gckHARDWARE Hardware
 *          Pointer to the gckHARDWARE object.
 *
 *      gctADDRESS Address
 *          Hardware address of command buffer.
 *
 *      gctUINT32 Bytes
 *          Number of bytes for the prefetch unit (until after the first LINK).
 *
 *  OUTPUT:
 *
 *      Nothing.
 */
gceSTATUS
gckWLFE_Execute(gckHARDWARE Hardware, gctADDRESS Address, gctUINT32 Bytes)
{
    gceSTATUS status;
    gctUINT32 control;
    gctUINT32 eventEnable = 0xFFFFFFFF;
    gckCOMMAND command = Hardware->kernel->command;
    gctUINT32 address;

    gcmkHEADER_ARG("Hardware=0x%x Address=0x%llx Bytes=0x%x", Hardware, Address, Bytes);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    gcmkASSERT(Hardware->wlFE);
    gcmkASSERT(command);

    if (command->feType == gcvHW_FE_END) {
        gctUINT idle = 0;
        gctUINT32 timer = 0, delay = 10;

        /* Make sure FE is idle. */
        do {
            gcmkVERIFY_OK(gckOS_Udelay(Hardware->os, delay));

            gcmkVERIFY_OK(gckOS_ReadRegisterEx(Hardware->os, Hardware->kernel,
                                               0x00004, &idle));

            timer += delay;
            delay *= 2;

#if gcdGPU_TIMEOUT
            if (timer >= Hardware->kernel->timeOut * 1000) {
                gcmkPRINT("[Galcore]: GPU timeout...\n");
                gcmkONERROR(gcvSTATUS_DEVICE);
            }
#endif
        } while (idle != 0x7FFFFFFF);
    }

    /* Enable all events. */
    gcmkONERROR(gckOS_WriteRegisterEx(Hardware->os, Hardware->kernel,
                                      0x00014, eventEnable));

    if (gckHARDWARE_IsFeatureAvailable(Hardware, gcvFEATURE_BIT_SRAM_PARITY)) {
        gcmkONERROR(gckOS_WriteRegisterEx(Hardware->os, Hardware->kernel,
                                          0x00344, 0x1));
    }

    gcmkSAFECASTVA(address, Address);

    /* Write address register. */
    gcmkONERROR(gckOS_WriteRegisterEx(Hardware->os, Hardware->kernel,
                                      0x00654, address));

    /* Build control register. */
    control = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) |
                   ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) ((Bytes + 7) >> 3) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    /* Set big endian */
    if (Hardware->bigEndian)
        control |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) ==
 32) ? ~0U : (~(~0U << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20)));

    /* Make sure writing to command buffer and previous AHB register is done. */
    gcmkONERROR(gckOS_MemoryBarrier(Hardware->os, gcvNULL));

    if (Hardware->type == gcvHARDWARE_2D) {
        gcmkONERROR(gckOS_WriteRegisterEx(Hardware->os, Hardware->kernel,
                                          0x00658, control));
    } else {
        /* Write control register. */
        switch (Hardware->options.secureMode) {
        case gcvSECURE_NONE:
            gcmkONERROR(gckOS_WriteRegisterEx(Hardware->os, Hardware->kernel,
                                              0x00658, control));
            break;
        case gcvSECURE_IN_NORMAL:
#if defined(__KERNEL__)
            gcmkONERROR(gckOS_WriteRegisterEx(Hardware->os, Hardware->kernel,
                                              0x00658, control));
#endif
            gcmkONERROR(gckOS_WriteRegisterEx(Hardware->os, Hardware->kernel,
                                              0x003A4, control));
            break;
#if gcdENABLE_TRUST_APPLICATION
        case gcvSECURE_IN_TA:
            /* Send message to TA. */
            gcmkONERROR(gckKERNEL_SecurityStartCommand(Hardware->kernel, address, (gctUINT32)Bytes));
            break;
#endif
        default:
            break;
        }
    }

    /* Increase execute count. */
    Hardware->executeCount++;

    /* Record last execute address. */
    Hardware->lastExecuteAddress = Address;

    gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE,
                   "Started command buffer @ 0x%llx", Address);

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmkFOOTER();
    return status;
}

