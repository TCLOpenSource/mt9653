/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_ERROR_H_
#define _MTK_TV_DRM_ERROR_H_

#ifndef MTK_TV_DRM_ERROR
#define MTK_TV_DRM_ERROR    (‭0xF4240‬)//int 1000000
#endif

#define MTK_TV_DRM_ERROR_CODE(x)    (MTK_TV_DRM_ERROR|x)

#define	MTK_TV_EPERM   MTK_TV_DRM_ERROR_CODE(1)	/* Not super-user */
#define	MTK_TV_ENOENT  MTK_TV_DRM_ERROR_CODE(2)	/* No such file or directory */
#define	MTK_TV_ESRCH   MTK_TV_DRM_ERROR_CODE(3)	/* No such process */
#define	MTK_TV_EINTR   MTK_TV_DRM_ERROR_CODE(4)	/* Interrupted system call */
#define	MTK_TV_EIO     MTK_TV_DRM_ERROR_CODE(5)	/* I/O error */
#define	MTK_TV_ENXIO   MTK_TV_DRM_ERROR_CODE(6)	/* No such device or address */
#define	MTK_TV_E2BIG   MTK_TV_DRM_ERROR_CODE(7)	/* Arg list too long */
#define	MTK_TV_ENOEXEC MTK_TV_DRM_ERROR_CODE(8)	/* Exec format error */
#define	MTK_TV_EBADF   MTK_TV_DRM_ERROR_CODE(9)	/* Bad file number */
#define	MTK_TV_ECHILD  MTK_TV_DRM_ERROR_CODE(10)/* No children */
#define	MTK_TV_EAGAIN  MTK_TV_DRM_ERROR_CODE(11)/* No more processes */
#define	MTK_TV_ENOMEM  MTK_TV_DRM_ERROR_CODE(12)/* Not enough core */
#define	MTK_TV_EACCES  MTK_TV_DRM_ERROR_CODE(13)/* Permission denied */
#define	MTK_TV_EFAULT  MTK_TV_DRM_ERROR_CODE(14)/* Bad address */
#define	MTK_TV_ENOTBLK MTK_TV_DRM_ERROR_CODE(15)/* Block device required */
#define	MTK_TV_EBUSY   MTK_TV_DRM_ERROR_CODE(16)/* Mount device busy */
#define	MTK_TV_EEXIST  MTK_TV_DRM_ERROR_CODE(17)/* File exists */
#define	MTK_TV_EXDEV   MTK_TV_DRM_ERROR_CODE(18)/* Cross-device link */
#define	MTK_TV_ENODEV  MTK_TV_DRM_ERROR_CODE(19)/* No such device */
#define	MTK_TV_ENOTDIR MTK_TV_DRM_ERROR_CODE(20)/* Not a directory */
#define	MTK_TV_EISDIR  MTK_TV_DRM_ERROR_CODE(21)/* Is a directory */
#define	MTK_TV_EINVAL  MTK_TV_DRM_ERROR_CODE(22)/* Invalid argument */
#define	MTK_TV_ENFILE  MTK_TV_DRM_ERROR_CODE(23)/* Too many open files in system */
#define	MTK_TV_EMFILE  MTK_TV_DRM_ERROR_CODE(24)/* Too many open files */
#define	MTK_TV_ENOTTY  MTK_TV_DRM_ERROR_CODE(25)/* Not a typewriter */
#define	MTK_TV_ETXTBSY MTK_TV_DRM_ERROR_CODE(26)/* Text file busy */
#define	MTK_TV_EFBIG   MTK_TV_DRM_ERROR_CODE(27)/* File too large */
#define	MTK_TV_ENOSPC  MTK_TV_DRM_ERROR_CODE(28)/* No space left on device */
#define	MTK_TV_ESPIPE  MTK_TV_DRM_ERROR_CODE(29)/* Illegal seek */
#define	MTK_TV_EROFS   MTK_TV_DRM_ERROR_CODE(30)/* Read only file system */
#define	MTK_TV_EMLINK  MTK_TV_DRM_ERROR_CODE(31)/* Too many links */
#define	MTK_TV_EPIPE   MTK_TV_DRM_ERROR_CODE(32)/* Broken pipe */
#define	MTK_TV_EDOM    MTK_TV_DRM_ERROR_CODE(33)/* Math arg out of domain of func */
#define	MTK_TV_ERANGE  MTK_TV_DRM_ERROR_CODE(34)/* Math result not representable */
#define	MTK_TV_ENOMSG  MTK_TV_DRM_ERROR_CODE(35)/* No message of desired type */
#define	MTK_TV_EIDRM   MTK_TV_DRM_ERROR_CODE(36)/* Identifier removed */
#define	MTK_TV_ECHRNG  MTK_TV_DRM_ERROR_CODE(37)/* Channel number out of range */
#define	MTK_TV_EL2NSYNC MTK_TV_DRM_ERROR_CODE(38)/* Level 2 not synchronized */
#define	MTK_TV_EL3HLT   MTK_TV_DRM_ERROR_CODE(39)/* Level 3 halted */
#define	MTK_TV_EL3RST   MTK_TV_DRM_ERROR_CODE(40)/* Level 3 reset */
#define	MTK_TV_ELNRNG   MTK_TV_DRM_ERROR_CODE(41)/* Link number out of range */
#define	MTK_TV_EUNATCH  MTK_TV_DRM_ERROR_CODE(42)/* Protocol driver not attached */
#define	MTK_TV_ENOCSI   MTK_TV_DRM_ERROR_CODE(43)/* No CSI structure available */
#define	MTK_TV_EL2HLT   MTK_TV_DRM_ERROR_CODE(44)/* Level 2 halted */
#define	MTK_TV_EDEADLK  MTK_TV_DRM_ERROR_CODE(45)/* Deadlock condition */
#define	MTK_TV_ENOLCK   MTK_TV_DRM_ERROR_CODE(46)/* No record locks available */
#define MTK_TV_EBADE    MTK_TV_DRM_ERROR_CODE(50)/* Invalid exchange */
#define MTK_TV_EBADR    MTK_TV_DRM_ERROR_CODE(51)/* Invalid request descriptor */
#define MTK_TV_EXFULL   MTK_TV_DRM_ERROR_CODE(52)/* Exchange full */
#define MTK_TV_ENOANO   MTK_TV_DRM_ERROR_CODE(53)/* No anode */
#define MTK_TV_EBADRQC  MTK_TV_DRM_ERROR_CODE(54)/* Invalid request code */
#define MTK_TV_EBADSLT  MTK_TV_DRM_ERROR_CODE(55)/* Invalid slot */
#define MTK_TV_EDEADLOCK MTK_TV_DRM_ERROR_CODE(56)/* File locking deadlock error */
#define MTK_TV_EBFONT       MTK_TV_DRM_ERROR_CODE(57)/* Bad font file fmt */
#define MTK_TV_ENOSTR       MTK_TV_DRM_ERROR_CODE(60)/* Device not a stream */
#define MTK_TV_ENODATA      MTK_TV_DRM_ERROR_CODE(61)/* No data (for no delay io) */
#define MTK_TV_ETIME        MTK_TV_DRM_ERROR_CODE(62)/* Timer expired */
#define MTK_TV_ENOSR        MTK_TV_DRM_ERROR_CODE(63)/* Out of streams resources */
#define MTK_TV_ENONET       MTK_TV_DRM_ERROR_CODE(64)/* Machine is not on the network */
#define MTK_TV_ENOPKG       MTK_TV_DRM_ERROR_CODE(65)/* Package not installed */
#define MTK_TV_EREMOTE      MTK_TV_DRM_ERROR_CODE(66)/* The object is remote */
#define MTK_TV_ENOLINK      MTK_TV_DRM_ERROR_CODE(67)/* The link has been severed */
#define MTK_TV_EADV         MTK_TV_DRM_ERROR_CODE(68)/* Advertise error */
#define MTK_TV_ESRMNT       MTK_TV_DRM_ERROR_CODE(69)/* Srmount error */
#define	MTK_TV_ECOMM        MTK_TV_DRM_ERROR_CODE(70)/* Communication error on send */
#define MTK_TV_EPROTO       MTK_TV_DRM_ERROR_CODE(71)/* Protocol error */
#define	MTK_TV_EMULTIHOP    MTK_TV_DRM_ERROR_CODE(74)/* Multihop attempted */
#define	MTK_TV_ELBIN        MTK_TV_DRM_ERROR_CODE(75)/* Inode is remote (not really error) */
#define	MTK_TV_EDOTDOT      MTK_TV_DRM_ERROR_CODE(76)/* Cross mount point (not really error) */
#define MTK_TV_EBADMSG      MTK_TV_DRM_ERROR_CODE(77)/* Trying to read unreadable message */
#define MTK_TV_EFTYPE       MTK_TV_DRM_ERROR_CODE(79)/* Inappropriate file type or format */
#define MTK_TV_ENOTUNIQ     MTK_TV_DRM_ERROR_CODE(80)/* Given log. name not unique */
#define MTK_TV_EBADFD       MTK_TV_DRM_ERROR_CODE(81)/* f.d. invalid for this operation */
#define MTK_TV_EREMCHG      MTK_TV_DRM_ERROR_CODE(82)/* Remote address changed */
#define MTK_TV_ELIBACC      MTK_TV_DRM_ERROR_CODE(83)/* Can't access a needed shared lib */
#define MTK_TV_ELIBBAD      MTK_TV_DRM_ERROR_CODE(84)/* Accessing a corrupted shared lib */
#define MTK_TV_ELIBSCN      MTK_TV_DRM_ERROR_CODE(85)/* .lib section in a.out corrupted */
#define MTK_TV_ELIBMAX      MTK_TV_DRM_ERROR_CODE(86)/* Attempting to link in too many libs */
#define MTK_TV_ELIBEXEC     MTK_TV_DRM_ERROR_CODE(87)/* Attempting to exec a shared library */
#define MTK_TV_ENOSYS       MTK_TV_DRM_ERROR_CODE(88)/* Function not implemented */
#define MTK_TV_ENMFILE      MTK_TV_DRM_ERROR_CODE(89)/* No more files */
#define MTK_TV_ENOTEMPTY    MTK_TV_DRM_ERROR_CODE(90)/* Directory not empty */
#define MTK_TV_ENAMETOOLONG MTK_TV_DRM_ERROR_CODE(91)/* File or path name too long */
#define MTK_TV_ELOOP        MTK_TV_DRM_ERROR_CODE(92)/* Too many symbolic links */
#define MTK_TV_EOPNOTSUPP   MTK_TV_DRM_ERROR_CODE(95)
#define MTK_TV_EPFNOSUPPORT MTK_TV_DRM_ERROR_CODE(96)/* Protocol family not supported */
#define MTK_TV_ECONNRESET   MTK_TV_DRM_ERROR_CODE(104)/* Connection reset by peer */
#define MTK_TV_ENOBUFS      MTK_TV_DRM_ERROR_CODE(105)/* No buffer space available */
#define MTK_TV_EAFNOSUPPORT MTK_TV_DRM_ERROR_CODE(106)
#define MTK_TV_EPROTOTYPE   MTK_TV_DRM_ERROR_CODE(107)/* Protocol wrong type for socket */
#define MTK_TV_ENOTSOCK     MTK_TV_DRM_ERROR_CODE(108)/* Socket operation on non-socket */
#define MTK_TV_ENOPROTOOPT  MTK_TV_DRM_ERROR_CODE(109)/* Protocol not available */
#define MTK_TV_ESHUTDOWN    MTK_TV_DRM_ERROR_CODE(110)/* Can't send after socket shutdown */
#define MTK_TV_ECONNREFUSED MTK_TV_DRM_ERROR_CODE(111)/* Connection refused */
#define MTK_TV_EADDRINUSE   MTK_TV_DRM_ERROR_CODE(112)/* Address already in use */
#define MTK_TV_ECONNABORTED MTK_TV_DRM_ERROR_CODE(113)/* Connection aborted */
#define MTK_TV_ENETUNREACH  MTK_TV_DRM_ERROR_CODE(114)/* Network is unreachable */
#define MTK_TV_ENETDOWN     MTK_TV_DRM_ERROR_CODE(115)/* Network interface is not configured */
#define MTK_TV_ETIMEDOUT    MTK_TV_DRM_ERROR_CODE(116)/* Connection timed out */
#define MTK_TV_EHOSTDOWN    MTK_TV_DRM_ERROR_CODE(117)/* Host is down */
#define MTK_TV_EHOSTUNREACH MTK_TV_DRM_ERROR_CODE(118)/* Host is unreachable */
#define MTK_TV_EINPROGRESS  MTK_TV_DRM_ERROR_CODE(119)/* Connection already in progress */
#define MTK_TV_EALREADY     MTK_TV_DRM_ERROR_CODE(120)/* Socket already connected */
#define MTK_TV_EDESTADDRREQ MTK_TV_DRM_ERROR_CODE(121)/* Destination address required */
#define MTK_TV_EMSGSIZE     MTK_TV_DRM_ERROR_CODE(122)/* Message too long */
#define MTK_TV_EPROTONOSUPPORT MTK_TV_DRM_ERROR_CODE(123)/* Unknown protocol */
#define MTK_TV_ESOCKTNOSUPPORT MTK_TV_DRM_ERROR_CODE(124)/* Socket type not supported */
#define MTK_TV_EADDRNOTAVAIL   MTK_TV_DRM_ERROR_CODE(125)/* Address not available */
#define MTK_TV_ENETRESET       MTK_TV_DRM_ERROR_CODE(126)
#define MTK_TV_EISCONN         MTK_TV_DRM_ERROR_CODE(127)/* Socket is already connected */
#define MTK_TV_ENOTCONN        MTK_TV_DRM_ERROR_CODE(128)/* Socket is not connected */
#define MTK_TV_ETOOMANYREFS    MTK_TV_DRM_ERROR_CODE(129)
#define MTK_TV_EPROCLIM        MTK_TV_DRM_ERROR_CODE(130)
#define MTK_TV_EUSERS          MTK_TV_DRM_ERROR_CODE(131)
#define MTK_TV_EDQUOT          MTK_TV_DRM_ERROR_CODE(132)
#define MTK_TV_ESTALE          MTK_TV_DRM_ERROR_CODE(133)
#define MTK_TV_ENOTSUP         MTK_TV_DRM_ERROR_CODE(134)/* Not supported */
#define MTK_TV_ENOMEDIUM       MTK_TV_DRM_ERROR_CODE(135)/* No medium (in tape drive) */
#define MTK_TV_ENOSHARE        MTK_TV_DRM_ERROR_CODE(136)/* No such host or network path */
#define MTK_TV_ECASECLASH      MTK_TV_DRM_ERROR_CODE(137)/* Filename exists with different case */
#define MTK_TV_EILSEQ          MTK_TV_DRM_ERROR_CODE(138)
#define MTK_TV_EOVERFLOW       MTK_TV_DRM_ERROR_CODE(139)/* Value too large for defined data type */


#endif
