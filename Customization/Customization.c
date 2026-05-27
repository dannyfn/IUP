#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>
#include <iup.h>
#include "devinfo.h"
#undef BOARD_QC02
#undef BOARD_MF900
#undef BOARD_QC03
#undef BOARD_CP103
#undef BOARD_CP105
#include "cust.h"

#ifndef VERSION_TAG
#define VERSION_TAG "V1.0.2"
#endif
#define MAX_MODEL_SIZE 16
#define MAX_FILE_ITEM 4
#define MAX_CUST_MBN_INFO 64

typedef struct {
    char model[MAX_MODEL_SIZE];
    uint32_t cust_mbn_start;
    uint32_t cust_mbn_len;
} cust_mbn_info_t;

typedef struct {
    uint32_t magic;
    uint32_t count;
    cust_mbn_info_t cust_mbn_info[MAX_CUST_MBN_INFO];
} merge_mbn_header_t;

typedef struct {
    char magic[6];
    char model[MAX_MODEL_SIZE];
    char version[MAX_MODEL_SIZE];
    uint8_t files[MAX_FILE_ITEM * 128];
} raw_partition_header_t;

Ihandle *txt_model;
Ihandle *chk_calibrated;
Ihandle *tgl_domestic;
Ihandle *tgl_overseas;
Ihandle *rad_region;
char current_model[MAX_MODEL_SIZE] = "";
char innosz_path[MAX_PATH] = "";
char cust_path[MAX_PATH] = "";

int ReadInnoszHeader(const char* path, merge_mbn_header_t* header) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return 0;
    size_t read_bytes = fread(header, 1, sizeof(merge_mbn_header_t), fp);
    fclose(fp);
    return (read_bytes >= sizeof(uint32_t) * 2);
}

int ReadCustHeader(const char* path, raw_partition_header_t* header) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return 0;
    size_t read_bytes = fread(header, 1, sizeof(raw_partition_header_t), fp);
    fclose(fp);
    return (read_bytes >= sizeof(header->magic) + sizeof(header->model));
}

int ReadDevInfoBoardName(const char* path, char* board_name, size_t board_name_size) {
    struct device_info dev;
    FILE* fp = fopen(path, "rb");
    if (!fp) return 0;

    size_t read_bytes = fread(&dev, 1, sizeof(struct device_info), fp);
    fclose(fp);
    if (read_bytes < 40) return 0;

    strncpy(board_name, dev.board_name, board_name_size - 1);
    board_name[board_name_size - 1] = '\0';
    return board_name[0] != '\0';
}

void InitNewDevInfo(struct device_info* dev) {
    memset(dev, 0, sizeof(*dev));
    memcpy(dev->magic, DEVICE_MAGIC, DEVICE_MAGIC_SIZE);
    dev->verity_mode = true;
}

int FindModelInInnosz(merge_mbn_header_t* header, const char* model, uint32_t* start, uint32_t* len) {
    for (uint32_t i = 0; i < header->count && i < MAX_CUST_MBN_INFO; i++) {
        if (_strnicmp(header->cust_mbn_info[i].model, model, MAX_MODEL_SIZE) == 0) {
            *start = header->cust_mbn_info[i].cust_mbn_start;
            *len = header->cust_mbn_info[i].cust_mbn_len;
            return 1;
        }
    }
    return 0;
}

