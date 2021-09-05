#ifndef EBR_H
#define EBR_H

class EBR {
public:
    char estado;
    char fit;
    int inicio;
    int tamano;
    int next;
    char lname[16];
    EBR();
    EBR(char estado_, char fit_, int inicio_,int tamano,int next_,char lname_[16]);
    EBR(const EBR& orig);
    virtual ~EBR();
private:

};

#endif /* EBR */
