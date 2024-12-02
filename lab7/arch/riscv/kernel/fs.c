#include "fs.h"
#include "buf.h"
#include "defs.h"
#include "slub.h"
#include "task_manager.h"
#include "virtio.h"
#include "vm.h"
#include "mm.h"

// --------------------------------------------------
// ----------- read and write interface -------------
struct sfs_fs* __sfs;

void disk_op(int blockno, uint8_t *data, bool write) {
    struct buf b;
    b.disk = 0;
    b.blockno = blockno;
    b.data = (uint8_t *)PHYSICAL_ADDR(data);
    virtio_disk_rw((struct buf *)(PHYSICAL_ADDR(&b)), write);
}

#define disk_read(blockno, data) disk_op((blockno), (data), 0)
#define disk_write(blockno, data) disk_op((blockno), (data), 1)

// --------------------------------------------------
// ----------------- Tool Functions -----------------
// --------------------------------------------------

static void __strcpy(char * dst, char * src){
    while (*src) {
        *dst = *src;
        dst++;
        src++;
    }
    *dst = '\0';
}
static uint32_t __strlen(const char * str){
    uint32_t len = 0;
    while (*str) {
        len++;
        str++;
    }
    return len;
}
static int __strcmp(const char *a, const char *b) {
  while (*a && *b) {
    if (*a < *b)
      return -1;
    if (*a > *b)
      return 1;
    a++;
    b++;
  }
  if (*a && !*b)
    return 1;
  if (*b && !*a)
    return -1;
  return 0;
}
static uint32_t hash_fn(uint32_t block_num) {
    return block_num % SFS_BUFFER_SIZE;
}
static int hash_insert(mem_block_ptr block) {
    uint32_t block_num = block->blockno;
    int idx = hash_fn(block_num);
    if (__sfs->buffer[idx] == NULL)  { __sfs->buffer[idx] = block; return idx; }
    if (__sfs->buffer[idx]->blockno == block_num) { return idx; }
    int start = idx;
    idx = (idx + 1) % SFS_BUFFER_SIZE;
    // find a victim
    while (__sfs->buffer[idx] != NULL && start != idx) {
        if (__sfs->buffer[idx]->blockno == block_num) {
            if (__sfs->buffer[idx]->block.block != block->block.block) { printf("error: block not match\n"); }
            return idx;
        }
        else if (__sfs->buffer[idx]->reclaim_count == 0) {
            // dirty -> write back
            if (__sfs->buffer[idx]->dirty) { disk_write(__sfs->buffer[idx]->blockno, (uint8_t * )__sfs->buffer[idx]->block.block); }
            kfree(__sfs->buffer[idx]->block.block);
            kfree(__sfs->buffer[idx]);
            __sfs->buffer[idx] = block;
            return idx;
        }
        idx = (idx + 1) % SFS_BUFFER_SIZE;
    }
    if (__sfs->buffer[idx] == NULL) {
        __sfs->buffer[idx] = block;
        return idx;
    }
    // no victim -> use idx
    // printf("warning: buffer full\n");
    if (__sfs->buffer[idx]->dirty) { disk_write(__sfs->buffer[idx]->blockno, (uint8_t * )__sfs->buffer[idx]->block.block); }
    kfree(__sfs->buffer[idx]->block.block);
    kfree(__sfs->buffer[idx]);
    __sfs->buffer[idx] = block;
    return idx;
}
static int hash_look_up(uint32_t block_num){
    int idx = hash_fn(block_num);
    int start = idx;
    if (__sfs->buffer[idx] == NULL) return -1;
    if (__sfs->buffer[idx]->blockno == block_num) return idx;
    idx = (idx + 1) % SFS_BUFFER_SIZE;
    while (__sfs->buffer[idx] != NULL && start != idx) {
        if (__sfs->buffer[idx]->blockno == block_num) return idx;
        idx = (idx + 1) % SFS_BUFFER_SIZE;
    }
    return -1;
}
static int min(int a, int b){
    return a < b ? a : b;
}
static int max(int a, int b){
    return a > b ? a : b;
}
static void reset_buffer(uint8_t *buf){
    for (int i = 0; i < SFS_BLOCK_SIZE; i++) buf[i] = 0;
}
static int next_file_descriptor(){
    for (int i = 0; i < 16; i++) {
        if (current->fs.fds[i] == NULL) return i;
    }
    return -1;
}