int FindCustPayload(merge_mbn_header_t* header, const char* model, int board_idx, uint32_t* start, uint32_t* len) {
    int typed_board_name = (_stricmp(g_mifi_board[board_idx].board_name, model) == 0);

    if (FindModelInInnosz(header, model, start, len)) {
        return 1;
    }

    if (typed_board_name) {
        for (int m = 0; m < 16; m++) {
            const char* alias = g_board_model_map[board_idx][m];
            if (alias[0] == '\0') {
                break;
            }
            if (_stricmp(alias, "Default") == 0 || _stricmp(alias, model) == 0) {
                continue;
            }
            return FindModelInInnosz(header, "Default", start, len);
        }
    }

    if (!typed_board_name &&
        FindModelInInnosz(header, g_mifi_board[board_idx].board_name, start, len)) {
        return 1;
    }

    for (int m = 0; m < 16; m++) {
        const char* alias = g_board_model_map[board_idx][m];
        if (alias[0] == '\0') {
            break;
        }
        if (_stricmp(alias, "Default") == 0 || _stricmp(alias, model) == 0) {
            continue;
        }
        if (FindModelInInnosz(header, alias, start, len)) {
            return 1;
        }
    }

    return FindModelInInnosz(header, "Default", start, len);
}

/* Forward declaration */
int FindBoardIndex(const char* model, int* out_index);

/* Read the current rfc filename from the XML (the entry with num_partition_sectors="3328") */
int ReadXmlRfcName(const char* xml_path, char* rfc_name, size_t rfc_name_size) {
    FILE* fp = fopen(xml_path, "r");
    if (!fp) return 0;

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "num_partition_sectors=\"3328\"")) {
            char* p1 = strstr(line, "filename=\"");
            if (p1) {
                p1 += 10;
                char* p2 = strchr(p1, '"');
                if (p2 && (size_t)(p2 - p1) < rfc_name_size) {
                    strncpy(rfc_name, p1, p2 - p1);
                    rfc_name[p2 - p1] = '\0';
                    fclose(fp);
                    return 1;
                }
            }
        }
    }
    fclose(fp);
    return 0;
}

void LoadFiles() {
    char cwd[MAX_PATH];
    char devinfo_path[MAX_PATH];
    char board_name[MAX_MODEL_SIZE] = "";
    char cust_model[MAX_MODEL_SIZE] = "";

    GetCurrentDirectoryA(MAX_PATH, cwd);
    
    sprintf(innosz_path, "%s\\innosz.mbn", cwd);
    sprintf(cust_path, "%s\\cust.mbn", cwd);
    sprintf(devinfo_path, "%s\\devinfo.bin", cwd);
    
    current_model[0] = '\0';
    ReadDevInfoBoardName(devinfo_path, board_name, sizeof(board_name));
    
    raw_partition_header_t cust_header;
    if (ReadCustHeader(cust_path, &cust_header)) {
        if (memcmp(cust_header.magic, "INNOSZ", 6) == 0) {
            strncpy(cust_model, cust_header.model, MAX_MODEL_SIZE - 1);
            cust_model[MAX_MODEL_SIZE - 1] = '\0';
        }
    }
    
    if (cust_model[0] != '\0' && _stricmp(cust_model, "Default") != 0) {
        strncpy(current_model, cust_model, MAX_MODEL_SIZE - 1);
        current_model[MAX_MODEL_SIZE - 1] = '\0';
    } else if (board_name[0] != '\0') {
        strncpy(current_model, board_name, MAX_MODEL_SIZE - 1);
        current_model[MAX_MODEL_SIZE - 1] = '\0';
    } else if (cust_model[0] != '\0') {
        strncpy(current_model, cust_model, MAX_MODEL_SIZE - 1);
        current_model[MAX_MODEL_SIZE - 1] = '\0';
    } else {
        strcpy(current_model, "Default");
    }
    
    IupSetAttribute(txt_model, "VALUE", current_model);

    /* Restore calibration checkbox from devinfo boot_mode */
    {
        struct device_info dev;
        FILE* fp = fopen(devinfo_path, "rb");
        if (fp) {
            size_t read_bytes = fread(&dev, 1, sizeof(struct device_info), fp);
            fclose(fp);
            if (read_bytes >= 40) {
                if (dev.boot_mode == BOOT_MODE_FTM) {
                    IupSetAttribute(chk_calibrated, "VALUE", "ON");
                } else {
                    IupSetAttribute(chk_calibrated, "VALUE", "OFF");
                }
            }
        }
    }

    /* Restore domestic/overseas radio from XML rfc filename */
    {
        int board_idx = -1;
        int found_board = FindBoardIndex(current_model, &board_idx);
        
        if (found_board) {
            char xml_path[MAX_PATH];
            char xml_rfc[RFC_NAME_MAX_LEN] = "";
            sprintf(xml_path, "%s\\rawprogram_nand_p2K_b128K_SMT.xml", cwd);
            int read_xml = ReadXmlRfcName(xml_path, xml_rfc, sizeof(xml_rfc));
            
            if (read_xml && xml_rfc[0] != '\0') {
                if (_stricmp(xml_rfc, g_mifi_board[board_idx].rfc_name[0]) == 0) {
                    /* Domestic */
                    IupSetAttribute(tgl_domestic, "VALUE", "ON");
                } else {
                    /* Overseas (default) */
                    IupSetAttribute(tgl_overseas, "VALUE", "ON");
                }
            } else {
                IupSetAttribute(tgl_overseas, "VALUE", "ON");
            }
        } else {
            IupSetAttribute(tgl_overseas, "VALUE", "ON");
        }
    }
}

