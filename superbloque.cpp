
#include "superbloque.h"

SuperBloque::SuperBloque() {
}

SuperBloque::SuperBloque(int s_filesystem_type_, int s_inodes_count_, int s_blocks_count_, int s_free_blocks_count_, int s_free_inodes_count_, time_t s_mtime_,time_t s_umtime_,int s_mnt_count_,char s_magic_[10], int s_inode_size_,int s_block_size_,int s_first_ino_,int s_first_blo_,int s_bm_inode_start_,int s_bm_block_start_,int s_inode_start_,int s_block_start_){
    s_filesystem_type=s_filesystem_type_;
    s_inodes_count=s_inodes_count_;
    s_blocks_count=s_blocks_count_;
    s_free_blocks_count=s_free_blocks_count_;
    s_free_inodes_count=s_free_inodes_count_;
    s_mtime=s_mtime_;
    s_umtime=s_umtime_;
    s_mnt_count=s_mnt_count_;
    for(int i=0;i<10;i++){
    s_magic[i]=s_magic_[i];
    }
    s_inode_size=s_inode_size_;
    s_block_size=s_block_size_;
    s_first_ino=s_first_ino_;
    s_first_blo=s_first_blo_;
    s_bm_inode_start=s_bm_inode_start_;
    s_bm_block_start=s_bm_block_start_;
    s_inode_start=s_inode_start_;
    s_block_start=s_block_start_;
}
SuperBloque::SuperBloque(const SuperBloque& orig) {
}

SuperBloque::~SuperBloque() {
}

