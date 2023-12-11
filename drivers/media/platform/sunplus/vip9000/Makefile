#
# (C) COPYRIGHT 2018-2021 Arm Limited.
#
# This program is free software and is provided to you under the terms of the
# GNU General Public License version 2 as published by the Free Software
# Foundation, and any use by you of this program is subject to the terms
# of such GNU licence.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you can access it online at
# http://www.gnu.org/licenses/gpl-2.0.html.
#
# SPDX-License-Identifier: GPL-2.0-only
#

#ccflags-y += -Wall -g -DDEBUG -DETHOSN_NS
#ccflags-y += -Wall -g -Werror -DDEBUG -DETHOSN_NS
#ccflags-y += -Wall -g -Werror -DDEBUG $(EXTRA_CCFLAGS)
#ccflags-y += -Wall -g -Werror -DDEBUG
# ccflags-y += -Wall -Werror $(EXTRA_CCFLAGS)

ccflags-y += -Wall -Werror -DNO_DMA_COHERENT=1 -DUSE_VDK=1 -DEGL_API_WL=0 \
			 -DEGL_API_FB=1 -DEGL_API_DFB=0 -DEGL_API_DRI=0 -DEGL_API_X=0 \
			 -DEGL_API_GBM=0 -DX11_DRI3=0 -DEGL_API_NULLWS=0 -DgcdSTATIC_LINK=0 \
			 -DABI=0 -DLINUX_OABI=0 -DDEBUG=0 -DCUSTOM_PIXMAP=0 -DUSE_OPENCL=1 \
			 -DUSE_OPENVX=1 -DUSE_VULKAN=1 -DUSE_FB_DOUBLE_BUFFER=0 -DUSE_PLATFORM_DRIVER=1 \
			 -DENABLE_GPU_CLOCK_BY_DRIVER=0 -DFPGA_BUILD=0 -DGL4_DRI_BUILD=0 \
			 -DVIVANTE_NO_GL4=0 -DVIVANTE_NO_VG=0 -DVIVANTE_ENABLE_DRM=0 \
			 -DVIVANTE_ENABLE_VG=0 -DVIVANTE_ENABLE_3D=1 -DVIVANTE_ENABLE_2D=0 \
			 -DLINUX_EMULATOR=0 -DGPU_CONFIG=VIP9000NANODI_PLUS_PID0X1000000B  \
			 -DVSI_USE_VXC_BINARY=1 -DUSE_VXC_BINARY=1 -DUSE_OVXLIB=1 -DUSE_3D_VG=1 \
			 -DUSE_POWER_MANAGEMENT=1 -DgcdNONPAGED_MEMORY_BUFFERABLE=1 \
			 -DENABLE_NONPAGED_MEMORY_BUFFERABLE=1 -DNONPAGED_MEMORY_BUFFERABLE=1 -DUSE_LOADTIME_OPT=1 -DUSE_DESTRUCTOR=1 -DgcdENABLE_DRM=0 -DHOST=\""KERNEL"\"
ccflags-y += $(EXTRA_CCFLAGS)
#ccflags-y += -I$(src)/hal/inc
ccflags-y += -I$(src)/hal/kernel/inc
ccflags-y += -I$(src)/hal/kernel
ccflags-y += -I$(src)/hal/kernel/arch
ccflags-y += -I$(src)/hal/kernel/inc
ccflags-y += -I$(src)/hal/os/linux/kernel
ccflags-y += -I$(src)/hal/security_v1/
ccflags-y += -I$(src)/hal/os/linux/kernel/allocator/default/

obj-$(CONFIG_VIP9000_NPU) += galcore.o
#obj-y += hal/kernel/inc

galcore-objs := hal/os/linux/kernel/gc_hal_kernel_device.o \
				hal/os/linux/kernel/gc_hal_kernel_linux.o \
				hal/os/linux/kernel/gc_hal_kernel_math.o \
				hal/os/linux/kernel/gc_hal_kernel_os.o \
				hal/os/linux/kernel/gc_hal_kernel_iommu.o \
				hal/os/linux/kernel/gc_hal_kernel_debug.o \
				hal/os/linux/kernel/gc_hal_kernel_debugfs.o \
				hal/os/linux/kernel/gc_hal_kernel_allocator.o \
				hal/os/linux/kernel/allocator/default/gc_hal_kernel_allocator_user_memory.o \
				hal/os/linux/kernel/allocator/default/gc_hal_kernel_allocator_dma.o \
				hal/os/linux/kernel/allocator/default/gc_hal_kernel_allocator_gfp.o \
				hal/os/linux/kernel/allocator/default/gc_hal_kernel_allocator_reserved_mem.o \
				hal/os/linux/kernel/gc_hal_kernel_driver.o \
				hal/os/linux/kernel/platform/sunplus/gc_hal_kernel_platform_sunplus.o \
				hal/os/linux/kernel/allocator/default/gc_hal_kernel_allocator_dmabuf.o \
				hal/os/linux/kernel/gc_hal_kernel_drm.o \
				hal/kernel/gc_hal_kernel.o \
				hal/kernel/gc_hal_kernel_command.o \
				hal/kernel/gc_hal_kernel_db.o \
				hal/kernel/gc_hal_kernel_event.o \
				hal/kernel/gc_hal_kernel_heap.o \
				hal/kernel/gc_hal_kernel_mmu.o \
				hal/kernel/gc_hal_kernel_video_memory.o \
				hal/kernel/gc_hal_kernel_power.o \
				hal/kernel/gc_hal_kernel_security_v1.o \
				hal/kernel/gc_hal_kernel_preemption.o \
				hal/kernel/arch/gc_hal_kernel_context.o \
				hal/kernel/arch/gc_hal_kernel_hardware.o \
				hal/kernel/arch/gc_hal_kernel_hardware_func.o \
				hal/kernel/arch/gc_hal_kernel_hardware_func_flop_reset.o \
				hal/kernel/arch/gc_hal_kernel_hardware_async_fe.o \
				hal/kernel/arch/gc_hal_kernel_hardware_mc_fe.o \
				hal/kernel/arch/gc_hal_kernel_hardware_waitlink_fe.o \
				hal/kernel/arch/gc_hal_kernel_recorder.o \
				hal/os/linux/kernel/gc_hal_kernel_sync.o \
				hal/os/linux/kernel/gc_hal_kernel_security_channel_emulator.o \
				hal/security_v1/gc_hal_ta.o \
				hal/security_v1/gc_hal_ta_hardware.o \
				hal/security_v1/gc_hal_ta_mmu.o \
				hal/security_v1/os/emulator/gc_hal_ta_emulator.o