int FindBoardIndex(const char* model, int* out_index) {
    int matched_index = -1;

    for (int b = 0; b < BOARD_MAX; b++) {
        for (int m = 0; m < 16; m++) {
            if (g_board_model_map[b][m][0] == '\0') break;
            if (_stricmp(g_board_model_map[b][m], model) == 0) {
                if (matched_index != -1 && matched_index != b) {
                    return 0;
                }
                matched_index = b;
            }
        }
    }

    if (matched_index != -1) {
        *out_index = matched_index;
        return 1;
    }

    for (int b = 0; b < BOARD_MAX; b++) {
        if (_stricmp(g_mifi_board[b].board_name, model) == 0) {
            *out_index = b;
            return 1;
        }
    }
    return 0;
}

int ReplaceFileWithBuffer(const char* filepath, const void* data, size_t len) {
    char temp_path[MAX_PATH];
    FILE* fp;

    sprintf(temp_path, "%s.tmp", filepath);
    fp = fopen(temp_path, "wb");
    if (!fp) {
        return 0;
    }

    if (fwrite(data, 1, len, fp) != len) {
        fclose(fp);
        DeleteFileA(temp_path);
        return 0;
    }

    if (fclose(fp) != 0) {
        DeleteFileA(temp_path);
        return 0;
    }

    if (!MoveFileExA(temp_path, filepath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
        DeleteFileA(temp_path);
        return 0;
    }

    return 1;
}

int UpdateDevInfo(const char* filepath, const char* model, const char* board_name, unsigned int feature_mask, int boot_mode) {
    struct device_info dev;
    DWORD attrs = GetFileAttributesA(filepath);

    if (attrs == INVALID_FILE_ATTRIBUTES) {
        DWORD err = GetLastError();
        if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND) {
            return 0;
        }
        InitNewDevInfo(&dev);
    } else {
        FILE* fp = fopen(filepath, "rb");
        if (!fp) return 0;
        size_t read_bytes = fread(&dev, 1, sizeof(struct device_info), fp);
        fclose(fp);
        if (read_bytes < 40) return 0;
    }
    
    strncpy(dev.reserved, model, sizeof(dev.reserved) - 1);
    dev.reserved[sizeof(dev.reserved) - 1] = '\0';
    
    strncpy(dev.board_name, board_name, sizeof(dev.board_name) - 1);
    dev.board_name[sizeof(dev.board_name) - 1] = '\0';
    
    dev.feature_mask = feature_mask;
    
    dev.boot_mode = boot_mode;
    
    return ReplaceFileWithBuffer(filepath, &dev, sizeof(struct device_info));
}

