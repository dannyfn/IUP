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
