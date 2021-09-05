
#ifndef GRUPO_H
#define GRUPO_H

class Grupo {
public:
    int gid;
    char nombre[10];
    Grupo();
    Grupo(int gid_,char nombre_[10]);
    Grupo(const Grupo& orig);
    virtual ~Grupo();
private:

};

#endif /* GRUPO_H */
