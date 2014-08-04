#
# (C) Copyright 2008
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	:= $(obj)libzynq.o

# add by hgf
COBJS-$(CONFIG_FPGA_XILINX) += pcap.o
COBJS-$(CONFIG_FPGA_XILINX) += xdevcfg_sinit.o
COBJS-$(CONFIG_FPGA_XILINX) += xdevcfg_g.o
COBJS-$(CONFIG_FPGA_XILINX) += xdevcfg.o
#COBJS-$(CONFIG_FPGA_XILINX) += xdevcfg_selftest_example.o
#COBJS-$(CONFIG_FPGA_XILINX) += xdevcfg_selftest.o
COBJS-$(CONFIG_FPGA_XILINX) += xil_printf.o
COBJS-y += xil_io.o
COBJS-$(CONFIG_FPGA_XILINX) += xdevcfg_intr.o
COBJS-$(CONFIG_FPGA_XILINX) += xil_assert.o
COBJS-$(CONFIG_FPGA_XILINX) += outbyte.o
COBJS-$(CONFIG_FPGA_XILINX) += xuartps_hw.o
COBJS-y += xil_cache.o
COBJS-y += xusbps_sinit.o
COBJS-y += xusbps_intr.o
COBJS-y += xusbps_hw.o
COBJS-y += xusbps_g.o
COBJS-y += xusbps_endpoint.o
COBJS-y += xusbps.o
COBJS-y += xil_testmem.o
COBJS-y += xscugic_tapp_example.o
COBJS-y += xscugic_sinit.o
COBJS-y += xscugic.o
COBJS-y += xil_exception.o
COBJS-y += xscugic_intr.o
COBJS-y += xscugic_g.o
#COBJS-y += xemacps_example_intr_dma.o
#COBJS-y += xemacps_example_util.o
#COBJS-y += xemacps.o
#COBJS-y += xemacps_bdring.o
#COBJS-y += xemacps_control.o
#COBJS-y += xemacps_intr.o
#COBJS-y += xemacps_sinit.o
#COBJS-y += xemacps_g.o
COBJS-y += xiicps_selftest_example.o
COBJS-y += xiicps.o
COBJS-y += xiicps_g.o
COBJS-y += xiicps_hw.o
COBJS-y += xiicps_intr.o
COBJS-y += xiicps_sinit.o
COBJS-y += xiicps_selftest.o
COBJS-y += xiicps_options.o

COBJS-y += xqspips_selftest_example.o

COBJS-y += xqspips_sinit.o
COBJS-y += xqspips.o
COBJS-y += xqspips_g.o
COBJS-y += xqspips_hw.o
COBJS-y += xqspips_options.o
COBJS-y += xqspips_selftest.o

COBJS-y += xscutimer_polled_example.o

COBJS-y += xscutimer_sinit.o
COBJS-y += xscutimer.o
COBJS-y += xscutimer_g.o
COBJS-y += xscutimer_selftest.o


COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
