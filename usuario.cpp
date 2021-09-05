
#include "usuario.h"

Usuario::Usuario() {

}
Usuario::Usuario(char nombre_[10], char pass_[10], Mount* particion_, int gid_, int uid_){
    for(int i=0; i<10;i++){
        nombre[i]=nombre_[i];
        pass[i]=pass_[i];
    }
    particion=particion_;
    uid=uid_;
}

Usuario::Usuario(const Usuario& orig) {
}

Usuario::~Usuario() {
}

