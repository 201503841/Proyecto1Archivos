

#ifndef MOUNT_H
#define MOUNT_H

class Mount {
public:
    char part_type;
    char path[500];
    char name[5];
    char part_status;
    char part_fit;
    int part_start;
    int part_size;
    char part_name[16];
    int letra;
    int numero;
    char id[16];
    Mount *next;
    Mount *prev;
    Mount();
    Mount(char part_type_,char path_[500], char name_[5],char part_status_, char part_fit_,int part_start, int part_size_, char part_name_[16], Mount *next_, Mount *prev_);
    Mount(const Mount& orig);
    virtual ~Mount();
private:

};

#endif /* MOUNT_H */