// --------------------------------------------------
// ----------------- Other Functions ----------------
// --------------------------------------------------

int buffer_update(mem_block_ptr node) {
    // stupid implementation
    // do deep copy of the node
    mem_block_ptr copy = (mem_block_ptr)kmalloc(sizeof(mem_block));
    memcpy(copy, node, sizeof(mem_block));
    if (node->is_inode) {
        copy->block.din = (struct sfs_inode *)kmalloc(sizeof(struct sfs_inode));
        memcpy(copy->block.din, node->block.din, sizeof(struct sfs_inode));
    }
    else {
        copy->block.block = (char *)kmalloc(sizeof(char) * SFS_BLOCK_SIZE);
        memcpy(copy->block.block, node->block.block, SFS_BLOCK_SIZE);
    }
    int idx = hash_look_up(node->blockno); 
    // if not in buffer
    if (idx == -1){
        return hash_insert(copy);
    }
    // if in buffer
    else {
        kfree(__sfs->buffer[idx]->block.block); 
        kfree(__sfs->buffer[idx]);
        __sfs->buffer[idx] = copy;
        return idx;
    }
}
int set_block_dirty(int block_num){
    mem_block_ptr ptr;
    if (get_block_from_buffer(block_num, &ptr)) {
        ptr->dirty = 1;
        return 1;
    }
    return 0;
}
int get_block_from_buffer(uint32_t blockno, struct sfs_memory_block **block) {
    int idx = hash_look_up(blockno);
    if (idx == -1) return 0;
    *block = __sfs->buffer[idx];
    return 1;
}
int next_free_block(){
    for (int i = 0; i < __sfs->super.blocks; i++) {
        if (!(__sfs->freemap[i / 8] & (1 << (i % 8)))) {
            __sfs->freemap[i / 8] |= (1 << (i % 8));
            __sfs->super.unused_blocks--;
            __sfs->super_dirty = 1;
            return i;
        }
    }
    return 0;
}
uint8_t *read_block(uint32_t blockno, bool is_inode){
    // try to get from buffer
    mem_block_ptr ptr;
    if (get_block_from_buffer(blockno, &ptr)) {
        return ptr->block.block;
    }
    // read from disk
    else { 
        uint8_t *buf = (uint8_t *)kmalloc(sizeof(uint8_t) * SFS_BLOCK_SIZE);
        disk_read(blockno, buf); 
        // insert into buffer
        mem_block_ptr node = (mem_block_ptr)kmalloc(sizeof(mem_block));
        node->blockno = blockno;
        node->is_inode = is_inode;
        node->dirty = 0;
        node->reclaim_count = 0;
        node->block.block = buf;
        hash_insert(node);
        return buf;
    } 
}
int write_block(uint32_t blockno, bool is_inode, uint8_t *buf){
    // try to get from buffer
    // buf should be from buffer
    mem_block_ptr ptr;
    if (get_block_from_buffer(blockno, &ptr)) {
        if (ptr->block.block != buf){
            printf("error, block addr = %x, buf addr = %x\n", ptr->block.block, buf);
            while(1);
        }
        ptr->dirty = 1;
        return 0; // indicate a write hit, no need to write to disk and buf can't be freed by caller
    }
    // write to disk
    else {
        // buf is not persistent
        // so do copy here
        mem_block_ptr node = (mem_block_ptr)kmalloc(sizeof(mem_block));
        node->blockno = blockno;
        node->is_inode = is_inode;
        node->dirty = 1;
        node->reclaim_count = 0;
        node->block.block = (uint8_t *)kmalloc(sizeof(uint8_t) * SFS_BLOCK_SIZE);
        memcpy(node->block.block, buf, SFS_BLOCK_SIZE);
        hash_insert(node);
        return 1; // indicate a write miss and buf can be freed by caller
    }
}
uint32_t allocate_block_from_idx(struct sfs_inode * inode, int block_idx){
    if (block_idx < SFS_NDIRECT) {
        uint32_t blockno = next_free_block();
        inode->direct[block_idx] = blockno;
        inode->blocks++;
        return blockno;
    }
    else {
        uint8_t *buf;
        if (inode->indirect == 0) {
            inode->indirect = next_free_block();
            buf = read_block(inode->indirect, 0);
            reset_buffer(buf);
            write_block(inode->indirect, 0, buf);
        }
        else { buf = read_block(inode->indirect, 0); }
        uint32_t *indirect_block = (uint32_t *)buf;
        uint32_t blockno = next_free_block();
        indirect_block[block_idx - SFS_NDIRECT] = blockno;
        write_block(inode->indirect, 0, buf);
        inode->blocks++;
        return blockno;
    }
}
uint32_t block_from_idx(struct sfs_inode * inode, int block_idx){
    if (block_idx < SFS_NDIRECT) {
        return inode->direct[block_idx];
    }
    else {
        if (inode->indirect == 0) {
            printf("error: indirect block not found\n");
            return 0;
        }
        uint8_t *buf = read_block(inode->indirect, 0);
        uint32_t *indirect_block = (uint32_t *)buf;
        uint32_t ino = indirect_block[block_idx - SFS_NDIRECT];
        write_block(inode->indirect, 0, buf);
        return ino;
    }
}
uint32_t find_in_dir(uint32_t dir_inode,const char * name){
    // get dir inode
    uint8_t *__dir = read_block(dir_inode, 1);
    struct sfs_inode * din = (struct sfs_inode *)__dir;
    // search in dir
    uint8_t *buf;
    uint32_t num_entries = din->size / 32;
    int num_entries_each = SFS_BLOCK_SIZE / sizeof(struct sfs_entry);
    for (int i = 0; i < din->blocks; i++) {
        uint32_t block_number = block_from_idx(din, i);
        buf = read_block(block_number, 1);
        struct sfs_entry *entry = (struct sfs_entry *)buf;
        if (i == din->blocks - 1) num_entries_each = num_entries - i * num_entries_each;
        for (int j = 0; j < num_entries_each; j++) {
            if (__strcmp(entry[j].filename, name) == 0) {
                uint32_t ino = entry[j].ino;
                return ino;
            }
        }
    }
    return 0;
}
void register_entry(uint32_t dir_inode, char * filename, uint32_t fino){
    // check if dir exists
    uint32_t ino = find_in_dir(dir_inode, filename);
    if (ino != 0) {
        printf("dir exists\n");
        return ;
    }
    // get dir inode
    uint8_t *__dir = read_block(dir_inode, 1);
    struct sfs_inode * din = (struct sfs_inode *)__dir;
    // update dir
    uint8_t *buf = (uint8_t *)kmalloc(sizeof(uint8_t) * SFS_BLOCK_SIZE);
    int block_limit = (SFS_BLOCK_SIZE / sizeof(struct sfs_entry)) * SFS_NDIRECT;
    // 1. within direct blocks
    if (din->size < block_limit) {
        int num_entries = din->size / 32;  
        int num_entries_each = SFS_BLOCK_SIZE / sizeof(struct sfs_entry);
        // 1.1 need a new block
        if ((num_entries + 1) % num_entries_each == 0) {
            din->direct[din->blocks] = next_free_block();
            buf = read_block(din->direct[din->blocks], 0);
            struct sfs_entry *entry = (struct sfs_entry *)buf;
            entry[0].ino = fino;
            __strcpy(entry[0].filename, filename);
            write_block(din->direct[din->blocks], 0, buf);
            din->blocks++;
        }
        // 1.2 within a block 
        else {
            buf = read_block(din->direct[din->blocks - 1], 0);
            struct sfs_entry *entry = (struct sfs_entry *)buf;
            int idx = num_entries % num_entries_each;
            entry[idx].ino = fino;
            __strcpy(entry[idx].filename, filename);
            write_block(din->direct[din->blocks - 1], 0, buf);
        }
        
    }
    // 2. need indirect block
    else if (din->size == block_limit) {
        din->indirect = next_free_block();
        buf = read_block(din->indirect, 0);
        reset_buffer(buf);
        uint32_t *indirect_block = (uint32_t *)buf; 
        uint32_t next_block = next_free_block();
        indirect_block[0] = next_block;
        write_block(din->indirect, 0, buf);
        // write new block
        buf = read_block(next_block, 0);
        reset_buffer(buf);
        struct sfs_entry *entry = (struct sfs_entry *)buf;
        entry[0].ino = fino;
        __strcpy(entry[0].filename, filename);
        write_block(next_block, 0, buf);
        din->blocks++;
    }
    // 3. within indirect block
    else {
        int num_entries = din->size / 32;  
        int num_entries_each = SFS_BLOCK_SIZE / sizeof(struct sfs_entry);
        // 3.1 new a new block
        if ((num_entries + 1) % num_entries_each == 0) {
            buf = read_block(din->indirect, 0);
            uint32_t *indirect_block = (uint32_t *)buf;
            uint32_t next_block = next_free_block();
            indirect_block[din->blocks - SFS_NDIRECT] = next_block;
            write_block(din->indirect, 0, buf);   

            buf = read_block(next_block, 0);
            struct sfs_entry *entry = (struct sfs_entry *)buf;
            entry[0].ino = fino;
            __strcpy(entry[0].filename, filename);
            write_block(next_block, 0, buf);
            din->blocks++;
        }
        // 3.2 within a block
        else {
            buf = read_block(din->indirect, 0);
            uint32_t *indirect_block = (uint32_t *)buf;
            uint32_t next_block = indirect_block[din->blocks - SFS_NDIRECT - 1];
            read_block(next_block, buf);
            buf = read_block(next_block, 0);

            struct sfs_entry *entry = (struct sfs_entry *)buf;
            int idx = num_entries % num_entries_each;
            entry[idx].ino = fino;
            __strcpy(entry[idx].filename, filename);
            write_block(next_block,0, buf); 
        }
    }
    din->size += 32;
    write_block(dir_inode, 1, __dir);
}
uint32_t mkdir(uint32_t dir_inode,char * dir_name){
    uint32_t new_dir_ino = next_free_block();
    register_entry(dir_inode, dir_name, new_dir_ino);
    struct sfs_inode* new_dir_inode = (struct sfs_inode *)read_block(new_dir_ino, 1);
    new_dir_inode->size = 64;
    new_dir_inode->type = SFS_DIRECTORY;
    new_dir_inode->links = 1;
    new_dir_inode->blocks = 1;
    new_dir_inode->direct[0] = next_free_block();
    new_dir_inode->indirect = 0;
    uint8_t * buf = read_block(new_dir_inode->direct[0], 0);
    reset_buffer(buf);
    struct sfs_entry *entry = (struct sfs_entry *) buf;
    entry[0].ino = new_dir_ino;
    __strcpy(entry[0].filename, ".");
    entry[1].ino = dir_inode;
    __strcpy(entry[1].filename, "..");
    write_block(new_dir_inode->direct[0], 0, buf);
    write_block(new_dir_ino, 1, (uint8_t *)new_dir_inode);
    return new_dir_ino;
}
void init_fd(int fd, uint32_t fino, uint32_t dir_inode, uint32_t flags){
    if (current->fs.fds[fd] == NULL) {
        current->fs.fds[fd] = (struct file *)kmalloc(sizeof(struct file));
    }
    current->fs.fds[fd]->flags = flags;
    current->fs.fds[fd]->off = 0;
    current->fs.fds[fd]->inode = (struct sfs_inode * )read_block(fino, 1);
    current->fs.fds[fd]->path = (struct sfs_inode *)read_block(dir_inode, 1);
    current->fs.fds[fd]->inode_blockno = fino;
    current->fs.fds[fd]->path_blockno = dir_inode;
    reclaim_block(fino);
    reclaim_block(dir_inode);
    // printf("fino = %d, dir_inode = %d\n", fino, dir_inode);
    // printf("inode = %x, path = %x\n", current->fs.fds[fd]->inode, current->fs.fds[fd]->path);
}
uint32_t touch(uint32_t dir_inode, char * filename){
    uint32_t fino = next_free_block();
    register_entry(dir_inode, filename, fino);
    struct sfs_inode* file_inode = (struct sfs_inode *)read_block(fino, 1);
    file_inode->size = 0;
    file_inode->type = SFS_FILE;
    file_inode->links = 1;
    file_inode->blocks = 0;
    file_inode->indirect = 0;
    write_block(fino, 1, (uint8_t *)file_inode);
    return fino;
}
int recycle_block(uint32_t blockno){
    // disclaime the block
    mem_block_ptr ptr;
    int in_buffer = get_block_from_buffer(blockno, &ptr);
    uint32_t pid = current->pid;
    if (in_buffer){
        if (ptr->reclaim_count) 
        {
            int k;
            for (k = 0; k < ptr->reclaim_count && ptr->reclaim_pid[k] != pid; k++);
            if (k == ptr->reclaim_count) { printf("recycle failed. reclaim pid not found\n"); return 0; }
            // left shift
            for (int i = k; i < ptr->reclaim_count - 1; i++) ptr->reclaim_pid[i] = ptr->reclaim_pid[i + 1];
            ptr->reclaim_count--;
        }
    }
    return in_buffer;
}
int reclaim_block(uint32_t blockno){
    mem_block_ptr ptr;
    if (get_block_from_buffer(blockno, &ptr)) {
        if (ptr->reclaim_count == SFS_RECLAIM_THRESHOLD) {
            printf("reclaim failed. reclaim count exceed threshold\n");
            return -1;
        }
        // check if already in reclaim list
        int i;
        for (i = 0; i < ptr->reclaim_count && ptr->reclaim_pid[i] != current->pid; i++);
        if (i == ptr->reclaim_count)  ptr->reclaim_pid[ptr->reclaim_count++] = current->pid;
        return 1;
    }
    return 0;
}

