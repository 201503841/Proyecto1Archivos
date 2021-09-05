/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   Parti.cpp
 * Author: heidy
 *
 * Created on 16 de febrero de 2020, 19:29
 */

#include "parti.h"

Parti::Parti() {
    estado='I';
    inicio=-1;
    tamano=0;
}

Parti::Parti(char estado_,char tipo_ ,char fit_, int inicio_, int tamano_, char pname_[16]) {

    estado=estado_;
    tipo=tipo_;
    fit=fit_;
    inicio=inicio_;
    tamano=tamano_;
    int i;
    for(i=0;i<16;i++){
        pname[i]=pname_[i];
    }
}


Parti::~Parti() {
}

