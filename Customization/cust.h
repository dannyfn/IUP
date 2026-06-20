#ifndef __CUST_H__
#define __CUST_H__

#define PRODUCT_NAME_MAX_LEN 16
#define RFC_NAME_MAX_LEN 16

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


enum{
    BOARD_QC02,
    BOARD_QC024200,
    BOARD_QC03,
    BOARD_MF900,
    BOARD_CP103,
    BOARD_CP105,
    BOARD_MAX,
};

typedef struct {
    char board_name[PRODUCT_NAME_MAX_LEN];
    char rfc_name[2][RFC_NAME_MAX_LEN];
    unsigned char feature_mask;
} product_config_t;

product_config_t g_mifi_board[BOARD_MAX] = {
    {
        .board_name = "QC02",
        .rfc_name[0] = "rfc113.mbn",
        .rfc_name[1] = "rfc114-B20.mbn",
        .feature_mask = 0x01,
    },    
    {
        .board_name = "QC024200",
        .rfc_name[0] = "rfc113.mbn",
        .rfc_name[1] = "rfc114-B20.mbn",
        .feature_mask = 0x01,
    },
    {
        .board_name = "QC03",
        .rfc_name[0] = "rfc117.mbn",
        .rfc_name[1] = "rfc118-B20.mbn",
        .feature_mask = 0x03,
    },
    {
        .board_name = "MF900",
        .rfc_name[0] = "rfc115.mbn",
        .rfc_name[1] = "rfc116-B20.mbn",
        .feature_mask = 0x01,
    },    
    {
        .board_name = "CP103",
        .rfc_name[0] = "rfc117.mbn",
        .rfc_name[1] = "rfc118-B20.mbn",
        .feature_mask = 0x1D,
    },
    {
        .board_name = "CP105",
        .rfc_name[0] = "rfc115.mbn",
        .rfc_name[1] = "rfc116-B20.mbn",
        .feature_mask = 0x1D,
    },
};

static char g_board_model_map[BOARD_MAX][16][PRODUCT_NAME_MAX_LEN] = {
    [BOARD_QC02]     = { "Default" },
    [BOARD_QC024200] = { "Default" },
    [BOARD_QC03]     = { "Default" },
    [BOARD_MF900]    = { "Default","MF967","MF900A"},
    [BOARD_CP103]    = { "Default","CPF967"},
    [BOARD_CP105]    = { "Default" },
};

#endif /* __CUST_H__ */
