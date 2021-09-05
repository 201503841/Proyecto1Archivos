
#include <time.h>
#ifndef INODO_H
#define INODO_H

class Inodo {
public:
    int     i_uid;
    char     i_gid[10];
    int     i_size;
    time_t  i_atime;
    time_t  i_ctime;
    time_t  i_mtime;
    int     i_block[15];
    int    i_type;
    int     i_perm;
    Inodo();
    Inodo(int i_uid_, char i_gid_[10],int i_size_, time_t i_atime_,time_t i_ctime_,time_t i_mtime_,int i_type_, int i_perm_);
    Inodo(const Inodo& orig);
    virtual ~Inodo();
private:

};

#endif /* INODO_H */
