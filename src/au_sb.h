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
#ifndef AU_SB_H_
#define AU_SB_H_

/* returns true (nonzero) if a sound blaster DSP is detected in the ISA bus
 * and sets the internal base_port so that subsequent calls can find it
 */
int sb_detect(void);

/* returns 0 for success, non-zero if the DSP isn't responding at the currently
 * selected base port
 */
int sb_reset_dsp(void);

void *sb_buffer(int *size);

void sb_set_output_rate(int rate);

void sb_start(int rate, int nchan);
void sb_pause(void);
void sb_continue(void);
void sb_stop(void);

void sb_volume(int vol);

#endif	/* AU_SB_H_ */
