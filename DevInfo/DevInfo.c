#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <iup.h>
#include "devinfo.h"


Ihandle *txt_boardname1, *txt_modelname1, *txt_bootmode1, *txt_featuremask1;
Ihandle *txt_boardname2, *txt_modelname2, *txt_bootmode2, *txt_featuremask2;

char file_path1[MAX_PATH] = "";
char file_path2[MAX_PATH] = "";
struct device_info dev_info1;
struct device_info dev_info2;
int num_files_found = 0;
int is_app_unlocked = 0;
Ihandle *btn_unlock_ptr;

void GetBootModeString(int bootMode, char* out) {
    if (bootMode == BOOT_MODE_NORMAL) {
        strcpy(out, "NORMAL");
    } else if (bootMode == BOOT_MODE_FTM) {
        strcpy(out, "FTM");
    } else if (bootMode == BOOT_MODE_POFFCHG) {
        strcpy(out, "POFFCHG");
    } else {
        sprintf(out, "0x%08X", bootMode);
    }
}

void GetFeatureMaskString(unsigned int featureMask, char* out) {
    out[0] = '\0';
    if (featureMask == 0) {
        strcpy(out, "None");
        return;
    }
    int has = 0;
    if (featureMask & FEATURE_ADD_LED) {
        strcat(out, "LED");
        has = 1;
    }
    if (featureMask & FEATURE_ADD_LCD) {
        if (has) strcat(out, ", ");
        strcat(out, "LCD");
        has = 1;
    }
    if (featureMask & FEATURE_NO_BATT) {
        if (has) strcat(out, ", ");
        strcat(out, "NO_BATT");
        has = 1;
    }
    if (featureMask & FEATURE_ADD_ETHERNET) {
        if (has) strcat(out, ", ");
        strcat(out, "ETHERNET");
        has = 1;
    }
    if (featureMask & FEATURE_NO_POWER_KEY) {
        if (has) strcat(out, ", ");
        strcat(out, "NO_POWER_KEY");
        has = 1;
    }
    unsigned int knownBits = FEATURE_ADD_LED | FEATURE_ADD_LCD | FEATURE_NO_BATT | FEATURE_ADD_ETHERNET | FEATURE_NO_POWER_KEY;
    unsigned int unknown = featureMask & ~knownBits;
    if (unknown) {
        if (has) strcat(out, ", ");
        char unk[32];
        sprintf(unk, "Unknown(0x%02X)", unknown);
        strcat(out, unk);
    }
}

int ParseBootMode(const char* str) {
    char lower[128];
    strncpy(lower, str, sizeof(lower) - 1);
    lower[sizeof(lower) - 1] = '\0';
    for(int i = 0; lower[i]; i++) lower[i] = tolower((unsigned char)lower[i]);
    
    if (strstr(lower, "normal") || strstr(lower, "55775577")) return BOOT_MODE_NORMAL;
    if (strstr(lower, "ftm") || strstr(lower, "57575757")) return BOOT_MODE_FTM;
    if (strstr(lower, "poffchg") || strstr(lower, "75757575")) return BOOT_MODE_POFFCHG;
    
    char* hex = strstr(lower, "0x");
    if (hex) return strtoul(hex + 2, NULL, 16);
    return BOOT_MODE_NORMAL;
}

unsigned int ParseFeatureMask(const char* str) {
    char upper[128];
    strncpy(upper, str, sizeof(upper) - 1);
    upper[sizeof(upper) - 1] = '\0';
    for(int i = 0; upper[i]; i++) upper[i] = toupper((unsigned char)upper[i]);
    
    char* hex = strstr(upper, "0X");
    if (hex) return strtoul(hex + 2, NULL, 16);
    
    unsigned int mask = 0;
    if (strstr(upper, "LED")) mask |= FEATURE_ADD_LED;
    if (strstr(upper, "LCD")) mask |= FEATURE_ADD_LCD;
    if (strstr(upper, "NO_BATT")) mask |= FEATURE_NO_BATT;
    if (strstr(upper, "ETHERNET")) mask |= FEATURE_ADD_ETHERNET;
    if (strstr(upper, "NO_POWER_KEY")) mask |= FEATURE_NO_POWER_KEY;
    if (strstr(upper, "NONE")) mask = 0;
    
    if (mask == 0 && !strstr(upper, "NONE")) {
        mask = strtoul(str, NULL, 10);
    }
    return mask;
}

