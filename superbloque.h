
#include <time.h>
#ifndef SUPERBLOQUE_H
#define SUPERBLOQUE_H

class SuperBloque {
public:
    int s_filesystem_type;
    int s_inodes_count;
    int s_blocks_count;
    int s_free_blocks_count;
    int s_free_inodes_count;
    time_t s_mtime;
    time_t s_umtime;
    int s_mnt_count;
    char s_magic[10];
    int s_inode_size;
    int s_block_size;
    int s_first_ino;
    int s_first_blo;
    int s_bm_inode_start;
    int s_bm_block_start;
    int s_inode_start;
    int s_block_start;
    int s_journal_start;
    SuperBloque();
    SuperBloque(int s_filesystem_type_, int s_inodes_count_, int s_blocks_count_, int s_free_blocks_count_, int s_free_inodes_count_, time_t s_mtime,time_t s_umtime,int s_mnt_count_,char s_magic_[10], int s_inode_size_,int s_block_size_,int s_first_ino_,int s_first_blo_,int s_bm_inode_start_,int s_bm_block_start_,int s_inode_start_,int s_block_start_);
    SuperBloque(const SuperBloque& orig);
    virtual ~SuperBloque();
private:

};

#endif /* SUPERBLOQUE_H */
