
#include "inodo.h"

Inodo::Inodo() {
}

Inodo::Inodo(int i_uid_, char i_gid_[10], int i_size_, time_t i_atime_, time_t i_ctime_, time_t i_mtime_, int i_type_, int i_perm_){
    i_uid=i_uid_;
    i_size=i_size_;
    i_atime=i_atime_;
    i_ctime=i_ctime_;
    i_mtime=i_mtime_;
    i_type=i_type_;
    i_perm=i_perm_;
    for(int j=0;j<10;j++){
        i_gid[j]=i_gid_[j];
    }
    for(int i=0; i<15; i++){
        i_block[i]=-1;
    }
}
Inodo::Inodo(const Inodo& orig) {
}

Inodo::~Inodo() {
}
