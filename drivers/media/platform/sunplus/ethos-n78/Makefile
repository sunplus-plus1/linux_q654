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
ccflags-y += -Wall -Werror $(EXTRA_CCFLAGS)

obj-$(CONFIG_ETHOS_N78_NPU) += ethosn.o

ethosn-objs := ethosn_core.o \
             ethosn_backport.o \
             ethosn_buffer.o \
             ethosn_debug.o \
             ethosn_device.o \
             ethosn_dma.o \
             ethosn_dma_carveout.o \
             ethosn_dma_iommu.o \
             ethosn_driver.o \
             ethosn_network.o \
             ethosn_smc.o