int UpdateXml(const char* filepath, const char* rfc_name) {
    FILE* fp = fopen(filepath, "r");
    if (!fp) return 0;
    
    char temp_path[MAX_PATH];
    sprintf(temp_path, "%s.tmp", filepath);
    FILE* fp_out = fopen(temp_path, "w");
    if (!fp_out) {
        fclose(fp);
        return 0;
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "num_partition_sectors=\"3328\"")) {
            char* p1 = strstr(line, "filename=\"");
            if (p1) {
                p1 += 10;
                char* p2 = strchr(p1, '"');
                if (p2) {
                    fwrite(line, 1, p1 - line, fp_out);
                    fprintf(fp_out, "%s", rfc_name);
                    fprintf(fp_out, "%s", p2);
                    continue;
                }
            }
        }
        fprintf(fp_out, "%s", line);
    }
    fclose(fp);
    fclose(fp_out);
    
    if (!MoveFileExA(temp_path, filepath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
        DeleteFileA(temp_path);
        return 0;
    }
    return 1;
}

int btn_configure_cb(Ihandle* ih) {
    char* new_model = IupGetAttribute(txt_model, "VALUE");
    if (!new_model || new_model[0] == '\0') {
        IupMessage("Error", "型号名不能为空!");
        return IUP_DEFAULT;
    }

    if (_stricmp(new_model, "Default") == 0) {
        IupMessage("Error", "Default仅用于恢复默认, 请填写明确的型号或板型名称!");
        return IUP_DEFAULT;
    }
    
    int board_idx = -1;
    if (!FindBoardIndex(new_model, &board_idx)) {
        IupMessage("Error", "不支持此型号配置!");
        return IUP_DEFAULT;
    }
    
    merge_mbn_header_t innosz_header;
    if (!ReadInnoszHeader(innosz_path, &innosz_header)) {
        IupMessage("Error", "错误码0x801,请联系作者!");
        return IUP_DEFAULT;
    }
    
    uint32_t start, len;
    if (!FindCustPayload(&innosz_header, new_model, board_idx, &start, &len)) {
        IupMessage("Error", "固件不支持该型号!");
        return IUP_DEFAULT;
    }
    
    FILE* innosz_fp = fopen(innosz_path, "rb");
    if (!innosz_fp) {
        IupMessage("Error", "错误码0x802,请联系作者!");
        return IUP_DEFAULT;
    }
    
    fseek(innosz_fp, start, SEEK_SET);
    
    uint8_t* cust_data = (uint8_t*)malloc(len);
    if (!cust_data) {
        fclose(innosz_fp);
        IupMessage("Error", "错误码0x803,请联系作者!");
        return IUP_DEFAULT;
    }
    
    size_t read_bytes = fread(cust_data, 1, len, innosz_fp);
    fclose(innosz_fp);
    
    if (read_bytes != len) {
        free(cust_data);
        IupMessage("Error", "错误码0x804,请联系作者!");
        return IUP_DEFAULT;
    }
    
    if (!ReplaceFileWithBuffer(cust_path, cust_data, len)) {
        free(cust_data);
        IupMessage("Error", "错误码0x805,请联系作者!");
        return IUP_DEFAULT;
    }
    free(cust_data);
    
    int is_domestic = IupGetInt(tgl_domestic, "VALUE");
    const char* rfc_name = g_mifi_board[board_idx].rfc_name[is_domestic ? 0 : 1];
    int is_ftm = IupGetInt(chk_calibrated, "VALUE");
    
    char cwd[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, cwd);
    char devinfo_path[MAX_PATH];
    char devinfo_smt_path[MAX_PATH];
    char xml_path[MAX_PATH];
    
    sprintf(devinfo_path, "%s\\devinfo.bin", cwd);
    sprintf(devinfo_smt_path, "%s\\devinfo_smt.bin", cwd);
    sprintf(xml_path, "%s\\rawprogram_nand_p2K_b128K_SMT.xml", cwd);
    
    if (!UpdateDevInfo(devinfo_path, new_model, g_mifi_board[board_idx].board_name,
                       g_mifi_board[board_idx].feature_mask,
                       is_ftm ? BOOT_MODE_FTM : BOOT_MODE_NORMAL) ||
        !UpdateDevInfo(devinfo_smt_path, new_model, g_mifi_board[board_idx].board_name,
                       g_mifi_board[board_idx].feature_mask,
                       BOOT_MODE_FTM) ||
        !UpdateXml(xml_path, rfc_name)) {
        IupMessage("Error", "版本配置失败, 请检查相关文件是否存在且可写!");
        return IUP_DEFAULT;
    }
    
    strncpy(current_model, new_model, MAX_MODEL_SIZE - 1);
    current_model[MAX_MODEL_SIZE - 1] = '\0';
    
    IupMessage("Success", "版本配置成功!");
    
    return IUP_DEFAULT;
}


