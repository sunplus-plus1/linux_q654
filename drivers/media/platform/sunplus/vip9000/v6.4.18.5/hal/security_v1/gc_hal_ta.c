/****************************************************************************
*    Copyright (C) 2005 - 2024 by Vivante Corp.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the license, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****************************************************************************/

#include "gc_hal_types.h"
#include "gc_hal_base.h"
#include "gc_hal_security_interface.h"
#include "gc_hal_ta.h"
#include "gc_hal.h"

#define _GC_OBJ_ZONE gcvZONE_KERNEL

/*
* Responsibility of TA (trust application).
* 1) Start FE.
*   When non secure driver asks for start FE. TA enable MMU and start FE.
*   TA always execute MMU enable processes because it has no idea whether
*   GPU has been power off.
*
* 2) Setup page table
*   When non secure driver asks for set up GPU address to physical address
*   mapping, TA check the attribute of physical address and attribute of
*   GPU address to make sure they are match. Then it change page table.
*
*/

gcTA_MMU SharedMmu = gcvNULL;

/*******************************************************************************
**
**  gcTA_Construct
**
**  Construct a new gcTA object.
*/
int
gcTA_Construct(
    IN gctaOS Os,
    IN gceCORE Core,
    OUT gcTA *TA
    )
{
    gceSTATUS status;
    gctPOINTER pointer;
    gcTA ta = gcvNULL;

    gcmkHEADER();
    gcmkVERIFY_ARGUMENT(TA != gcvNULL);

    /* Construct a gcTA object. */
    gcmkONERROR(gctaOS_Allocate(sizeof(struct _gcTA), &pointer));

    gctaOS_ZeroMemory(pointer, sizeof(struct _gcTA));

    ta = (gcTA)pointer;

    ta->os = Os;
    ta->core = Core;

    gcmkONERROR(gctaHARDWARE_Construct(ta, &ta->hardware));

    if (gctaHARDWARE_IsFeatureAvailable(ta->hardware, gcvFEATURE_SECURITY))
    {
        if (SharedMmu == gcvNULL)
        {
            gcmkONERROR(gctaMMU_Construct(ta, &ta->mmu));

            /* Record shared MMU. */
            SharedMmu = ta->mmu;
            ta->destoryMmu = gcvTRUE;
        }
        else
        {
            ta->mmu = SharedMmu;
            ta->destoryMmu = gcvFALSE;
        }

        gcmkONERROR(gctaHARDWARE_PrepareFunctions(ta->hardware));
    }

    *TA = ta;

    gcmkFOOTER_NO();
    return 0;

OnError:
    if (ta)
    {
        if (ta->mmu && ta->destoryMmu)
        {
            gcmkVERIFY_OK(gctaMMU_Destory(ta->mmu));
        }

        if (ta->hardware)
        {
            gcmkVERIFY_OK(gctaHARDWARE_Destroy(ta->hardware));
        }

        gcmkVERIFY_OK(gctaOS_Free(ta));
    }
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcTA_Construct
**
**  Destroy a gcTA object.
*/
int
gcTA_Destroy(
    IN gcTA TA
    )
{
    if (TA->mmu && TA->destoryMmu)
    {
        gcmkVERIFY_OK(gctaMMU_Destory(TA->mmu));
    }

    if (TA->hardware)
    {
        gcmkVERIFY_OK(gctaHARDWARE_Destroy(TA->hardware));
    }

    gcmkVERIFY_OK(gctaOS_Free(TA));

    /* Destroy. */
    return 0;
}


/*
*   Map a scatter gather list into gpu address space.
*
*/
gceSTATUS
gcTA_MapMemory(
    IN gcTA TA,
    IN gctUINT32 *PhysicalArray,
    IN gctPHYS_ADDR_T Physical,
    IN gctUINT32 PageCount,
    OUT gctUINT32 *GPUAddress
    )
{
    gceSTATUS status;
    gcTA_MMU mmu;
    gctUINT32 pageCount = PageCount;
    gctUINT32 i;
    gctUINT32 gpuAddress = *GPUAddress;
    gctBOOL mtlbSecure = gcvFALSE;
    gctBOOL physicalSecure = gcvFALSE;

    mmu = TA->mmu;

    /* Fill in page table. */
    for (i = 0; i < pageCount; i++)
    {
        gctUINT32 physical;
        gctUINT32_PTR entry;

        if (PhysicalArray)
        {
            physical = PhysicalArray[i];
        }
        else
        {
            physical = (gctUINT32)Physical + 4096 * i;
        }

        gcmkONERROR(gctaMMU_GetPageEntry(mmu, gpuAddress, gcvNULL, &entry, &mtlbSecure));

        status = gctaOS_IsPhysicalSecure(TA->os, physical, &physicalSecure);

        if (gcmIS_SUCCESS(status) && physicalSecure != mtlbSecure)
        {
            gcmkONERROR(gcvSTATUS_NOT_SUPPORTED);
        }

        gctaMMU_SetPage(mmu, physical, entry);

        gpuAddress += 4096;
    }

    return gcvSTATUS_OK;

OnError:
    return status;
}

gceSTATUS
gcTA_UnmapMemory(
    IN gcTA TA,
    IN gctUINT32 GPUAddress,
    IN gctUINT32 PageCount
    )
{
    gceSTATUS status;

    gcmkONERROR(gctaMMU_FreePages(TA->mmu, GPUAddress, PageCount));

    return gcvSTATUS_OK;

OnError:
    return status;
}

gceSTATUS
gcTA_StartCommand(
    IN gcTA TA,
    IN gctUINT32 Address,
    IN gctUINT32 Bytes
    )
{
    gctaHARDWARE_Execute(TA, Address, Bytes);
    return gcvSTATUS_OK;
}

int
gcTA_Dispatch(
    IN gcTA TA,
    IN gcsTA_INTERFACE * Interface
    )
{
    int command = Interface->command;

    gceSTATUS status = gcvSTATUS_OK;

    switch (command)
    {
    case KERNEL_START_COMMAND:
        /* Enable MMU every time FE starts.
        ** Because if normal world stop GPU and power off GPU, MMU states is reset.
        */
        gcmkONERROR(gctaHARDWARE_SetMMU(TA->hardware, TA->mmu->mtlbLogical));

        gcmkONERROR(gcTA_StartCommand(
            TA,
            (gctUINT32)Interface->u.StartCommand.address,
            Interface->u.StartCommand.bytes
            ));
        break;

    case KERNEL_MAP_MEMORY:
        gcmkONERROR(gcTA_MapMemory(
            TA,
            Interface->u.MapMemory.physicals,
            Interface->u.MapMemory.physical,
            Interface->u.MapMemory.pageCount,
            (gctUINT32 *)&Interface->u.MapMemory.gpuAddress
            ));

        break;

    case KERNEL_UNMAP_MEMORY:
        status = gcTA_UnmapMemory(
            TA,
            (gctUINT32)Interface->u.UnmapMemory.gpuAddress,
            Interface->u.UnmapMemory.pageCount
            );
        break;

    case KERNEL_DUMP_MMU_EXCEPTION:
        status = gctaHARDWARE_DumpMMUException(TA->hardware);
        break;

    case KERNEL_HANDLE_MMU_EXCEPTION:
        status = gctaHARDWARE_HandleMMUException(
            TA->hardware,
            Interface->u.HandleMMUException.mmuStatus,
            Interface->u.HandleMMUException.physical,
            (gctUINT32)Interface->u.HandleMMUException.gpuAddress
            );
        break;

    case KERNEL_READ_MMU_EXCEPTION:
        status = gctaHARDWARE_ReadMMUException(
            TA->hardware,
            &Interface->u.ReadMMUException.mmuStatus,
            &Interface->u.ReadMMUException.mmuException
            );
        break;

    default:
        gcmkASSERT(0);

        status = gcvSTATUS_INVALID_ARGUMENT;
        break;
    }

OnError:
    Interface->result = status;

    return 0;
}