// --------------------------------------------
// -------------- SFS functions ---------------
// --------------------------------------------

int sfs_init(){
    if (__sfs != NULL && __sfs->meta.init) return 0;
    uint8_t *buf = (uint8_t *)kmalloc(sizeof(uint8_t) * SFS_BLOCK_SIZE);
    if (__sfs == NULL) {
        __sfs = (struct sfs_fs *)kmalloc(sizeof(struct sfs_fs));
    }
    disk_read(0, buf);
    // init super
    memcpy(&__sfs->super,buf,sizeof(struct sfs_super));
    kfree(buf);
    if (__sfs->super.magic != SFS_MAGIC) {
        printf("sfs: invalid magic\n");
        return 1;
    }
    __sfs->super_dirty = 0;
    // init buffer
    __sfs->buffer = (buffer_t) kmalloc(sizeof(mem_block_ptr) * SFS_BUFFER_SIZE);
    for (int i = 1; i < SFS_BUFFER_SIZE; i++) __sfs->buffer[i] = NULL;  
    // init freemap
    int bytes = __sfs->super.blocks / 8;
    int num_blocks = bytes / SFS_BLOCK_SIZE;
    if (num_blocks % SFS_BLOCK_SIZE != 0 || num_blocks == 0) num_blocks++;
    __sfs->freemap = (bitmap *)kmalloc(sizeof(bitmap) * num_blocks * SFS_BLOCK_SIZE);
    for (int i = 0; i < num_blocks; i++) { disk_read(2 + i, (uint8_t *)__sfs->freemap + i * SFS_BLOCK_SIZE); }
    // init meta
    __sfs->meta.data_block_start = 2 + __sfs->super.blocks;
    __sfs->meta.init = 1;
    return 0;
};

