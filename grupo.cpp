#include "grupo.h"

Grupo::Grupo() {
}

Grupo::Grupo(int gid_, char nombre_[10]){
    gid=gid_;
    for(int i=0;i<10;i++){
        nombre[i]=nombre_[i];
    }
}

Grupo::Grupo(const Grupo& orig) {
}

Grupo::~Grupo() {
}

