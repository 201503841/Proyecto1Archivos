/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   Parti.h
 * Author: heidy
 *
 * Created on 16 de febrero de 2020, 19:29
 */

#ifndef PARTI_H
#define PARTI_H
//#include <cstdlib>
#include <string>

using namespace std;

class Parti {
public:
    char estado;
    char tipo;
    char fit;
    int inicio;
    int tamano;
    char pname[16];
    Parti();
    Parti(char estado_,char tipo_ ,char fit_, int inicio_, int tamano_, char pname_[16]);
    Parti(const Parti& orig);
    virtual ~Parti();
private:

};

#endif /* PARTI_H */
