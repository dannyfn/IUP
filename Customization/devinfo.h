#ifndef _DEVINFO_H_
#define _DEVINFO_H_

#include <stdbool.h>

#define DEVICE_MAGIC        "ANDROID-BOOT!"
#define DEVICE_MAGIC_SIZE   13
#define MAX_PANEL_ID_LEN    64
#define MAX_VERSION_LEN     64

/* Boot mode constants */
#define BOOT_MODE_NORMAL    0x55775577
#define BOOT_MODE_FTM       0x57575757
#define BOOT_MODE_POFFCHG   0x75757575

/* Feature mask bit definitions */
#define  FEATURE_ADD_LED 0x1
#define  FEATURE_ADD_LCD 0x2
#define  FEATURE_NO_BATT 0x4
#define  FEATURE_ADD_ETHERNET 0x8
#define  FEATURE_NO_POWER_KEY 0x10
/* Known board names */
#define BOARD_QC02          "QC02"
#define BOARD_MF900         "MF900"
#define BOARD_QC03          "QC03"
#define BOARD_CP103 "CP103"
#define BOARD_CP105 "CP105"

struct device_info
{
    unsigned char magic[DEVICE_MAGIC_SIZE];
    int           boot_mode;
    unsigned int  feature_mask;
    char          board_name[16];
    char          reserved[16];
    bool          is_unlocked;
    bool          is_tampered;
    bool          is_unlock_critical;
    bool          charger_screen_enabled;
    char          display_panel[MAX_PANEL_ID_LEN];
    char          bootloader_version[MAX_VERSION_LEN];
    char          radio_version[MAX_VERSION_LEN];
    bool          verity_mode;   /* 1 = enforcing, 0 = logging */
};

#endif /* _DEVINFO_H_ */
