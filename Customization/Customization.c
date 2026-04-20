#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>
#include <iup.h>

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

int FindModelInInnosz(merge_mbn_header_t* header, const char* model, uint32_t* start, uint32_t* len) {
    for (uint32_t i = 0; i < header->count && i < MAX_CUST_MBN_INFO; i++) {
        if (strncmp(header->cust_mbn_info[i].model, model, MAX_MODEL_SIZE) == 0) {
            *start = header->cust_mbn_info[i].cust_mbn_start;
            *len = header->cust_mbn_info[i].cust_mbn_len;
            return 1;
        }
    }
    return 0;
}

void LoadFiles() {
    char cwd[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, cwd);
    
    sprintf(innosz_path, "%s\\innosz.mbn", cwd);
    sprintf(cust_path, "%s\\cust.mbn", cwd);
    
    current_model[0] = '\0';
    
    raw_partition_header_t cust_header;
    if (ReadCustHeader(cust_path, &cust_header)) {
        if (memcmp(cust_header.magic, "INNOSZ", 6) == 0) {
            strncpy(current_model, cust_header.model, MAX_MODEL_SIZE - 1);
            current_model[MAX_MODEL_SIZE - 1] = '\0';
        }
    }
    
    if (current_model[0] == '\0') {
        strcpy(current_model, "default");
    }
    
    IupSetAttribute(txt_model, "VALUE", current_model);
}

int btn_configure_cb(Ihandle* ih) {
    char* new_model = IupGetAttribute(txt_model, "VALUE");
    if (!new_model || new_model[0] == '\0') {
        IupMessage("Error", "Model cannot be empty!");
        return IUP_DEFAULT;
    }
    
    if (strcmp(new_model, current_model) == 0) {
        IupMessage("Info", "Model unchanged, no configuration needed!");
        return IUP_DEFAULT;
    }
    
    merge_mbn_header_t innosz_header;
    if (!ReadInnoszHeader(innosz_path, &innosz_header)) {
        IupMessage("Error", "Firmware version not supported!");
        return IUP_DEFAULT;
    }
    
    uint32_t start, len;
    if (!FindModelInInnosz(&innosz_header, new_model, &start, &len)) {
        IupMessage("Error", "Model not supported!");
        return IUP_DEFAULT;
    }
    
    FILE* innosz_fp = fopen(innosz_path, "rb");
    if (!innosz_fp) {
        IupMessage("Error", "Cannot open innosz.mbn!");
        return IUP_DEFAULT;
    }
    
    fseek(innosz_fp, start, SEEK_SET);
    
    uint8_t* cust_data = (uint8_t*)malloc(len);
    if (!cust_data) {
        fclose(innosz_fp);
        IupMessage("Error", "Memory allocation failed!");
        return IUP_DEFAULT;
    }
    
    size_t read_bytes = fread(cust_data, 1, len, innosz_fp);
    fclose(innosz_fp);
    
    if (read_bytes != len) {
        free(cust_data);
        IupMessage("Error", "Failed to read cust data!");
        return IUP_DEFAULT;
    }
    
    DeleteFileA(cust_path);
    
    FILE* cust_fp = fopen(cust_path, "wb");
    if (!cust_fp) {
        free(cust_data);
        IupMessage("Error", "Cannot create cust.mbn!");
        return IUP_DEFAULT;
    }
    
    fwrite(cust_data, 1, len, cust_fp);
    fclose(cust_fp);
    free(cust_data);
    
    strncpy(current_model, new_model, MAX_MODEL_SIZE - 1);
    current_model[MAX_MODEL_SIZE - 1] = '\0';
    
    IupMessage("Success", "Configuration successful!");
    
    return IUP_DEFAULT;
}