int sfs_open(const char *path, uint32_t flags) {
    sfs_init();
    // 1. search 
    // 1.1 hash search
    // 1.2 non hash search
    char * ptr = path;
    char * kname = (char *)kmalloc(sizeof(char) * SFS_MAX_FILENAME_LEN + 1);
    char * kptr = kname;
    uint32_t prev_inode = 0; 
    uint32_t next_inode = 0; 
    while (*ptr) {
        if (*ptr == '/'){
            *kptr = 0;
            if (next_inode == 0) next_inode = 1; 
            else {
                next_inode = find_in_dir(prev_inode, kname);
                if (next_inode == 0) {
                    if (flags & SFS_FLAG_WRITE) next_inode = mkdir(prev_inode, kname);
                    else {
                        printf("file not found\n");
                        return -1;
                    }
                }
            }
            prev_inode = next_inode;
            kptr = kname;
        }
        else {
            *kptr = *ptr;
            kptr++;
        }
        ptr++;
    }
    *kptr = 0;
    if (__strlen(kname) == 0) {printf("Invalid path\n"); return -1;}
    uint32_t fino;
    fino = find_in_dir(next_inode,kname);
    // check is file
    uint8_t * buf;
    if (!fino && (flags & SFS_FLAG_WRITE)) fino = touch(next_inode, kname);
    else if (fino){
        buf = read_block(fino, 1);
        struct sfs_inode * inode = (struct sfs_inode *)buf;
        if (inode->type == SFS_DIRECTORY) {
            kfree(kname);
            printf("%s Is a Directory\n", path);
            return -1;
        }
    }
    else {
        kfree(kname);
        printf("file not found\n");
        return -1;
    }

    int fd = next_file_descriptor();
    if (fd == -1) {printf("too many files opened\n"); return -1;}
    init_fd(fd, fino, next_inode, flags);
    kfree(kname);
    return fd;
}

