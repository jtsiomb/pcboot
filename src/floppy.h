/*
pcboot - bootable PC demo/game kernel
Copyright (C) 2018-2019  John Tsiombikas <nuclear@member.fsf.org>

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
#ifndef FLOPPY_H_
#define FLOPPY_H_

#define FDC_REG_DOUT	0x3f2
#define FDC_REG_STAT	0x3f4
#define FDC_REG_DATA	0x3f5

#define FDC_STAT_ACTA	0x01
#define FDC_STAT_ACTB	0x02
#define FDC_STAT_ACTC	0x04
#define FDC_STAT_ACTD	0x08
#define FDC_STAT_BUSY	0x10
#define FDC_STAT_NODMA	0x20
#define FDC_STAT_IODIR	0x40
#define FDC_STAT_RDY	0x80

#define FDC_DOUT_SEL(x)	(x)
#define FDC_DOUT_RST	0x04
#define FDC_DOUT_DMA	0x08
#define FDC_DOUT_MOTA	0x10
#define FDC_DOUT_MOTB	0x20
#define FDC_DOUT_MOTC	0x40
#define FDC_DOUT_MOTD	0x80

void floppy_motors_off(void);

#endif	/* FLOPPY_H_ */
