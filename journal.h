
#include <time.h>
#ifndef JOURNAL_H
#define JOURNAL_H



class Journal {
struct mkuser {
    int nusuario;
    char usuario[10];
    char pass[10];
    char grupo[10];
};
struct rmuser {
    char usuario[10];
};
struct mkgroup {
    char name[10];
};
struct rmgroup {
    char name[10];
};
struct mkcarpet{
    char path[500];
    int p;
};
struct mkArchivo{
    char path[500];
    int p;
    int size;
    char cont[500];
};
public:
    char tipo_operacion[10];
    char tipo[10];
    char    nombre[500];
    char    contenido[600];
    time_t  fecha;
    int     propietario;
    int     permisos;
    mkuser comandomkusr;
    rmuser comandormuser;
    mkgroup comandomkgroup;
    rmgroup comandormgroup;
    mkArchivo comandomkarchivo;
    mkcarpet comandomkcarpeta;
    Journal* siguiente;
    Journal* previo;
    Journal();
    Journal(const Journal& orig);
    virtual ~Journal();
private:

};

#endif /* JOURNAL_H */
