pcboot
======

pcboot is a minimal 32 bit protected mode kernel, which can be used to make
bootable PC demos or games. It's not designed as a regular standalone operating
system kernel, and cannot be used as such. The intent is to link it with your
codebase, or more likely build your demo/game code around the pcboot kernel
code.

One of the core design principles is to allow starting pcboot programs from
either traditional floppy disks, or USB sticks, and possibly CD-ROMs as well.
The main idea is to make "down to the metal" PC hacking simpler, not requiring
the maintainance of purpose-build retro-PCs. Nor having to deal with the
inadequacies of MS-DOS, but rather follow a more Amiga-like approach of giving
the finger to the ugly software stack of that time, and talk to the hardware,
which is much more timeless and interesting, without middle-men.

A reasonable effort has been made to make porting DOS protected mode programs to
run under pcboot, but don't expect that just re-building your existing code will
work out of the box.

License
-------
Copyright (C) 2018  John Tsiombikas <nuclear@member.fsf.org>
This program is free software. Feel free to use, modify, and/or redistribute it
under the terms of the GNU General Public License version 3, or at your option,
any later version published by the Free Software Foundation. See COPYING for
details.

Yes, this means that if you release any programs which use pcboot, you will have
to release the source code as well, under the same terms.

Let's drop any pretence that you have any good reason to keep your code to
yourself. This is strictly a tool for retro-coding. There are no corporate
secrets in a bootable PC demo. You can do it. If you insist on not sharing the
code of whatever silly thing you make with the help of this, you can fuck right
off.
