#ifndef PTYPE_H_
#define PTYPE_H_


#define PTYPE_EXT		0x5
#define PTYPE_EXT_LBA	0xf


#define PTYPES_SIZE		(sizeof partypes / sizeof *partypes)

struct {
	int type;
	const char *name;
} partypes[] = {
	{0, "empty"},
	{0x01, "fat12"},
	{0x02, "xenix root"},
	{0x03, "xenix usr"},
	{0x04, "fat16 (small)"},
	{0x05, "extended"},
	{0x06, "fat16"},
	{0x07, "hpfs/ntfs"},
	{0x08, "aix"},
	{0x09, "aix bootable"},
	{0x0a, "os/2 boot manager"},
	{0x0b, "fat32 (chs)"},
	{0x0c, "fat32 (lba)"},
	{0x0e, "fat16 (lba)"},
	{0x0f, "extended (lba)"},
	{0x11, "hidden fat12"},
	{0x12, "compaq diagnostics"},
	{0x14, "hidden fat16 (small)"},
	{0x16, "hidden fat16"},
	{0x17, "hidden hpfs/ntfs"},
	{0x1b, "hidden fat32"},
	{0x1c, "hidden fat32 (lba)"},
	{0x1d, "hidden fat16 (lba)"},
	{0x24, "nec dos"},
	{0x27, "windows recovery"},
	{0x39, "plan 9"},
	{0x3c, "partition magic"},
	{0x4d, "qnx"},
	{0x4e, "qnx 2nd"},
	{0x4f, "qnx 3rd"},
	{0x52, "cp/m"},
	{0x63, "hurd/sysv"},
	{0x64, "netware 286"},
	{0x65, "netware 386"},
	{0x80, "minix (old)"},
	{0x81, "minix"},
	{0x82, "linux swap/solaris"},
	{0x83, "linux"},
	{0x84, "windows suspend"},
	{0x85, "linux extended"},
	{0x86, "ntfs volume?"},
	{0x87, "ntfs volume?"},
	{0x88, "linux plaintext"},
	{0x8e, "linux lvm"},
	{0x9f, "bsd/os"},
	{0xa0, "laptop diagnostic"},
	{0xa5, "freebsd slice"},
	{0xa6, "openbsd slice"},
	{0xa7, "nextstep"},
	{0xa8, "darwin ufs"},
	{0xa9, "netbsd slice"},
	{0xab, "darwin boot"},
	{0xaf, "hfs/hfs+"},
	{0xb7, "bsdi"},
	{0xb8, "bsdi swap"},
	{0xbe, "solaris boot"},
	{0xbf, "solaris"},
	{0xde, "dell diagnostic"},
	{0xeb, "beos"},
	{0xee, "gpt"},
	{0xef, "efi (fat)"},
	{0xf0, "linux/pa-risc boot"},
	{0xf2, "dos secondary"},
	{0xfb, "vmware vmfs"},
	{0xfc, "vmware vmkcore"},
	{0xfd, "linux raid auto"}
};

#endif	/* PTYPE_H_ */
