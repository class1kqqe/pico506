// Copyright (c) Kuba Szczodrzyński 2025-9-11.

#define FFCONF_DEF 80386

// Function Configurations
#define FF_FS_READONLY	1
#define FF_FS_MINIMIZE	1
#define FF_USE_FIND		0
#define FF_USE_MKFS		0
#define FF_USE_FASTSEEK 0
#define FF_USE_EXPAND	0
#define FF_USE_CHMOD	0
#define FF_USE_LABEL	1
#define FF_USE_FORWARD	0
#define FF_USE_STRFUNC	0
#define FF_PRINT_LLI	0
#define FF_PRINT_FLOAT	0
#define FF_STRF_ENCODE	0

// Locale and Namespace Configurations
#define FF_CODE_PAGE   437
#define FF_USE_LFN	   0
#define FF_MAX_LFN	   255
#define FF_LFN_UNICODE 0
#define FF_LFN_BUF	   255
#define FF_SFN_BUF	   12
#define FF_FS_RPATH	   0
#define FF_PATH_DEPTH  10

// Drive/Volume Configurations
#define FF_VOLUMES		   1
#define FF_STR_VOLUME_ID   0
#define FF_MULTI_PARTITION 0
#define FF_MIN_SS		   512
#define FF_MAX_SS		   512
#define FF_LBA64		   0
#define FF_MIN_GPT		   0x10000000
#define FF_USE_TRIM		   0

// System Configurations
#define FF_FS_TINY		0
#define FF_FS_EXFAT		0
#define FF_FS_NORTC		0
#define FF_NORTC_MON	1
#define FF_NORTC_MDAY	1
#define FF_NORTC_YEAR	2025
#define FF_FS_CRTIME	0
#define FF_FS_NOFSINFO	0
#define FF_FS_LOCK		0
#define FF_FS_REENTRANT 0
#define FF_FS_TIMEOUT	1000
