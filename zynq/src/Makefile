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

LIB	:= $(obj)libzynq_veriry.o

# add by star-star
COBJS-y += memory_test/xil_testmem.o
COBJS-y += common/xdevcfg.o
COBJS-y += common/xdevcfg_g.o
COBJS-y += common/xdevcfg_intr.o
COBJS-y += common/xdevcfg_sinit.o
COBJS-y += common/pcap.o
# COBJS-y += common/xil_printf.o
COBJS-y += common/xil_io.o
COBJS-y += common/xil_assert.o
COBJS-y += common/outbyte.o
COBJS-y += common/xuartps_hw.o
COBJS-y += common/xil_cache.o
COBJS-y += common/xil_exception.o
COBJS-y += common/xgpio.o
COBJS-y += common/xgpio_extra.o
COBJS-y += common/xgpio_g.o
COBJS-y += common/xgpio_intr.o
COBJS-y += common/xgpio_selftest.o
COBJS-y += common/xgpio_sinit.o

COBJS-y += i2c_test/xiicps.o
COBJS-y += i2c_test/xiicps_g.o
COBJS-y += i2c_test/xiicps_hw.o
COBJS-y += i2c_test/xiicps_intr.o
COBJS-y += i2c_test/xiicps_sinit.o
COBJS-y += i2c_test/xiicps_selftest.o
COBJS-y += i2c_test/xiicps_options.o
COBJS-y += i2c_test/xiicps_selftest_example.o
COBJS-y += i2c_test/xiicps_eeprom_polled_example.o
COBJS-y += i2c_test/xiicps_master.o
COBJS-y += i2c_test/xiicps_polled_master_example.o
COBJS-y += i2c_test/zynq_i2c.o


COBJS-y += scu_timer_test/xscutimer_polled_example.o
COBJS-y += scu_timer_test/xscutimer_sinit.o
COBJS-y += scu_timer_test/xscutimer.o
COBJS-y += scu_timer_test/xscutimer_g.o
COBJS-y += scu_timer_test/xscutimer_selftest.o


COBJS-y += scu_gic_test/xscugic_tapp_example.o
COBJS-y += scu_gic_test/xscugic_sinit.o
COBJS-y += scu_gic_test/xscugic.o
COBJS-y += scu_gic_test/xscugic_intr.o
COBJS-y += scu_gic_test/xscugic_g.o


COBJS-y += usb_test/xusbps_sinit.o
COBJS-y += usb_test/xusbps_intr.o
COBJS-y += usb_test/xusbps_hw.o
COBJS-y += usb_test/xusbps_g.o
COBJS-y += usb_test/xusbps_endpoint.o
COBJS-y += usb_test/xusbps.o
COBJS-y += usb_test/zynq_usb.o

COBJS-y += qspi_test/xqspips_selftest_example.o
COBJS-y += qspi_test/xqspips_sinit.o
COBJS-y += qspi_test/xqspips.o
COBJS-y += qspi_test/xqspips_g.o
COBJS-y += qspi_test/xqspips_hw.o
COBJS-y += qspi_test/xqspips_options.o
COBJS-y += qspi_test/xqspips_selftest.o


#COBJS-y += xemacps_example_intr_dma.o
#COBJS-y += xemacps_example_util.o
#COBJS-y += xemacps.o
#COBJS-y += xemacps_bdring.o
#COBJS-y += xemacps_control.o
#COBJS-y += xemacps_intr.o
#COBJS-y += xemacps_sinit.o
#COBJS-y += xemacps_g.o

COBJS-y += gpio_test/xgpiops.o
COBJS-y += gpio_test/xgpiops_g.o
COBJS-y += gpio_test/xgpiops_hw.o
COBJS-y += gpio_test/xgpiops_intr.o
COBJS-y += gpio_test/xgpiops_sinit.o

COBJS-y += sd_test/ff.o
COBJS-y += sd_test/sd.o
COBJS-y += sd_test/main.o
COBJS-y += sd_test/mmc.o
COBJS-y += sd_test/fsbl_hooks.o
COBJS-y += sd_test/image_mover.o
COBJS-y += sd_test/nor.o
COBJS-y += sd_test/qspi.o
COBJS-y += sd_test/ps7_init.o

COBJS-y += oled_test/zynq_oled.o


COBJS-y += hdmi_test/zynq_hdmi.o

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