void UpdateUI() {
    
    if (file_path1[0]) {
        IupSetAttribute(txt_modelname1, "VALUE", dev_info1.model_name);
        if (is_app_unlocked) {
            IupSetAttribute(txt_boardname1, "VALUE", dev_info1.board_name);
            char buf[128];
            GetBootModeString(dev_info1.boot_mode, buf);
            IupSetAttribute(txt_bootmode1, "VALUE", buf);
            GetFeatureMaskString(dev_info1.feature_mask, buf);
            IupSetAttribute(txt_featuremask1, "VALUE", buf);
            
            IupSetAttribute(txt_boardname1, "ACTIVE", "YES");
            IupSetAttribute(txt_bootmode1, "ACTIVE", "YES");
            IupSetAttribute(txt_featuremask1, "ACTIVE", "YES");
        } else {
            IupSetAttribute(txt_boardname1, "VALUE", "********");
            IupSetAttribute(txt_bootmode1, "VALUE", "********");
            IupSetAttribute(txt_featuremask1, "VALUE", "********");
            
            IupSetAttribute(txt_boardname1, "ACTIVE", "NO");
            IupSetAttribute(txt_bootmode1, "ACTIVE", "NO");
            IupSetAttribute(txt_featuremask1, "ACTIVE", "NO");
        }
    } else {
        IupSetAttribute(txt_modelname1, "VALUE", "");
        IupSetAttribute(txt_boardname1, "VALUE", "");
        IupSetAttribute(txt_bootmode1, "VALUE", "");
        IupSetAttribute(txt_featuremask1, "VALUE", "");
        IupSetAttribute(txt_boardname1, "ACTIVE", "NO");
        IupSetAttribute(txt_bootmode1, "ACTIVE", "NO");
        IupSetAttribute(txt_featuremask1, "ACTIVE", "NO");
    }
    
    if (file_path2[0]) {
        IupSetAttribute(txt_modelname2, "VALUE", dev_info2.model_name);
        if (is_app_unlocked) {
            IupSetAttribute(txt_boardname2, "VALUE", dev_info2.board_name);
            char buf[128];
            GetBootModeString(dev_info2.boot_mode, buf);
            IupSetAttribute(txt_bootmode2, "VALUE", buf);
            GetFeatureMaskString(dev_info2.feature_mask, buf);
            IupSetAttribute(txt_featuremask2, "VALUE", buf);
            
            IupSetAttribute(txt_boardname2, "ACTIVE", "YES");
            IupSetAttribute(txt_bootmode2, "ACTIVE", "YES");
            IupSetAttribute(txt_featuremask2, "ACTIVE", "YES");
        } else {
            IupSetAttribute(txt_boardname2, "VALUE", "********");
            IupSetAttribute(txt_bootmode2, "VALUE", "********");
            IupSetAttribute(txt_featuremask2, "VALUE", "********");
            
            IupSetAttribute(txt_boardname2, "ACTIVE", "NO");
            IupSetAttribute(txt_bootmode2, "ACTIVE", "NO");
            IupSetAttribute(txt_featuremask2, "ACTIVE", "NO");
        }
    } else {
        IupSetAttribute(txt_modelname2, "VALUE", "");
        IupSetAttribute(txt_boardname2, "VALUE", "");
        IupSetAttribute(txt_bootmode2, "VALUE", "");
        IupSetAttribute(txt_featuremask2, "VALUE", "");
        IupSetAttribute(txt_boardname2, "ACTIVE", "NO");
        IupSetAttribute(txt_bootmode2, "ACTIVE", "NO");
        IupSetAttribute(txt_featuremask2, "ACTIVE", "NO");
    }
}

