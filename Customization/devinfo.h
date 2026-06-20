#ifndef _DEVINFO_H_
#define _DEVINFO_H_

#include <stdbool.h>
#include "cust.h"
#define DEVICE_MAGIC        "ANDROID-BOOT!"
#define DEVICE_MAGIC_SIZE   13
#define MAX_PANEL_ID_LEN    64
#define MAX_VERSION_LEN     64



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
