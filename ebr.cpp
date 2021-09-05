#include "ebr.h"

EBR::EBR() {
    inicio=-1;
    next=-1;
}

EBR::EBR(char estado_, char fit_, int inicio_,int tamano_,int next_,char lname_[16]) {
    estado=estado_;
    fit=fit_;
    inicio=inicio_;
    tamano=tamano_;
    next=next_;
    int i;
    for(i=0;i<16;i++){
        lname[i]=lname_[i];
    }
}

EBR::EBR(const EBR& orig) {
}

EBR::~EBR() {
}