int ReadDevInfo(const char* path, struct device_info* dev) {
    memset(dev, 0, sizeof(struct device_info));
    FILE* fp = fopen(path, "rb");
    if (!fp) return 0;
    size_t read_bytes = fread(dev, 1, sizeof(struct device_info), fp);
    fclose(fp);
    if (read_bytes < 40) return 0; /* accept older structs, minimum 40 bytes to cover board_name */
    if (memcmp(dev->magic, DEVICE_MAGIC, DEVICE_MAGIC_SIZE) != 0) return 0;
    return 1;
}

int WriteDevInfo(const char* path, struct device_info* dev) {
    if (!path[0]) return 0;
    FILE* fp = fopen(path, "r+b");
    if (!fp) fp = fopen(path, "wb");
    if (!fp) return 0;
    fwrite(dev, sizeof(struct device_info), 1, fp);
    fclose(fp);
    return 1;
}

void ScanDevInfoFiles() {
    num_files_found = 0;
    file_path1[0] = '\0';
    file_path2[0] = '\0';
    
    char cwd[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, cwd);
    
    char temp_path1[MAX_PATH];
    char temp_path2[MAX_PATH];
    sprintf(temp_path1, "%s\\devinfo_smt.bin", cwd);
    sprintf(temp_path2, "%s\\devinfo.bin", cwd);
    
    if (GetFileAttributesA(temp_path1) != INVALID_FILE_ATTRIBUTES) {
        strcpy(file_path1, temp_path1);
        if (ReadDevInfo(file_path1, &dev_info1)) {
            num_files_found++;
        } else {
            file_path1[0] = '\0';
        }
    }
    
    if (GetFileAttributesA(temp_path2) != INVALID_FILE_ATTRIBUTES) {
        strcpy(file_path2, temp_path2);
        if (ReadDevInfo(file_path2, &dev_info2)) {
            num_files_found++;
        } else {
            file_path2[0] = '\0';
        }
    }
}

void LoadConfigFromTxt(const char* path, struct device_info* dev) {
    FILE* fp = fopen(path, "r");
    if (!fp) return;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == ';' || line[0] == '\r' || line[0] == '\n') continue;
        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char* key = line;
        char* val = eq + 1;
        
        while(*key == ' ' || *key == '\t') key++;
        char* end_key = key + strlen(key) - 1;
        while(end_key > key && (*end_key == ' ' || *end_key == '\t')) *end_key-- = '\0';
        
        while(*val == ' ' || *val == '\t') val++;
        char* end_val = val + strlen(val) - 1;
        while(end_val > val && (*end_val == ' ' || *end_val == '\t' || *end_val == '\r' || *end_val == '\n')) *end_val-- = '\0';
        
        if (_stricmp(key, "board_name") == 0) {
            strncpy(dev->board_name, val, sizeof(dev->board_name) - 1);
        } else if (_stricmp(key, "boot_mode") == 0) {
            dev->boot_mode = ParseBootMode(val);
        } else if (_stricmp(key, "feature_mask") == 0) {
            dev->feature_mask = ParseFeatureMask(val);
        }
    }
    fclose(fp);
}

int unlock_ok_cb(Ihandle* ih) {
    Ihandle* txt = (Ihandle*)IupGetAttribute(ih, "PWD_TXT");
    Ihandle* dlg = (Ihandle*)IupGetAttribute(ih, "PWD_DLG");
    char* pwd = IupGetAttribute(txt, "VALUE");
    if (pwd && strcmp(pwd, "7046") == 0) {
        is_app_unlocked = 1;
        UpdateUI();
        IupSetAttribute(btn_unlock_ptr, "ACTIVE", "NO");
        IupHide(dlg);
        return IUP_DEFAULT;
    } else {
        IupMessage("Error", "Incorrect password!");
        return IUP_DEFAULT;
    }
}

int unlock_cancel_cb(Ihandle* ih) {
    Ihandle* dlg = (Ihandle*)IupGetAttribute(ih, "PWD_DLG");
    IupHide(dlg);
    return IUP_DEFAULT;
}

