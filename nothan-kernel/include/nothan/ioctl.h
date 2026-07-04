#ifndef _NOTHAN_IOCTL_H
#define _NOTHAN_IOCTL_H

#define _IOC_NRBITS    8
#define _IOC_TYPEBITS  8
#define _IOC_SIZEBITS  14
#define _IOC_DIRBITS   2

#define _IOC_NRSHIFT    0
#define _IOC_TYPESHIFT  (_IOC_NRSHIFT   + _IOC_NRBITS)
#define _IOC_SIZESHIFT  (_IOC_TYPESHIFT + _IOC_TYPEBITS)
#define _IOC_DIRSHIFT   (_IOC_SIZESHIFT + _IOC_SIZEBITS)

#define _IOC_NONE   0U
#define _IOC_WRITE  1U
#define _IOC_READ   2U

#define _IOC(dir, type, nr, size) \
	(((dir)  << _IOC_DIRSHIFT)  | \
	 ((type) << _IOC_TYPESHIFT) | \
	 ((nr)   << _IOC_NRSHIFT)   | \
	 ((size) << _IOC_SIZESHIFT))

#define _IO(type, nr)          _IOC(_IOC_NONE,           (type), (nr), 0)
#define _IOR(type, nr, t)      _IOC(_IOC_READ,           (type), (nr), sizeof(t))
#define _IOW(type, nr, t)      _IOC(_IOC_WRITE,          (type), (nr), sizeof(t))
#define _IOWR(type, nr, t)     _IOC(_IOC_READ|_IOC_WRITE,(type), (nr), sizeof(t))

/* Decode helpers */
#define _IOC_DIR(cmd)   (((cmd) >> _IOC_DIRSHIFT)  & 0x3U)
#define _IOC_TYPE(cmd)  (((cmd) >> _IOC_TYPESHIFT) & 0xFFU)
#define _IOC_NR(cmd)    (((cmd) >> _IOC_NRSHIFT)   & 0xFFU)
#define _IOC_SIZE(cmd)  (((cmd) >> _IOC_SIZESHIFT) & 0x3FFFU)

#endif /* _NOTHAN_IOCTL_H */