int sfs_close(int fd){
    sfs_init();
    if (current->fs.fds[fd] == NULL) return -1;
    // recycle all data blocks 
    struct sfs_inode * inode = current->fs.fds[fd]->inode;
    for (int i = 0; i < inode->blocks; i++) {
        recycle_block(block_from_idx(inode, i));
    }
    
    // free inode and path    
    recycle_block(current->fs.fds[fd]->inode_blockno);
    recycle_block(current->fs.fds[fd]->path_blockno);
    kfree(current->fs.fds[fd]->inode);
    current->fs.fds[fd]->inode = NULL;
    kfree(current->fs.fds[fd]->path);
    current->fs.fds[fd]->path = NULL;
    kfree(current->fs.fds[fd]);
    current->fs.fds[fd] = NULL;
    return 0;
}

int sfs_seek(int fd, int32_t off, int fromwhere){
    sfs_init();
    int32_t cur_off = current->fs.fds[fd]->off;
    if (fromwhere == SEEK_SET) cur_off = off;
    else if (fromwhere == SEEK_CUR) cur_off += off;
    else if (fromwhere == SEEK_END) cur_off = current->fs.fds[fd]->inode->size + off;
    // check within file size
    if (cur_off < 0 || cur_off > current->fs.fds[fd]->inode->size) return -1;
    current->fs.fds[fd]->off = cur_off;
    return 0;
}

