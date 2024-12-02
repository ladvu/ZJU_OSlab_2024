#pragma once

#include "defs.h"

#define SFS_MAX_INFO_LEN     (4096 - 3 * 4 - 1)
#define SFS_MAGIC            0x1f2f3f4f
#define SFS_NDIRECT          11
#define SFS_DIRECTORY        1
#define SFS_MAX_FILENAME_LEN 27
#define SFS_BUFFER_SIZE (5)
#define SEEK_CUR 0
#define SEEK_SET 1
#define SEEK_END 2
#define SFS_RECLAIM_THRESHOLD (2) // reclaim threshold

#define SFS_FILE 0
#define SFS_DIRECTORY 1

#define SFS_FLAG_READ (0x1)
#define SFS_FLAG_WRITE (0x2)
#define SFS_BLOCK_SIZE (4096)

struct sfs_super {
    uint32_t magic;
    uint32_t blocks;
    uint32_t unused_blocks;
    char info[SFS_MAX_INFO_LEN + 1];
};



extern struct sfs_meta _meta;
struct sfs_inode {
    uint32_t size;                 // 文件大小
    uint16_t type;                 // 文件类型，文件/目录
    uint16_t links;                // 硬链接数量
    uint32_t blocks;               // 本文件占用的 block 数量
    uint32_t direct[SFS_NDIRECT];  // 直接数据块的索引值
    uint32_t indirect;             // 间接索引块的索引值
};

struct sfs_entry {
    uint32_t ino;                            // 文件的 inode 编号
    char filename[SFS_MAX_FILENAME_LEN + 1]; // 文件名
};

struct sfs_memory_block {
    union {
        struct sfs_inode* din;   // 可能是 inode 块
        char *block;      // 可能是数据块
    } block;
    bool is_inode;        // 是否是 inode
    uint32_t blockno;     // block 编号
    bool dirty;           // 脏位，保证写回数据
    int reclaim_count;    // 指向次数，因为硬链接有可能会打开同一个 inode，所以需要记录次数
    uint32_t reclaim_pid[SFS_RECLAIM_THRESHOLD]; // link list maybe better
};
typedef struct sfs_memory_block mem_block;
typedef mem_block * mem_block_ptr;
typedef mem_block_ptr * buffer_t;
struct sfs_meta{
    uint8_t init;
    uint32_t data_block_start;
};

typedef uint8_t bitmap;
struct sfs_fs {
    struct sfs_meta meta;             // SFS 的元信息
    struct sfs_super super;           // SFS 的超级块
    bitmap *freemap;           // freemap 区域管理，可自行设计
    bool super_dirty;          // 超级块或 freemap 区域是否有修改
    buffer_t buffer;          // buffer 
};
/**
 * 功能: 初始化 simple file system
 * @ret : 成功初始化返回 0，否则返回非 0 值
 */
int sfs_init();


/**
 * 功能: 打开一个文件, 读权限下如果找不到文件，则返回一个小于 0 的值，表示出错，写权限如果没有找到文件，则创建该文件（包括缺失路径）
 * @path : 文件路径 (绝对路径)
 * @flags: 读写权限 (read, write, read | write)
 * @ret  : file descriptor (fd), 每个进程根据 fd 来唯一的定位到其一个打开的文件
 *         正常返回一个大于 0 的 fd 值, 其他情况表示出错
 */
int sfs_open(const char* path, uint32_t flags);


/**
 * 功能: 关闭一个文件，并将其修改过的内容写回磁盘
 * @fd  : 该进程打开的文件的 file descriptor (fd)
 * @ret : 正确关闭返回 0, 其他情况表示出错
 */
int sfs_close(int fd);


/**
 * 功能  : 根据 fromwhere + off 偏移量来移动文件指针(可参考 C 语言的 fseek 函数功能)
 * @fd  : 该进程打开的文件的 file descriptor (fd)
 * @off : 偏移量
 * @fromwhere : SEEK_SET(文件头), SEEK_CUR(当前), SEEK_END(文件尾)
 * @ret : 表示错误码
 *        = 0 正确返回
 *        < 0 出错
 */
int sfs_seek(int fd, int32_t off, int fromwhere);


/**
 * 功能  : 从文件的文件指针开始读取 len 个字节到 buf 数组中 (结合 sfs_seek 函数使用)，并移动对应的文件指针
 * @fd  : 该进程打开的文件的 file descriptor (fd)
 * @buf : 读取内容的缓存区
 * @len : 要读取的字节的数量
 * @ret : 返回实际读取的字节的个数
 *        < 0 表示出错
 *        = 0 表示已经到了文件末尾，没有能读取的了
 *        > 0 表示实际读取的字节的个数，比如 len = 8，但是文件只剩 5 个字节的情况，就是返回 5
 */
int sfs_read(int fd, char* buf, uint32_t len);


/**
 * 功能  : 把 buf 数组的前 len 个字节写入到文件的文件指针位置(覆盖)(结合 sfs_seek 函数使用)，并移动对应的文件指针
 * @fd  : 该进程打开的文件的 file descriptor (fd)
 * @buf : 写入内容的缓存区
 * @len : 要写入的字节的数量
 * @ret : 返回实际的字节的个数
 *        < 0 表示出错
 *        >=0 表示实际写入的字节数量
 */
int sfs_write(int fd, char* buf, uint32_t len);


/**
 * 功能    : 获取 path 下的所有文件名，并存储在 files 数组中
 * @path  : 文件夹路径 (绝对路径)
 * @files : 保存该文件夹下所有的文件名
 * @ret   : > 0 表示该文件夹下有多少文件
 *          = 0 表示该 path 是一个文件
 *          < 0 表示出错
 */
int sfs_get_files(const char* path, char* files[]);

// tool functions

static int min(int a, int b);
static void __strcpy(char * dst, char * src);
static uint32_t __strlen(const char * str);
static int __strcmp(const char *a, const char *b);
static uint32_t hash_fn(uint32_t block_num);
static int hash_insert(mem_block_ptr block);
static int hash_look_up(uint32_t block_num);
static void reset_buffer(uint8_t *buf);
static int next_file_descriptor();
int buffer_update(mem_block_ptr node);
int set_block_dirty(int block_num);
int get_block_from_buffer(uint32_t blockno, struct sfs_memory_block **block);
int next_free_block();

int write_block(uint32_t blockno, bool is_inode, uint8_t *buf);
uint8_t *read_block(uint32_t blockno, bool is_inode);
uint32_t allocate_block_from_idx(struct sfs_inode * inode, int block_idx);
uint32_t block_from_idx(struct sfs_inode * inode, int block_idx);
uint32_t find_in_dir(uint32_t dir_inode, const char *name);
void register_entry(uint32_t dir_inode, char * filename, uint32_t fino);
uint32_t mkdir(uint32_t dir_inode, char *dir_name);
void init_fd(int fd, uint32_t fino, uint32_t dir_inode, uint32_t flags);
uint32_t touch(uint32_t dir_inode, char *filename);
int recycle_block(uint32_t blockno);
void to_buffer(uint8_t *data_block, bool dirty);
int reclaim_block(uint32_t blockno);
uint32_t allocate_block_from_idx(struct sfs_inode * inode, int block_idx);