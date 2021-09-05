
#ifndef USUARIO_H
#define USUARIO_H
#include "mount.h"

class Usuario {
public:
    char nombre[10];
    char pass[10];
    Mount *particion;
    char gid[10];
    int uid;
    Usuario();
    Usuario(char nombre_[10],char pass_[10],Mount *particion_,int gid_,int uid_);
    Usuario(const Usuario& orig);
    virtual ~Usuario();
private:

};

#endif /* USUARIO_H */