int btn_unlock_cb(Ihandle* ih) {
    if (is_app_unlocked) return IUP_DEFAULT;
    
    Ihandle* txt_pwd = IupText(NULL);
    IupSetAttribute(txt_pwd, "PASSWORD", "YES");
    IupSetAttribute(txt_pwd, "SIZE", "100x");
    
    Ihandle* btn_ok = IupButton("OK", NULL);
    IupSetAttribute(btn_ok, "PWD_TXT", (char*)txt_pwd);
    IupSetCallback(btn_ok, "ACTION", (Icallback)unlock_ok_cb);
    
    Ihandle* btn_cancel = IupButton("Cancel", NULL);
    IupSetCallback(btn_cancel, "ACTION", (Icallback)unlock_cancel_cb);
    
    Ihandle* hbox = IupHbox(btn_ok, btn_cancel, NULL);
    IupSetAttribute(hbox, "GAP", "5");
    IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
    
    Ihandle* vbox = IupVbox(IupLabel("Enter Password:"), txt_pwd, hbox, NULL);
    IupSetAttribute(vbox, "MARGIN", "15x15");
    IupSetAttribute(vbox, "GAP", "10");
    IupSetAttribute(vbox, "ALIGNMENT", "ACENTER");
    
    Ihandle* dlg = IupDialog(vbox);
    IupSetAttribute(dlg, "TITLE", "Unlock");
    IupSetAttribute(dlg, "MINBOX", "NO");
    IupSetAttribute(dlg, "MAXBOX", "NO");
    IupSetAttribute(dlg, "RESIZE", "NO");
    
    IupSetAttribute(btn_ok, "PWD_DLG", (char*)dlg);
    IupSetAttribute(btn_cancel, "PWD_DLG", (char*)dlg);
    
    IupPopup(dlg, IUP_CENTER, IUP_CENTER);
    IupDestroy(dlg);
    return IUP_DEFAULT;
}

void UpdateDevInfoFromUIText() {
    if (file_path1[0]) {
        strncpy(dev_info1.model_name, IupGetAttribute(txt_modelname1, "VALUE"), sizeof(dev_info1.model_name) - 1);
        if (is_app_unlocked) {
            strncpy(dev_info1.board_name, IupGetAttribute(txt_boardname1, "VALUE"), sizeof(dev_info1.board_name) - 1);
            dev_info1.boot_mode = ParseBootMode(IupGetAttribute(txt_bootmode1, "VALUE"));
            dev_info1.feature_mask = ParseFeatureMask(IupGetAttribute(txt_featuremask1, "VALUE"));
        }
    }
    if (file_path2[0]) {
        strncpy(dev_info2.model_name, IupGetAttribute(txt_modelname2, "VALUE"), sizeof(dev_info2.model_name) - 1);
        if (is_app_unlocked) {
            strncpy(dev_info2.board_name, IupGetAttribute(txt_boardname2, "VALUE"), sizeof(dev_info2.board_name) - 1);
            dev_info2.boot_mode = ParseBootMode(IupGetAttribute(txt_bootmode2, "VALUE"));
            dev_info2.feature_mask = ParseFeatureMask(IupGetAttribute(txt_featuremask2, "VALUE"));
        }
    }
}

int btn_write_cb(Ihandle* ih) {
    UpdateDevInfoFromUIText();
    int written = 0;
    if (file_path1[0] && WriteDevInfo(file_path1, &dev_info1)) written++;
    if (file_path2[0] && WriteDevInfo(file_path2, &dev_info2)) written++;
    
    char msg[128];
    if (written > 0) {
        sprintf(msg, "%d file(s) written successfully.", written);
        IupMessage("Success", msg);
    } else {
        IupMessage("Error", "Failed to write files or no files available.");
    }
    return IUP_DEFAULT;
}

Ihandle* MakeFieldRow(const char* label_text, Ihandle* text_ctrl) {
    Ihandle* lbl = IupLabel(label_text);
    IupSetAttribute(lbl, "RASTERSIZE", "80x");
    IupSetAttribute(lbl, "ALIGNMENT", "ARIGHT:ACENTER");
    Ihandle* hbox = IupHbox(lbl, text_ctrl, NULL);
    IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
    IupSetAttribute(hbox, "GAP", "5");
    return hbox;
}

