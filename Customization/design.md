# IUP GUI工具
## 加载功能
加载当前目录下的一个innosz.mbn文件, 根据如下数据结构,解析到文件头信息.
加载当前目录下的cust.mbn文件, 获取model信息.

### innosz.mbn数据结构
struct {
    char model[16];
    uint32_t cust_mbn_start
    uint32_t cust_mbn_len;
} cust_mbn_info;

struct merge_mbn_header_t {
    uint32_t magic;
    uint32_t count;//实际包含了多少个mbn
    cust_mbn_info[64];
}

### cust.mbn数据结构
typedef struct {
    char magic[6];//"INNOSZ"
    char model[16];
    char version[16];
    file_item_t files[4];
}raw_partition_header_t;

## GUI功能
一个文本输入框, 一个配置按钮
文本框显示当前加载的cust.mbn文件的model信息.
点用户手动修改文本框的model信息后, 点击配置按钮. 如果model不一样, 则从innosz里面解析出对应的cust_mbn对应的数据start和len, 然后删除原来的cust.mbn, 生成新的cust.mbn文件.

如果读取不到cust.mbn的model, 那就当成default即可. default对应的cust.mbn全是0



## 新需求
我现在要把devinfo整合到Customization里面
### GUI
1. 界面新增一个已校准未写号的checkbox, 目的是为了修改devinfo.bin的bootmode为ftm
2. 增加一个国内版和国外版的radio按钮, 二选一. 国内版选中, 对应rfc_name[0], 国外版选中, 对应rfc_name[1]. 默认是国外版

### devinfo.mbn和devinfo_smt.mbn配置

1. 当文本输入框输入一个model的时候, 先根据g_board_model_map查找是否有对应的model名字.
2. 如果有对应的model名字, 则就可以找到对应的board名字, 根据board名字分别对devinfo.mbn和devinfo_smt.mbn进行修改. 
3. 如果没有找到对应的model, 那么把这个当成board名字在g_mifi_board数组里面查找是否有对应的board名字.
4. 如果找到对应的board名字, 做步骤2一样的操作
5. 如果找不到, 提示不支持此型号配置.

当checkbox勾选的时候, 那么需要对devinfo.mbn的boot mode数据进行修改为ftm.

### rawprogram_nand_p2K_b128K_SMT.xml配置
<program PAGES_PER_BLOCK="64" SECTOR_SIZE_IN_BYTES="2048" filename="rfc116-B20.mbn" num_partition_sectors="3328" physical_partition_number="0" start_sector="1280"/>

根据num_partition_sectors="3328"找到xml文件的这一样, 然后把filename改为我们board数据结构里面定义的rfc名字