int sfs_read(int fd, char *buf, uint32_t len){
    sfs_init();
    uint32_t bytes_to_read = min(len, current->fs.fds[fd]->inode->size - current->fs.fds[fd]->off); 
    if (bytes_to_read == 0) return 0;
    uint32_t cur_off = current->fs.fds[fd]->off;
    uint32_t end_offset = cur_off + bytes_to_read - 1;
    if (cur_off < 0 || cur_off >= current->fs.fds[fd]->inode->size || \
        end_offset < 0 || end_offset >= current->fs.fds[fd]->inode->size) return -1;
    uint32_t start_block = cur_off / SFS_BLOCK_SIZE;
    uint32_t end_block = end_offset / SFS_BLOCK_SIZE;
    uint32_t start_off = cur_off % SFS_BLOCK_SIZE;
    uint32_t end_off = end_offset % SFS_BLOCK_SIZE;
    uint8_t *block_buf = NULL;
    uint32_t cur_block = start_block;
    uint32_t cur_len = 0;
    while (cur_block <= end_block) {
        uint32_t blockno = block_from_idx(current->fs.fds[fd]->inode, cur_block);
        block_buf = read_block(blockno, 0);
        reclaim_block(blockno);
        if (cur_block == start_block && cur_block == end_block) {
            memcpy(buf, block_buf + start_off, bytes_to_read);
            cur_len += bytes_to_read;
        }
        else if (cur_block == start_block) {
            memcpy(buf, block_buf + start_off, SFS_BLOCK_SIZE - start_off);
            cur_len += SFS_BLOCK_SIZE - start_off;
        }
        else if (cur_block == end_block) {
            memcpy(buf + cur_len, block_buf, end_off + 1);
            cur_len += end_off + 1;
        }
        else {
            memcpy(buf + cur_len, block_buf, SFS_BLOCK_SIZE);
            cur_len += SFS_BLOCK_SIZE;
        }
        recycle_block(blockno);
        cur_block++;
    }
    current->fs.fds[fd]->off += cur_len;
    return cur_len;
}