int main(int argc, char **argv) {

    /* Set process code page to UTF-8 so Chinese strings display correctly */
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    IupOpen(&argc, &argv);
    /* Tell IUP to treat all strings as UTF-8 */
    IupSetGlobal("UTF8MODE", "Yes");   
    /* Increase font size globally */
    IupSetGlobal("DEFAULTFONTSIZE", "12");

    txt_model = IupText(NULL);
    IupSetAttribute(txt_model, "EXPAND", "HORIZONTAL");
    IupSetAttribute(txt_model, "VISIBLECOLUMNS", "15");

    Ihandle *lbl_model = IupLabel("主板或型号名:");
    IupSetAttribute(lbl_model, "ALIGNMENT", "ARIGHT:ACENTER");

    chk_calibrated = IupToggle("已校准未写号", NULL);
    IupSetAttribute(chk_calibrated, "VALUE", "OFF");

    tgl_domestic = IupToggle("国内版", NULL);
    tgl_overseas = IupToggle("国外版", NULL);
    
    rad_region = IupRadio(IupHbox(tgl_domestic, tgl_overseas, NULL));
    IupSetAttribute(rad_region, "GAP", "5");

    Ihandle *btn_configure = IupButton("配置", NULL);
    IupSetCallback(btn_configure, "ACTION", (Icallback)btn_configure_cb);
    IupSetAttribute(btn_configure, "PADDING", "30x5");

    Ihandle *hbox1 = IupHbox(lbl_model, txt_model, NULL);
    IupSetAttribute(hbox1, "ALIGNMENT", "ACENTER");
    IupSetAttribute(hbox1, "GAP", "5");

    Ihandle *hbox_radio = IupHbox(rad_region, NULL);
    IupSetAttribute(hbox_radio, "ALIGNMENT", "ACENTER");

    Ihandle *hbox_chk = IupHbox(chk_calibrated, NULL);
    IupSetAttribute(hbox_chk, "ALIGNMENT", "ACENTER");

    Ihandle *hbox2 = IupHbox(btn_configure, NULL);
    IupSetAttribute(hbox2, "ALIGNMENT", "ACENTER");

    Ihandle *vbox = IupVbox(
        hbox1,
        hbox_radio,
        hbox_chk,
        hbox2,
        NULL);
    IupSetAttribute(vbox, "MARGIN", "20x20");
    IupSetAttribute(vbox, "GAP", "2");
    IupSetAttribute(vbox, "ALIGNMENT", "ACENTER");

    Ihandle *dlg = IupDialog(vbox);
    IupSetfAttribute(dlg, "TITLE", "版本配置工具%s", VERSION_TAG);
    IupSetAttribute(dlg, "MINBOX", "NO");
    IupSetAttribute(dlg, "MAXBOX", "NO");
    IupSetAttribute(dlg, "RESIZE", "NO");

    LoadFiles();

    IupShowXY(dlg, IUP_CENTER, IUP_CENTER);
    IupMainLoop();
    IupClose();

    return EXIT_SUCCESS;
}