int btn_default_cb(Ihandle* ih) {
    merge_mbn_header_t innosz_header;
    if (!ReadInnoszHeader(innosz_path, &innosz_header)) {
        IupMessage("Error", "Firmware version not supported!");
        return IUP_DEFAULT;
    }
    
    uint32_t start, len;
    if (!FindModelInInnosz(&innosz_header, "default", &start, &len)) {
        IupMessage("Error", "Default model not found!");
        return IUP_DEFAULT;
    }
    
    FILE* innosz_fp = fopen(innosz_path, "rb");
    if (!innosz_fp) {
        IupMessage("Error", "Cannot open innosz.mbn!");
        return IUP_DEFAULT;
    }
    
    fseek(innosz_fp, start, SEEK_SET);
    
    uint8_t* cust_data = (uint8_t*)malloc(len);
    if (!cust_data) {
        fclose(innosz_fp);
        IupMessage("Error", "Memory allocation failed!");
        return IUP_DEFAULT;
    }
    
    size_t read_bytes = fread(cust_data, 1, len, innosz_fp);
    fclose(innosz_fp);
    
    if (read_bytes != len) {
        free(cust_data);
        IupMessage("Error", "Failed to read cust data!");
        return IUP_DEFAULT;
    }
    
    DeleteFileA(cust_path);
    
    FILE* cust_fp = fopen(cust_path, "wb");
    if (!cust_fp) {
        free(cust_data);
        IupMessage("Error", "Cannot create cust.mbn!");
        return IUP_DEFAULT;
    }
    
    fwrite(cust_data, 1, len, cust_fp);
    fclose(cust_fp);
    free(cust_data);
    
    strcpy(current_model, "default");
    IupSetAttribute(txt_model, "VALUE", current_model);
    
    IupMessage("Success", "Reset to default successful!");
    
    return IUP_DEFAULT;
}

int main(int argc, char **argv) {
    IupOpen(&argc, &argv);

    txt_model = IupText(NULL);
    IupSetAttribute(txt_model, "EXPAND", "HORIZONTAL");
    IupSetAttribute(txt_model, "SIZE", "200x");

    Ihandle *lbl_model = IupLabel("Model:");
    IupSetAttribute(lbl_model, "RASTERSIZE", "60x");
    IupSetAttribute(lbl_model, "ALIGNMENT", "ARIGHT:ACENTER");

    Ihandle *btn_configure = IupButton("Configure", NULL);
    IupSetCallback(btn_configure, "ACTION", (Icallback)btn_configure_cb);
    IupSetAttribute(btn_configure, "RASTERSIZE", "100x");

    Ihandle *btn_default = IupButton("Default", NULL);
    IupSetCallback(btn_default, "ACTION", (Icallback)btn_default_cb);
    IupSetAttribute(btn_default, "RASTERSIZE", "100x");

    Ihandle *hbox1 = IupHbox(lbl_model, txt_model, NULL);
    IupSetAttribute(hbox1, "ALIGNMENT", "ACENTER");
    IupSetAttribute(hbox1, "GAP", "5");

    Ihandle *hbox2 = IupHbox(btn_configure, btn_default, NULL);
    IupSetAttribute(hbox2, "ALIGNMENT", "ACENTER");
    IupSetAttribute(hbox2, "GAP", "10");

    Ihandle *vbox = IupVbox(
        hbox1,
        hbox2,
        NULL);
    IupSetAttribute(vbox, "MARGIN", "20x20");
    IupSetAttribute(vbox, "GAP", "15");
    IupSetAttribute(vbox, "ALIGNMENT", "ACENTER");

    Ihandle *dlg = IupDialog(vbox);
    IupSetAttribute(dlg, "TITLE", "Customization Tool");
    IupSetAttribute(dlg, "SIZE", "300x");
    IupSetAttribute(dlg, "MINBOX", "NO");
    IupSetAttribute(dlg, "MAXBOX", "NO");
    IupSetAttribute(dlg, "RESIZE", "NO");

    LoadFiles();

    IupShowXY(dlg, IUP_CENTER, IUP_CENTER);
    IupMainLoop();
    IupClose();

    return EXIT_SUCCESS;
}