int main(int argc, char **argv) {
    /* Set process code page to UTF-8 so Chinese strings display correctly */
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    IupOpen(&argc, &argv);
    /* Tell IUP to treat all strings as UTF-8 */
    IupSetGlobal("UTF8MODE", "Yes");


    txt_boardname1 = IupText(NULL); IupSetAttribute(txt_boardname1, "EXPAND", "HORIZONTAL");
    txt_modelname1 = IupText(NULL); IupSetAttribute(txt_modelname1, "EXPAND", "HORIZONTAL");
    txt_bootmode1 = IupText(NULL); IupSetAttribute(txt_bootmode1, "EXPAND", "HORIZONTAL");
    txt_featuremask1 = IupText(NULL); IupSetAttribute(txt_featuremask1, "EXPAND", "HORIZONTAL");

    txt_boardname2 = IupText(NULL); IupSetAttribute(txt_boardname2, "EXPAND", "HORIZONTAL");
    txt_modelname2 = IupText(NULL); IupSetAttribute(txt_modelname2, "EXPAND", "HORIZONTAL");
    txt_bootmode2 = IupText(NULL); IupSetAttribute(txt_bootmode2, "EXPAND", "HORIZONTAL");
    txt_featuremask2 = IupText(NULL); IupSetAttribute(txt_featuremask2, "EXPAND", "HORIZONTAL");

    Ihandle *col1 = IupVbox(
        IupLabel("SMT设置"),
        MakeFieldRow("Model Name:", txt_modelname1),
        MakeFieldRow("Board Name:", txt_boardname1),
        MakeFieldRow("Boot Mode:", txt_bootmode1),
        MakeFieldRow("Feature Mask:", txt_featuremask1),
        NULL);
    IupSetAttribute(col1, "MARGIN", "10x10");
    IupSetAttribute(col1, "GAP", "5");

    Ihandle *col2 = IupVbox(
        IupLabel("升级设置"),
        MakeFieldRow("Model Name:", txt_modelname2),
        MakeFieldRow("Board Name:", txt_boardname2),
        MakeFieldRow("Boot Mode:", txt_bootmode2),
        MakeFieldRow("Feature Mask:", txt_featuremask2),
        NULL);
    IupSetAttribute(col2, "MARGIN", "10x10");
    IupSetAttribute(col2, "GAP", "5");

    Ihandle *sep = IupLabel(NULL);
    IupSetAttribute(sep, "SEPARATOR", "VERTICAL");

    Ihandle *files_hbox = IupHbox(col1, sep, col2, NULL);

    btn_unlock_ptr = IupButton("Unlock", NULL); IupSetCallback(btn_unlock_ptr, "ACTION", (Icallback)btn_unlock_cb); IupSetAttribute(btn_unlock_ptr, "RASTERSIZE", "80x");
    Ihandle *btn_write = IupButton("Write", NULL); IupSetCallback(btn_write, "ACTION", (Icallback)btn_write_cb); IupSetAttribute(btn_write, "RASTERSIZE", "80x");

    Ihandle *bottom_hbox = IupHbox(
        btn_unlock_ptr, btn_write,
        NULL);
    IupSetAttribute(bottom_hbox, "ALIGNMENT", "ACENTER");
    IupSetAttribute(bottom_hbox, "MARGIN", "10x10");
    IupSetAttribute(bottom_hbox, "GAP", "10");

    Ihandle *vbox = IupVbox(files_hbox, bottom_hbox, NULL);
    
    Ihandle *dlg = IupDialog(vbox);
    IupSetAttribute(dlg, "TITLE", "MIFI配置工具V1.0");
    IupSetAttribute(dlg, "SIZE", "500x");

    ScanDevInfoFiles();
    UpdateUI();

    IupShowXY(dlg, IUP_CENTER, IUP_CENTER);
    IupMainLoop();
    IupClose();

    return EXIT_SUCCESS;
}
