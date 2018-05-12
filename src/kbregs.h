/*
pcboot - bootable PC demo/game kernel
Copyright (C) 2018  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY, without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef KBREGS_H_
#define KBREGS_H_

#define KB_IRQ		1
#define PSAUX_IRQ	12

#define KB_DATA_PORT	0x60
#define KB_CMD_PORT		0x64
#define KB_STATUS_PORT	0x64

#define KB_ACK			0xfa
#define KB_NACK			0xfe

#define KB_STAT_OUTBUF_FULL	0x01
#define KB_STAT_INBUF_FULL	0x02
#define KB_STAT_SYSFLAG		0x04
#define KB_STAT_CMD			0x08
#define KB_STAT_ACTIVE		0x10
#define KB_STAT_AUX			0x20
#define KB_STAT_TIMEOUT		0x40
#define KB_STAT_PAR_ERROR	0x80

/* keyboard commands */
#define KB_CMD_GET_CMDBYTE		0x20
#define KB_CMD_SET_CMDBYTE		0x60
#define KB_CMD_AUX_ENABLE		0xa8
#define KB_CMD_READ_OUTPORT		0xd0
#define KB_CMD_WRITE_OUTPORT	0xd1
#define KB_CMD_PSAUX			0xd4
#define KB_CMD_PULSE_RESET		0xfe

/* controller command byte bits */
#define KB_CCB_KB_INTREN		0x01
#define KB_CCB_AUX_INTREN		0x02
#define KB_CCB_SYSFLAG			0x04
#define KB_CCB_KB_DISABLE		0x10
#define KB_CCB_AUX_DISABLE		0x20
#define KB_CCB_KB_XLAT			0x40

/* psaux commands (use with d4 prefix) */
#define AUX_CMD_STREAM_MODE		0xea
#define AUX_CMD_READ_MOUSE		0xeb
#define AUX_CMD_REMOTE_MODE		0xf0
#define AUX_CMD_ENABLE			0xf4
#define AUX_CMD_DEFAULTS		0xf6

#define AUX_PKT0_LEFTBN			0x01
#define AUX_PKT0_RIGHTBN		0x02
#define AUX_PKT0_MIDDLEBN		0x04
#define AUX_PKT0_ALWAYS1		0x08
#define AUX_PKT0_XSIGN			0x10
#define AUX_PKT0_YSIGN			0x20
#define AUX_PKT0_XOVF			0x40
#define AUX_PKT0_YOVF			0x80

#define AUX_PKT0_OVF_BITS		(AUX_PKT0_XOVF | AUX_PKT0_YOVF)
#define AUX_PKT0_BUTTON_BITS \
	(AUX_PKT0_LEFTBN | AUX_PKT0_RIGHTBN | AUX_PKT0_MIDDLEBN)

#endif	/* KBREGS_H_ */