int sfs_write(int fd, char *buf, uint32_t len){
    sfs_init();
    uint32_t cur_off = current->fs.fds[fd]->off;
    uint32_t end_offset = cur_off + len - 1;
    if (cur_off < 0 || cur_off > current->fs.fds[fd]->inode->size) return -1;
    uint32_t start_block = cur_off / SFS_BLOCK_SIZE;
    uint32_t end_block = end_offset / SFS_BLOCK_SIZE;
    uint32_t start_off = cur_off % SFS_BLOCK_SIZE;
    uint32_t end_off = end_offset % SFS_BLOCK_SIZE;
    uint8_t *block_buf = NULL;
    uint32_t cur_block = start_block;
    uint32_t cur_len = 0;
    current->fs.fds[fd]->inode->size = max(current->fs.fds[fd]->inode->size, end_offset + 1);
    while (cur_block <= end_block) {
        uint32_t blockno;
        if (cur_block < current->fs.fds[fd]->inode->blocks) { blockno = block_from_idx(current->fs.fds[fd]->inode, cur_block);}
        else { blockno = allocate_block_from_idx(current->fs.fds[fd]->inode, cur_block); }
        block_buf = read_block(blockno, 0);
        reclaim_block(blockno);
        if (cur_block == start_block && cur_block == end_block) {
            memcpy(block_buf + start_off, buf, len);
            write_block(blockno, 0, block_buf);
            cur_len += len;
        }
        else if (cur_block == start_block) {
            memcpy(block_buf + start_off, buf, SFS_BLOCK_SIZE - start_off);
            write_block(blockno, 0, block_buf);
            cur_len += SFS_BLOCK_SIZE - start_off;
        }
        else if (cur_block == end_block) {
            memcpy(block_buf, buf + cur_len, end_off + 1);
            write_block(blockno, 0, block_buf);
            cur_len += end_off + 1;
        }
        recycle_block(blockno);
        cur_block++;
    }
    current->fs.fds[fd]->off += cur_len;
    write_block(current->fs.fds[fd]->inode_blockno, 1, (uint8_t *)current->fs.fds[fd]->inode);
    if (cur_len != len) {
        printf("write error\n");
    }
    return cur_len;
}

int sfs_get_files(const char* path, char* files[]){
    sfs_init();
    if (__strlen(path) == 0) {
        printf("Invalid path\n");
        return 0;
    }
    char * ptr = path;
    char * kname = (char *)kmalloc(sizeof(char) * SFS_MAX_FILENAME_LEN + 1);
    char * kptr = kname;
    uint32_t __inode = 0; 
    while (*ptr) {
        if (*ptr == '/'){
            *kptr = 0;
            if (__inode == 0) __inode = 1; 
            else {
                __inode = find_in_dir(__inode, kname);
                if (__inode == 0) {
                    printf("No such file or dir\n");
                    return -1;
                }
            }
            kptr = kname;
        }
        else {
            *kptr = *ptr;
            kptr++;
        }
        ptr++;
    }
    *kptr = 0;
    if (__strlen(kname) != 0) {
        __inode = find_in_dir(__inode, kname);
        if (__inode == 0) {
            printf("No such file or dir\n");
            return -1;
        }
    }
    // get dir inode
    uint8_t *__dir = read_block(__inode, 1);
    struct sfs_inode * din = (struct sfs_inode *)__dir;
    if (din->type != SFS_DIRECTORY) {
        return 0;
    }
    // search in dir
    uint8_t *buf;
    uint32_t num_entries = din->size / 32;
    int cnt = 0;
    int num_entries_each = SFS_BLOCK_SIZE / sizeof(struct sfs_entry);
    for (int i = 0; i < din->blocks; i++) {
        uint32_t block_number = block_from_idx(din, i);
        buf = read_block(block_number, 0);
        struct sfs_entry *entry = (struct sfs_entry *)buf;
        if (i == din->blocks - 1) num_entries_each = num_entries - i * num_entries_each;
        for (int j = 0; j < num_entries_each; j++) {
            __strcpy(files[cnt], entry[j].filename);
            cnt++;
        }
    }
    kfree(kname);
    return cnt;
}
