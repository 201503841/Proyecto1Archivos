
#ifndef MBR_H
#define MBR_H
#include "parti.h"

class MBR {
public:
    int mbr_tamano;
    char mbr_fecha_creacion[30]="";
    int mbr_disk_signature;
    char disk_fit;
    Parti array_particion[4];
    MBR();
    MBR(int mbr_tamano_, int mbr_disk_signature_, char disk_fit_);
    MBR(const MBR& orig);
    virtual ~MBR();
private:

};

#endif /* MBR_H */
