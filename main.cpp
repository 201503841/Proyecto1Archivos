
#include <iostream>
#include <fstream>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <strings.h>
#include <libgen.h>
#include <math.h>
#include <stdint.h>
#include "mbr.h"
#include "ebr.h"
#include "mount.h"
#include "superbloque.h"
#include "inodo.h"
#include "journal.h"
#include "bitmap.h"
#include "usuario.h"
#include "grupo.h"

using namespace std;

Mount *primero;
Mount *ultimo;
Usuario actual;
Mount pactual;
int fjournal;

struct lista {
    int indexinodo;
    char *nruta;
    bool existe;
};

struct rutas {
    char * nombre;
    rutas *next;
};

struct Content {
    char b_name[12];
    int b_inodo;
};

struct BloqueCarpetas {
    Content b_content[4];
};

struct BloqueApuntador {
    int b_pointers[16];
};

struct BloqueArchivos {
    char b_content[64];
};
rutas *primera = NULL;
rutas *ultima = NULL;
void RecorreBReporte(Mount*particion, int id, FILE *repo);
void ReporteInodotree(Mount* particion, FILE* repo, int id);
int EjecutaMkdir(char path[500], int bande1, int bande2);
Journal *jprimero;
Journal *jultimo;
int idjournal;

/*
 *
 */


int EscribirArchivo(char ruta[500], int inicio, void *puntero, int tamano) {

    FILE *fp = NULL;

    fp = fopen(ruta, "r+b");
    if (fp == NULL) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!! Error No se pudo abrir el archivo en " << ruta << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        return 0;
    }
    if (fseek(fp, inicio, SEEK_SET) != 0) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error en la posición " << inicio << " en el archivo " << ruta << "!!!!!!!!!!!!!!!!!!!!!! \n";
        return 0;
    }
    size_t prueba = fwrite(puntero, tamano, 1, fp);
    fclose(fp);
    if (prueba == 1) {
        return 1;
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error ocurrió un error al escribir dentro del archivo " << ruta << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        return 0;
    }
}

int vaciarArchivo(char ruta[500], int inicio, int fin, int bff) {
    FILE *discoUbicado = NULL;
    discoUbicado = fopen(ruta, "rb+");
    if (discoUbicado == NULL) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no fue posible abrir el archivo en " << ruta << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        return 0;
    }
    if (fseek(discoUbicado, inicio, SEEK_SET) != 0) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no fue posible ir a la posición " << inicio << " en el archivo " << ruta << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        return 0;
    }
    char *data = NULL;
    int buff = 0;
    int tamano = fin - inicio + 1;
    while (tamano > 0) {
        buff = tamano;
        if (buff <= bff) {
            data = (char*) malloc(buff);
            tamano = 0;
        } else {
            data = (char*) malloc(bff);
            buff = bff;
            tamano = tamano - bff;
        }
        for (int i = 0; i < buff; i++) {
            data[i] = '\0';
        }
        fwrite(data, buff, 1, discoUbicado);
        free(data);
    }

    fclose(discoUbicado);
    return 1;
}

void *readDisk(char ruta[500], int pinicio, int tamano) {

    FILE *fp = NULL;
    fp = fopen(ruta, "rb");
    if (fp == NULL) {
        return NULL;
    }
    if (fseek(fp, pinicio, SEEK_SET) != 0) {
        return NULL;
    }
    void *puntero = (void*) malloc(tamano);
    size_t result = fread(puntero, tamano, 1, fp);
    fclose(fp);
    if (result == 1) {
        return puntero;
    } else {
        return NULL;
    }
}

char *quitaCm(char path[500]) {

    char auxpath[500] = "";
    char *retorno = "";
    strcpy(auxpath, path);
    char auxi[500] = "";
    if (auxpath[0] == '\"') {
        int j = 1;
        path = "/";
        int ct = 0;
        while ((auxpath[j] != '\"') || (auxpath[j] == '\n')) {
            auxi[ct] = auxpath[j];
            ct++;
            j++;
        }

    }

    retorno = auxi;
    return retorno;
}

SuperBloque* BuscarSuperBloque(Mount* Particion) {
    int inicio = Particion->part_start;
    if (Particion->part_type == 'l' || Particion->part_type == 'L') {
        inicio = inicio + sizeof (EBR);
    }
    return (SuperBloque*) readDisk(Particion->path, inicio, sizeof (SuperBloque));
}

int llenarBitMap(Mount* particion, int tipo, int posicion) {
    SuperBloque* sbloque = BuscarSuperBloque(particion);
    int inicio;
    if (tipo == 0) {
        inicio = sbloque->s_bm_inode_start;
    } else {
        inicio = sbloque->s_bm_block_start;
    }
    Bitmap* bm = (Bitmap*) readDisk(particion->path, inicio, sizeof (Bitmap));
    bm->bitmaps[posicion] = '1';
    if (EscribirArchivo(particion->path, inicio, bm, sizeof (Bitmap)) == 0) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se pudo escribir en el bitmap !!!!!!!!!!!!!!!!!!!!!!! \n";
        return -1;
    } else {
        return 1;
    }
}

int bmInodolibre(Mount* particion) {
    SuperBloque* sbloque = BuscarSuperBloque(particion);
    int inicio;
    inicio = sbloque->s_bm_inode_start;
    Bitmap* bm = (Bitmap*) readDisk(particion->path, inicio, sizeof (Bitmap));
    for (int i = 0; i < bm->size; i++) {
        if (bm->bitmaps[i] == '0') {
            return i;
        }
    }
    return -1;
}

int bmBloquelibre(Mount* particion) {
    SuperBloque* sbloque = BuscarSuperBloque(particion);
    int inicio;
    inicio = sbloque->s_bm_block_start;
    Bitmap* bm = (Bitmap*) readDisk(particion->path, inicio, sizeof (Bitmap));
    for (int i = 0; i < bm->size; i++) {
        if (bm->bitmaps[i] == '0') {
            return i;
        }
    }
    return -1;
}

int EscribirInodo(Inodo* in, Mount* particion, int id) {
    SuperBloque* sbloque = BuscarSuperBloque(particion);
    int inicioInodos = sbloque->s_inode_start;
    int posicion = inicioInodos + id * sizeof (Inodo);
    if (EscribirArchivo(particion->path, posicion, in, sizeof (Inodo)) == 1) {
        int bmlibre = bmInodolibre(particion);
        if (bmlibre < 0) {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!! Error no se encontro un bitmap de inodo libre !!!!!!!!!!!!!!!!!!!!\n";
            return 0;
        }
        llenarBitMap(particion, 0, id);
        sbloque->s_free_inodes_count = sbloque->s_free_inodes_count - 1;
        sbloque->s_first_ino = bmInodolibre(particion);
        EscribirArchivo(particion->path, particion->part_start, sbloque, sizeof (SuperBloque));
        return 1;
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no fue posible crear el Inodo de la carpeta raiz !!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
        return 0;
    }
}

int EscribirInodo2(Inodo* in, Mount* particion, int id) {
    SuperBloque* sbloque = BuscarSuperBloque(particion);
    int inicioInodos = sbloque->s_inode_start;
    int posicion = inicioInodos + id * sizeof (Inodo);
    if (EscribirArchivo(particion->path, posicion, in, sizeof (Inodo)) == 1) {
        int bmlibre = bmInodolibre(particion);
        if (bmlibre < 0) {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!! Error no se encontro un bitmap de inodo libre !!!!!!!!!!!!!!!!!!!!\n";
            return 0;
        }
        llenarBitMap(particion, 0, id);
        sbloque->s_first_ino = bmInodolibre(particion);
        EscribirArchivo(particion->path, particion->part_start, sbloque, sizeof (SuperBloque));
        return 1;
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no fue posible crear el Inodo de la carpeta raiz !!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
        return 0;
    }
}

int EscribirBloque(void* bloque, Mount* particion, int id, int tbloque) {
    SuperBloque* sbloque = BuscarSuperBloque(particion);
    int inicioBloques = sbloque->s_block_start;
    int posicion = inicioBloques + id * tbloque;
    if (EscribirArchivo(particion->path, posicion, bloque, tbloque) == 1) {
        int bmlibre = bmBloquelibre(particion);
        if (bmlibre < 0) {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!! Error no se encontro un bitmap de inodo libre !!!!!!!!!!!!!!!!!!!!\n";
            return 0;
        }
        llenarBitMap(particion, 1, id);
        sbloque->s_free_blocks_count = sbloque->s_free_blocks_count - 1;
        sbloque->s_first_blo = bmBloquelibre(particion);
        EscribirArchivo(particion->path, particion->part_start, sbloque, sizeof (SuperBloque));
        return 1;
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no fue posible crear el Inodo de la carpeta raiz !!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
        return 0;
    }
}

int EscribirBloque2(void* bloque, Mount* particion, int id, int tbloque) {
    SuperBloque* sbloque = BuscarSuperBloque(particion);
    int inicioBloques = sbloque->s_block_start;
    int posicion = inicioBloques + id * tbloque;
    if (EscribirArchivo(particion->path, posicion, bloque, tbloque) == 1) {
        int bmlibre = bmBloquelibre(particion);
        if (bmlibre < 0) {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!! Error no se encontro un bitmap de inodo libre !!!!!!!!!!!!!!!!!!!!\n";
            return 0;
        }
        llenarBitMap(particion, 1, id);
        sbloque->s_first_blo = bmBloquelibre(particion);
        EscribirArchivo(particion->path, particion->part_start, sbloque, sizeof (SuperBloque));
        return 1;
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no fue posible crear el Inodo de la carpeta raiz !!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
        return 0;
    }
}

int EscribirJournal(Mount* particion, int id, Journal* jour) {
    SuperBloque* sbloque = BuscarSuperBloque(particion);
    int inicioInodos = sbloque->s_journal_start;
    int posicion = inicioInodos + id * sizeof (Journal);
    if (EscribirArchivo(particion->path, posicion, jour, sizeof (Journal)) == 1) {
        return 1;
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no fue posible crear el Inodo de la carpeta raiz !!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
        return 0;
    }
}

void AgregarNodoJournal(Journal*NodoNuevo) {
    if (jprimero == NULL) {
        jprimero = NodoNuevo;
        jultimo = NodoNuevo;
    } else {
        jultimo->siguiente = NodoNuevo;
        NodoNuevo->previo = jultimo;
        jultimo = NodoNuevo;
    }
}

int recorreC(char*Ca, char*SC) {
    int c = 0;
    int s = 0;
    while (Ca[c]) {
        if (Ca[c] == SC[s]) {
            s++;
            if (s >= strlen(SC)) {
                return c;
            }
        }
        c++;

    }
    return 0;


}

int BuscarInodo(Mount* particion, char nombre[12], int inodoId) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int posicion = sb->s_inode_start + inodoId * sizeof (Inodo);
    Inodo* tempo = (Inodo*) readDisk(particion->path, posicion, sizeof (Inodo));
    int ridinodo = -1;
    for (int i = 0; i < 12; i++) {
        if (tempo->i_block[i] != -1) {
            int piniciobloque = sb->s_block_start + tempo->i_block[i] * sizeof (BloqueCarpetas);
            BloqueCarpetas* btempo = (BloqueCarpetas*) readDisk(particion->path, piniciobloque, sizeof (BloqueCarpetas));
            for (int j = 0; j < 4; j++) {
                if (strcasecmp(btempo->b_content[j].b_name, nombre) == 0) {
                    ridinodo = btempo->b_content[j].b_inodo;
                }
            }
            if (ridinodo>-1) {
                return ridinodo;
            }
        }
    }
}

int idCarpeta(Mount* particion, char ruta[500]) {
    char auxpath[500];
    strncpy(auxpath, ruta, 500);

    char *cut = strtok(auxpath, "/");
    int inodoId = 0;
    while (cut != NULL) {
        inodoId = BuscarInodo(particion, cut, inodoId);
        if (inodoId == -1 || inodoId < -1) {
            return -1;
        }
        cut = strtok(NULL, "/");
    }
    return inodoId;
}

int idCarpeta2(Mount* particion, char ruta[500]) {
    char auxpath[500];
    strncpy(auxpath, ruta, 500);

    char *arch = basename(auxpath);
    char *cut = strtok(auxpath, "/");
    int inodoId = 0;
    while (cut != NULL) {
        if (cut != arch) {
            inodoId = BuscarInodo(particion, cut, inodoId);
            if (inodoId == -1) {
                return -1;
            }
        }
        cut = strtok(NULL, "/");
    }
    return inodoId;
}

BloqueCarpetas * InicializarBC(int b_inode0, char b_name0[12], int b_inode1, char b_name1[12], int b_inode2, char b_name2[12], int b_inode3, char b_name3[12]) {
    BloqueCarpetas* llenar = (BloqueCarpetas*) malloc(sizeof (BloqueCarpetas));
    ;
    llenar->b_content[0].b_inodo = b_inode0;
    llenar->b_content[1].b_inodo = b_inode1;
    llenar->b_content[2].b_inodo = b_inode2;
    llenar->b_content[3].b_inodo = b_inode3;

    for (int i = 0; i < 12; i++) {
        llenar->b_content[0].b_name[i] = b_name0[i];
        llenar->b_content[1].b_name[i] = b_name1[i];
        llenar->b_content[2].b_name[i] = b_name2[i];
        llenar->b_content[3].b_name[i] = b_name3[i];
    }
    return llenar;
}

int ApuntadorSimple(Mount* particion, int idbs, char nombre[12], int idactual) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinicio = sb->s_block_start + idbs * sizeof (BloqueApuntador);
    BloqueApuntador* btempo = (BloqueApuntador*) readDisk(particion->path, pinicio, sizeof (BloqueApuntador));

    for (int i = 0; i < 16; i++) {
        if (btempo->b_pointers[i] == -1) {

            BloqueCarpetas* bcarp = InicializarBC(idactual, nombre, -1, "", -1, "", -1, "");
            int idbcarp = bmBloquelibre(particion);
            EscribirBloque(bcarp, particion, idbcarp, sizeof (BloqueCarpetas));
            btempo->b_pointers[i] = idbcarp;
            EscribirBloque2(btempo, particion, idbs, sizeof (BloqueApuntador));
            return 1;
        } else {
            int pbcarpeta = sb->s_block_start + btempo->b_pointers[i] * sizeof (BloqueCarpetas);
            BloqueCarpetas* bcarp = (BloqueCarpetas*) readDisk(particion->path, pbcarpeta, sizeof (BloqueCarpetas));
            for (int y = 0; y < 4; y++) {
                if (bcarp->b_content[y].b_inodo == -1) {
                    bcarp->b_content[y].b_inodo = idactual;
                    for (int x = 0; x < 12; x++) {
                        bcarp->b_content[y].b_name[x] = nombre[x];
                    }

                    EscribirBloque2(bcarp, particion, btempo->b_pointers[i], sizeof (BloqueCarpetas));
                    return 1;
                }
            }
            return 1;
        }
    }
    return -1;
}

int ApuntadorDoble(Mount *particion, int idbd, char nombre[12], int idactual) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinicio = sb->s_block_start + idbd * sizeof (BloqueApuntador);
    BloqueApuntador* btempo = (BloqueApuntador*) readDisk(particion->path, pinicio, sizeof (BloqueApuntador));
    int ap = -1;
    for (int i = 0; i < 16; i++) {
        if (btempo->b_pointers[i] == -1) {
            BloqueCarpetas* bcarp = InicializarBC(idactual, nombre, -1, "", -1, "", -1, "");
            int idbcarp = bmBloquelibre(particion);
            EscribirBloque(bcarp, particion, idbcarp, sizeof (BloqueCarpetas));
            btempo->b_pointers[i] = idbcarp;
            EscribirBloque2(btempo, particion, idbd, sizeof (BloqueApuntador));
        }
        ap = ApuntadorSimple(particion, btempo->b_pointers[i], nombre, idactual);
        if (ap > -1) {
            return ap;
        }
    }
    return -1;

}

int ApuntadorTriple(Mount *particion, int idbd, char nombre[12], int idactual) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinicio = sb->s_block_start + idbd * sizeof (BloqueApuntador);
    BloqueApuntador* btempo = (BloqueApuntador*) readDisk(particion->path, pinicio, sizeof (BloqueApuntador));
    int ap = -1;
    for (int i = 0; i < 16; i++) {
        if (btempo->b_pointers[i] == -1) {
            BloqueCarpetas* bcarp = InicializarBC(idactual, nombre, -1, "", -1, "", -1, "");
            int idbcarp = bmBloquelibre(particion);
            EscribirBloque(bcarp, particion, idbcarp, sizeof (BloqueCarpetas));
            btempo->b_pointers[i] = idbcarp;
            EscribirBloque2(btempo, particion, idbd, sizeof (BloqueApuntador));
        }
        ap = ApuntadorDoble(particion, btempo->b_pointers[i], nombre, idactual);
        if (ap > -1) {
            return ap;
        }
    }
    return -1;
}

int CrearArchivo(Mount *particion, int idpadre, char nombre[12], int idactual, int indexa) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinicio = sb->s_inode_start + idpadre * sizeof (Inodo);
    Inodo* tempo = (Inodo*) readDisk(particion->path, pinicio, sizeof (Inodo));
    //apuntadores directos
    if (indexa < 12) {
        if (tempo->i_block[indexa] == -1) {
            BloqueCarpetas*btempo = InicializarBC(idactual, nombre, -1, "", -1, "", -1, "");
            int idbloque = bmBloquelibre(particion);
            EscribirBloque(btempo, particion, idbloque, sizeof (BloqueCarpetas));
            tempo->i_block[indexa] = idbloque;
            EscribirInodo2(tempo, particion, idpadre);
            return 1;
        } else {
            int idbloque = tempo->i_block[indexa];
            int pbloque = sb->s_block_start + idbloque * sizeof (BloqueCarpetas);
            BloqueCarpetas* btempo = (BloqueCarpetas*) readDisk(particion->path, pbloque, sizeof (BloqueCarpetas));
            for (int i = 0; i < 4; i++) {
                if (btempo->b_content[i].b_inodo == -1) {
                    btempo->b_content[i].b_inodo = idactual;
                    for (int j = 0; j < 12; j++) {
                        btempo->b_content[i].b_name[j] = nombre[j];
                    }
                    EscribirBloque2(btempo, particion, idbloque, sizeof (BloqueCarpetas));
                    return 1;
                }
            }
            indexa = indexa + 1;
            CrearArchivo(particion, idpadre, nombre, idactual, indexa);
        }
    }//apuntadores indirectos
    else if (indexa < 15) {
        int rs = -1;
        if (tempo->i_block[indexa] == -1) {
            BloqueApuntador* btempo = new BloqueApuntador();
            for (int i = 0; i < 16; i++) {
                btempo->b_pointers[i] = -1;
            }
            int idbloque = bmBloquelibre(particion);
            if (idbloque < 0) {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se encontro un bitmap libre para el bloque de apuntadores !!!!!!!!!!!!!!!!!!!!!! \n";

            }
            EscribirBloque(btempo, particion, idbloque, sizeof (BloqueApuntador));
            tempo->i_block[indexa] = idbloque;
            EscribirInodo2(tempo, particion, idpadre);
        }

        if (indexa == 12) {
            rs = ApuntadorSimple(particion, tempo->i_block[indexa], nombre, idactual);
        } else if (indexa == 13) {
            rs = ApuntadorDoble(particion, tempo->i_block[indexa], nombre, idactual);
        } else if (indexa == 14) {
            return ApuntadorTriple(particion, tempo->i_block[indexa], nombre, idactual);
        }

        if (rs > -1) {
            return rs;
        } else {
            return CrearArchivo(particion, idpadre, nombre, idactual, ++indexa);
        }
    }
}

int ArApuntadorSimple(Mount* particion, int idbs, int idactual) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinicio = sb->s_block_start + idbs * sizeof (BloqueApuntador);
    BloqueApuntador* btempo = (BloqueApuntador*) readDisk(particion->path, pinicio, sizeof (BloqueApuntador));
    for (int i = 0; i < 16; i++) {
        if (btempo->b_pointers[i] == -1) {
            btempo->b_pointers[i] = idactual;
            EscribirBloque2(btempo, particion, idbs, sizeof (BloqueApuntador));
            return 1;
        }
    }
    return -1;
}

int ArApuntadorDoble(Mount* particion, int idbs, int idactual) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinicio = sb->s_block_start + idbs * sizeof (BloqueApuntador);
    BloqueApuntador* btempo = (BloqueApuntador*) readDisk(particion->path, pinicio, sizeof (BloqueApuntador));
    int mv = -1;
    for (int i = 0; i < 16; i++) {
        if (btempo->b_pointers[i] == -1) {
            BloqueApuntador* btempo2 = new BloqueApuntador();
            for (int i = 0; i < 16; i++) {
                btempo2->b_pointers[i] = -1;
            }
            int idbloqueAp = bmBloquelibre(particion);
            if (idbloqueAp < 0) {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se encontro un bitmap libre para el bloque de apuntadores !!!!!!!!!!!!!!!!!!!!!! \n";

            }
            EscribirBloque(btempo2, particion, idbloqueAp, sizeof (BloqueApuntador));
            btempo->b_pointers[i] = idbloqueAp;
            EscribirBloque2(btempo, particion, idbs, sizeof (BloqueApuntador));
        }
        mv = ArApuntadorSimple(particion, btempo->b_pointers[i], idactual);
        if (mv > -1) {
            return mv;
        }
    }
    return -1;
}

int ArApuntadorTriple(Mount* particion, int idbs, int idactual) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinicio = sb->s_block_start + idbs * sizeof (BloqueApuntador);
    BloqueApuntador* btempo = (BloqueApuntador*) readDisk(particion->path, pinicio, sizeof (BloqueApuntador));
    int mv = -1;
    for (int i = 0; i < 16; i++) {
        if (btempo->b_pointers[i] == -1) {
            BloqueApuntador* btempo2 = new BloqueApuntador();
            for (int i = 0; i < 16; i++) {
                btempo2->b_pointers[i] = -1;
            }
            int idbloqueAp = bmBloquelibre(particion);
            if (idbloqueAp < 0) {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se encontro un bitmap libre para el bloque de apuntadores !!!!!!!!!!!!!!!!!!!!!! \n";

            }
            EscribirBloque(btempo2, particion, idbloqueAp, sizeof (BloqueApuntador));
            btempo->b_pointers[i] = idbloqueAp;
            EscribirBloque2(btempo, particion, idbs, sizeof (BloqueApuntador));
        }
        mv = ArApuntadorDoble(particion, btempo->b_pointers[i], idactual);
        if (mv > -1) {
            return mv;
        }
    }
    return -1;
}

int EscribirContenido2(Mount *particion, Inodo *inodo, int idinodo, int idbloque, int indexa) {
    if (indexa < 12) {
        if (inodo->i_block[indexa] == -1) {
            inodo->i_block[indexa] = idbloque;
            EscribirInodo2(inodo, particion, idinodo);
            return 1;
        } else {
            indexa = indexa + 1;
            return EscribirContenido2(particion, inodo, idinodo, idbloque, indexa);
        }
    } else if (indexa < 15) {
        int mv = -1;
        if (inodo->i_block[indexa] == -1) {
            BloqueApuntador* btempo = new BloqueApuntador();
            for (int i = 0; i < 16; i++) {
                btempo->b_pointers[i] = -1;
            }
            int idbloqueAp = bmBloquelibre(particion);
            if (idbloqueAp < 0) {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se encontro un bitmap libre para el bloque de apuntadores !!!!!!!!!!!!!!!!!!!!!! \n";

            }
            EscribirBloque(btempo, particion, idbloqueAp, sizeof (BloqueApuntador));
            inodo->i_block[indexa] = idbloqueAp;
            EscribirInodo2(inodo, particion, idinodo);
        }

        if (indexa == 12) {
            mv = ArApuntadorSimple(particion, inodo->i_block[indexa], idbloque);
        } else if (indexa == 13) {
            mv = ArApuntadorDoble(particion, inodo->i_block[indexa], idbloque);
        } else if (indexa == 14) {
            return mv = ArApuntadorTriple(particion, inodo->i_block[indexa], idbloque);
        }
        if (mv>-1) {
            return mv;
        } else {
            indexa = indexa + 1;
            return EscribirContenido2(particion, inodo, idinodo, idbloque, indexa);
        }
    }
}

bool EscribirContenido(Mount* particion, int inodoPadre, char Contenido[280320]) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinicio = sb->s_inode_start + inodoPadre * sizeof (Inodo);
    Inodo* in = (Inodo*) readDisk(particion->path, pinicio, sizeof (Inodo));
    float nblock = strlen(Contenido);
    nblock = nblock / 64 + 0.99;
    int bloques = nblock;
    bool escribe = false;
    int blcount = 0;
    int i = 0;
    while (blcount < bloques) {
        BloqueArchivos* bnuevo = new BloqueArchivos();
        int idbloque = bmBloquelibre(particion);
        for (int x = 0; x < 64; x++) {
            bnuevo->b_content[x] = Contenido[x + (64 * i)];
        }
        blcount++;
        int pbloque = sb->s_block_start + idbloque * sizeof (BloqueArchivos);
        EscribirBloque(bnuevo, particion, idbloque, sizeof (BloqueArchivos));
        EscribirContenido2(particion, in, inodoPadre, idbloque, 0);
        i++;
        escribe = true;
    }
    return escribe;
}

void mkfile(char path[500], int size, char cont[500], int bande, int p) {
    char auxpath[500] = "";
    strncpy(auxpath, path, 500);
    char *name = basename(auxpath);
    if (bande == 1) {
        SuperBloque* sb = BuscarSuperBloque(actual.particion);
        char data[150];
        if (size > 0) {
            char a = '0';
            int contador = 0;

            while (contador < size) {
                a = '0';
                for (int y = 0; y <= 9; y++) {

                    if (contador < size) {
                        data[contador] = a;
                    } else {
                        break;
                    }
                    contador++;
                    a++;
                }
            }
        } else if (cont[0] != '\0') {
            int w = 0;
            while (cont[w]) {
                cont[w] = tolower(cont[w]);
            }
            FILE *archivo;
            char caracter;

            archivo = fopen(cont, "r");

            if (archivo == NULL) {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se pudo abrir !!!!!!!!!!!!!!!!!!!!!!!!\n";

            } else {
                int c = 0;
                while ((caracter = fgetc(archivo)) != EOF) {
                    if (c < 150) {
                        data[c] = caracter;
                        c++;
                    }
                }
            }
            fclose(archivo);
        }

        if (idCarpeta2(actual.particion, path)>-1) {
            Inodo* nuevo = new Inodo(actual.uid, actual.gid, 0, time(NULL), time(NULL), time(NULL), 1, 664);
            int idactual = bmInodolibre(actual.particion);
            EscribirInodo(nuevo, actual.particion, idactual);
            int dap = idCarpeta2(actual.particion, path);
            CrearArchivo(actual.particion, dap, name, idactual, 0);

            if (EscribirContenido(actual.particion, idactual, data)) {
                cout << ">>>>>>>>>>>>>>>>>>>> Se creó el archivo " << name << "\n";
                if (fjournal == 1) {
                    Journal* tempo = new Journal();
                    strcpy(tempo->comandomkarchivo.path, path);
                    strcpy(tempo->comandomkarchivo.cont, cont);
                    tempo->comandomkarchivo.size = size;
                    tempo->comandomkcarpeta.p = bande;
                    strcpy(tempo->tipo_operacion, "mkfile");
                    strcpy(tempo->contenido, data);
                    strcpy(tempo->tipo, "Archivo");
                    strcpy(tempo->nombre, name);
                    tempo->fecha = time(NULL);
                    idjournal++;
                    EscribirJournal(actual.particion, idjournal, tempo);
                }
            }

        } else {
            if (p == 1) {
                char auxpath4[500] = "";
                strncpy(auxpath4, path, 500);
                char *tempo2 = strdup(auxpath4);
                char *pathc = dirname(tempo2);
                int id = EjecutaMkdir(pathc, 1, 1);
                if (id>-1) {
                    Inodo* nuevo = new Inodo(actual.uid, actual.gid, 0, time(NULL), time(NULL), time(NULL), 1, 664);
                    int idactual = bmInodolibre(actual.particion);
                    EscribirInodo(nuevo, actual.particion, idactual);
                    int dap = idCarpeta2(actual.particion, path);
                    CrearArchivo(actual.particion, dap, name, idactual, 0);

                    if (EscribirContenido(actual.particion, idactual, data)) {
                        cout << ">>>>>>>>>>>>>>>>>>>> Se creó el archivo " << name << "\n";
                        if (fjournal == 1) {
                            Journal* tempo = new Journal();
                            strcpy(tempo->comandomkarchivo.path, path);
                            strcpy(tempo->comandomkarchivo.cont, cont);
                            tempo->comandomkarchivo.size = size;
                            tempo->comandomkcarpeta.p = bande;
                            strcpy(tempo->tipo_operacion, "mkfile");
                            strcpy(tempo->contenido, data);
                            strcpy(tempo->tipo, "Archivo");
                            strcpy(tempo->nombre, name);
                            tempo->fecha = time(NULL);
                            idjournal++;
                            EscribirJournal(actual.particion, idjournal, tempo);
                        }
                    }
                }
            } else {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error la carpeta donde desea crear el archivo no existe !!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            }
        }
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!! Error falta un parametro obligatorio en mkfile !!!!!!!!!!!!!!!!!!!!!!!!!!\n";

    }
}

void analizamkfile(char comando[10000]) {
    int bande1 = 0;
    char auxcm[10000];
    strcpy(auxcm, comando);
    char *varpath;
    int varsize;
    char *varcont;
    char auxi[10000] = "";
    int j = 0;
    int ct = 0;
    int bandepfija = 0;
    bool bandeaux1 = false;
    bool bandeaux2 = false;
    bool bandeauxP = false;
    bool bandeauxP2 = true;
    while (auxcm[j] != '\0') {
        bandeauxP2 = false;
        if (auxcm[j] == '-') {
            bandeaux1 = true;
        }

        if (bandeaux1) {
            if (auxcm[j] == 'p' || auxcm[j] == 'P') {
                bandeaux2 = true;
            }
        }
        if (bandeaux1 && bandeaux2) {
            if (auxcm[j + 1] != '\0') {
                if (auxcm[j + 1] == 'a' || auxcm[j + 1] == 'A') {
                    bandeaux1 = false;
                    bandeaux2 = false;
                } else {
                    if (auxcm[j + 1] == ' ') {
                        bandeauxP = true;
                        bandeauxP2 = true;
                        auxi[j] = '\0';
                        auxi[j - 1] = '\0';
                        ct = ct - 3;
                        bandeaux1 = false;
                        bandeaux2 = false;
                    }
                }
            } else {
                bandeauxP = true;
                bandeauxP2 = true;
                auxi[j] = '\0';
                auxi[j - 1] = '\0';
                bandeaux1 = false;
                bandeaux2 = false;
            }
        }
        if (!bandeauxP2) {
            auxi[ct] = auxcm[j];
        }
        ct++;
        j++;
    }
    char *cut = strtok(auxi, " ");
    cut = strtok(NULL, " =");
    while (cut != NULL) {
        if (strcasecmp(cut, "-path") == 0) {
            cut = strtok(NULL, "= ");
            bande1 = 1;
            if (*cut != '\"') {
                varpath = cut;
            } else {
                varpath = quitaCm(cut);
            }
        } else if (strcasecmp(cut, "-size") == 0) {
            cut = strtok(NULL, "= ");
            varsize = atoi(cut);
        } else if (strcasecmp(cut, "-cont") == 0) {
            cut = strtok(NULL, "= ");
            varcont = cut;
        } else {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error en el comando Mkfile !!!!!!!!!!!!!!!!!!!!!!!!\n");
        }
        cut = strtok(NULL, "= ");
    }

    if (bandeauxP) {
        bandepfija = 1;
    }
    mkfile(varpath, varsize, varcont, bande1, bandepfija);
}

int datamin;

int CrearCarpeta(Mount* particion, int idpadre, char nombre[12], int idactual, int indexa) {
    datamin = indexa;
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinicio = sb->s_inode_start + idpadre * sizeof (Inodo);
    Inodo* tempo = (Inodo*) readDisk(particion->path, pinicio, sizeof (Inodo));
    //apuntadores directos
    if (indexa < 12) {
        if (tempo->i_block[indexa] == -1) {
            BloqueCarpetas* btempo = InicializarBC(idactual, nombre, -1, "", -1, "", -1, "");
            int idbloque = bmBloquelibre(particion);
            EscribirBloque(btempo, particion, idbloque, sizeof (BloqueCarpetas));
            tempo->i_block[indexa] = idbloque;
            EscribirInodo2(tempo, particion, idpadre);
            return idbloque;
        } else {
            int idbloque = tempo->i_block[indexa];
            int pbloque = sb->s_block_start + idbloque * sizeof (BloqueCarpetas);
            BloqueCarpetas* btempo = (BloqueCarpetas*) readDisk(particion->path, pbloque, sizeof (BloqueCarpetas));
            for (int i = 0; i < 4; i++) {
                if (btempo->b_content[i].b_inodo == -1) {

                    btempo->b_content[i].b_inodo = idactual;
                    for (int j = 0; j < 12; j++) {
                        btempo->b_content[i].b_name[j] = nombre[j];
                    }
                    EscribirBloque2(btempo, particion, idbloque, sizeof (BloqueCarpetas));
                    return idbloque;
                }
            }
            indexa = indexa + 1;
            CrearCarpeta(particion, idpadre, nombre, idactual, indexa);
        }
    } else if (indexa < 15) {
        int rs = -1;
        if (tempo->i_block[indexa] == -1) {
            BloqueApuntador* btempo = new BloqueApuntador();
            for (int i = 0; i < 16; i++) {
                btempo->b_pointers[i] = -1;
            }
            int idbloque = bmBloquelibre(particion);
            if (idbloque < 0) {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se encontro un bitmap libre para el bloque de apuntadores !!!!!!!!!!!!!!!!!!!!!! \n";

            }
            EscribirBloque(btempo, particion, idbloque, sizeof (BloqueApuntador));
            tempo->i_block[indexa] = idbloque;
            EscribirInodo2(tempo, particion, idpadre);
        }

        if (indexa == 12) {
            rs = ApuntadorSimple(particion, tempo->i_block[indexa], nombre, idactual);
        } else if (indexa == 13) {
            rs = ApuntadorDoble(particion, tempo->i_block[indexa], nombre, idactual);
        } else if (indexa == 14) {
            return ApuntadorTriple(particion, tempo->i_block[indexa], nombre, idactual);
        }

        if (rs > -1) {
            return rs;
        } else {
            return CrearCarpeta(particion, idpadre, nombre, idactual, ++indexa);
        }

    }

}

int crearBloqueCarpeta(Mount* particion, int idpadre, char nombrepadre[12]) {
    int idbloque = bmBloquelibre(particion);
    if (idbloque < 0) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se encontro un bloque libre !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
        return -1;
    }
    BloqueCarpetas* baux = InicializarBC(idpadre, nombrepadre, idbloque, "..", -1, "", -1, "");
    if (EscribirBloque(baux, particion, idbloque, sizeof (BloqueCarpetas)) == 0) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se puede escribir el bloque de carpetas !!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    }
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinodo = sb->s_inode_start + idpadre * sizeof (Inodo);
    Inodo* padre = (Inodo*) readDisk(particion->path, pinodo, sizeof (Inodo));
    padre->i_block[0] = idbloque;
    if (EscribirInodo2(padre, particion, idpadre) == 0) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se puede escribir el inodo !!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    }
}

int EjecutaMkdir(char path[500], int bande1, int bande2) {
    char auxpath[500] = "";

    strncpy(auxpath, path, strlen(path));
    if (bande1 == 1) {
        int idInodo = idCarpeta(actual.particion, path);
        if (idInodo > 0) {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!Error la carpeta " << path << "ya existe !!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
            return -1;
        }

        if (auxpath[strlen(auxpath) - 1] == '/') {
            auxpath[strlen(auxpath) - 1] = '\0';
        }
        char *cut = strtok(auxpath, "/");
        char *padre = NULL;
        int idInodo1 = 0;
        int idpadre = 0;
        while (cut != NULL) {
            idpadre = idInodo1;
            idInodo1 = BuscarInodo(actual.particion, cut, idInodo1);
            padre = cut;
            cut = strtok(NULL, "/");
            if (idInodo1 < 0) {
                //Aqui se valuan permiso

                if (cut == NULL) {
                    Inodo* tempo = new Inodo(actual.uid, actual.gid, 0, time(NULL), time(NULL), time(NULL), 0, 664);
                    idInodo1 = bmInodolibre(actual.particion);
                    EscribirInodo(tempo, actual.particion, idInodo1);
                    if (idInodo1 > 0) {
                        CrearCarpeta(actual.particion, idpadre, padre, idInodo1, 0);
                        crearBloqueCarpeta(actual.particion, idInodo1, padre);
                    }
                    if (idInodo1 > -1) {
                        printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Se creó la ruta %s\n", path);
                        if (fjournal == 1) {
                            Journal* tempo = new Journal();
                            strcpy(tempo->comandomkcarpeta.path, path);
                            tempo->comandomkcarpeta.p = bande2;
                            strcpy(tempo->tipo_operacion, "mkdir");
                            strcpy(tempo->contenido, "----");
                            strcpy(tempo->nombre, path);
                            strcpy(tempo->tipo, "Carpeta");
                            tempo->fecha = time(NULL);
                            idjournal++;
                            EscribirJournal(actual.particion, idjournal, tempo);
                        }
                        return idInodo1;
                    } else {
                        printf("---> Error, no fue posible crear la ruta %s\n", path);
                        return -1;
                    }
                } else if (bande2 == 1) {
                    Inodo* tempo = new Inodo(actual.uid, actual.gid, 0, time(NULL), time(NULL), time(NULL), 0, 664);
                    idInodo1 = bmInodolibre(actual.particion);
                    EscribirInodo(tempo, actual.particion, idInodo1);
                    if (idInodo1 > 0) {
                        CrearCarpeta(actual.particion, idpadre, padre, idInodo1, 0);
                        crearBloqueCarpeta(actual.particion, idInodo1, padre);
                    }
                    if (idInodo1 > -1) {
                        cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Se creó la carpeta padre " << padre << "\n";
                    } else {
                        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no fue posible crear la carpeta padre " << padre << " !!!!!!!!!!!!!!!!!!!!!!!!!\n";
                        return -1;
                    }
                } else {
                    cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no existe la carpeta padre " << padre << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
                    return -1;
                }
            }
        }

    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!! Error falta un parametro obligatorio en mkfile !!!!!!!!!!!!!!!!!!!!!!!!!!\n";
    }
}

void analizamkdir(char comando[10000]) {
    int bande1 = 0;
    char auxcm[10000];
    strcpy(auxcm, comando);
    char *varpath;
    int varsize;
    char *varcont;
    char auxi[10000] = "";
    int j = 0;
    int ct = 0;
    bool bandeaux1 = false;
    bool bandeaux2 = false;
    bool bandeauxP = false;
    bool bandeauxP2 = true;
    while (auxcm[j] != '\0') {
        bandeauxP2 = false;
        if (auxcm[j] == '-') {
            bandeaux1 = true;
        }

        if (bandeaux1) {
            if (auxcm[j] == 'p' || auxcm[j] == 'P') {
                bandeaux2 = true;
            }
        }
        if (bandeaux1 && bandeaux2) {
            if (auxcm[j + 1] != '\0') {
                if (auxcm[j + 1] == 'a' || auxcm[j + 1] == 'A') {
                    bandeaux1 = false;
                    bandeaux2 = false;
                } else {
                    if (auxcm[j + 1] == ' ') {
                        bandeauxP = true;
                        bandeauxP2 = true;
                        auxi[j] = '\0';
                        auxi[j - 1] = '\0';
                        ct = ct - 3;
                        bandeaux1 = false;
                        bandeaux2 = false;
                    }
                }
            } else {
                bandeauxP = true;
                bandeauxP2 = true;
                auxi[j] = '\0';
                auxi[j - 1] = '\0';
                bandeaux1 = false;
                bandeaux2 = false;
            }
        }
        if (!bandeauxP2) {
            auxi[ct] = auxcm[j];
        }
        ct++;
        j++;
    }
    char *cut = strtok(auxi, " ");
    cut = strtok(NULL, " =");
    while (cut != NULL) {
        if (strcasecmp(cut, "-path") == 0) {
            cut = strtok(NULL, "= ");
            bande1 = 1;
            if (*cut != '\"') {
                varpath = cut;
            } else {
                varpath = quitaCm(cut);
            }
        } else {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error en el comando Mkfile !!!!!!!!!!!!!!!!!!!!!!!!\n");
        }
        cut = strtok(NULL, "= ");
    }
    EjecutaMkdir(varpath, bande1, bandeauxP);
}

void rmusr(char usr[10], int bande) {

    if (bande == 1) {
        if (actual.uid == 1) {
            char linea[1024];
            FILE *f = fopen("Users.txt", "r+");
            if (f == NULL) {
                perror("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se pudo abrir users.txt !!!!!!!!!!!!!!!!!!!!!!!!\n");
                return;
            }
            int contador = 0;
            int tipo = 0;
            ;

            while (fgets(linea, 1024, (FILE*) f)) {
                int tlinea = strlen(linea);
                const char splt[2] = ",";
                char *cut = (char*) malloc(4 * sizeof (char*));
                cut = strtok(linea, splt);
                contador = 0;
                tipo = 0;
                Usuario aux;
                for (int i = 0; i < 10; i++) {
                    aux.nombre[i] = '\0';
                    aux.pass[i] = '\0';
                }
                aux.uid = 0;
                while (cut != NULL) {
                    if (contador == 0) {
                        aux.uid = atoi(cut);
                    } else if (contador == 1) {
                        if (strcmp(cut, "U") == 0)tipo = 1;
                    } else if (contador == 2) {
                        int e = 0;
                        for (int i = 0; i<sizeof (cut); i++) {
                            if (cut[i] == '\n') {

                            } else {
                                aux.gid[i] = cut[i];
                            }
                            e++;
                        }
                        for (int i = e; i < 10; i++) {
                            aux.gid[i] = '\0';
                        }

                    } else if (contador == 3) {
                        int e = 0;
                        for (int i = 0; i<sizeof (cut); i++) {
                            if (cut[i] == '\n') {

                            } else {
                                aux.nombre[i] = cut[i];
                            }
                            e++;
                        }
                        for (int i = e; i < 10; i++) {
                            aux.nombre[i] = '\0';
                        }
                    } else if (contador == 4) {
                        int e = 0;
                        for (int i = 0; i<sizeof (cut); i++) {
                            if (cut[i] == '\n') {

                            } else {
                                aux.pass[i] = cut[i];
                            }
                            e++;
                        }
                        for (int i = e; i < 10; i++) {
                            aux.pass[i] = '\0';
                        }
                    }
                    contador++;
                    cut = strtok(NULL, splt);
                }
                if (tipo == 1) {
                    if (strcmp(aux.nombre, usr) == 0) {
                        int posA = ftell(f);

                        fseek(f, posA - tlinea, SEEK_SET);
                        fputc('0', f);
                        fclose(f);
                        cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>El usuario se eliminó exitosamente\n";
                        if (fjournal == 1) {
                            Journal* tempo = new Journal();
                            strcpy(tempo->comandormuser.usuario, usr);
                            strcpy(tempo->tipo_operacion, "rmusr");
                            strcpy(tempo->tipo, "..");
                            strcpy(tempo->contenido, "-------");
                            strcpy(tempo->nombre, usr);
                            tempo->fecha = time(NULL);
                            idjournal++;
                            EscribirJournal(actual.particion, idjournal, tempo);
                        }
                        return;
                    }
                }
            }
            cout << "!!!!!!!!!!!!!!!!!!!!!Error no se encontró el usuario que se desea eliminar !!!!!!!!!!!!!!!!!!!!!\n";
            fclose(f);
            return;

        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!Error solo el usuario root puede eliminar usuarios !!!!!!!!!!!!!!!!!!!!!!!!!!!\n";

        }
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!! Error falta un parametro obligatorio en rmusr !!!!!!!!!!!!!!!!!!!!!!!!!!\n";
    }

}

void analizarmusr(char comando[10000]) {
    int bande1 = 0;
    char *cut = strtok(comando, " ");
    cut = strtok(NULL, " =");
    char *id;
    while (cut != NULL) {
        if (strcasecmp(cut, "-usr") == 0) {
            cut = strtok(NULL, "= ");
            bande1 = 1;
            id = cut;
        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Existe un error en el comando rmusr !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            return;
        }
        cut = strtok(NULL, "= ");
    }
    rmusr(id, bande1);
}

void mkusr(char usr[10], char pass[10], char grp[10], int bande, int bande1, int bande2) {
    if (bande == 1 && bande1 == 1 && bande2 == 1) {
        if (actual.uid == 1) {


            int GExiste = 0;
            char linea[1024];
            FILE *archivo = fopen("Users.txt", "r+");
            if (archivo == NULL) {
                perror("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se pudo abrir users.txt !!!!!!!!!!!!!!!!!!!!!!!!\n");
                return;
            }
            int contador = 0;
            int tipo = 0;

            int cttt = 1;
            while (fgets(linea, 1024, (FILE*) archivo)) {

                const char separador[2] = ",";
                char *cut = (char*) malloc(4 * sizeof (char*));
                cut = strtok(linea, separador);
                Usuario aux;
                contador = 0;
                for (int i = 0; i < 10; i++) {
                    aux.nombre[i] = '\0';
                    aux.pass[i] = '\0';
                }
                aux.uid = 0;
                while (cut != NULL) {
                    if (contador == 0) {
                        aux.uid = atoi(cut);
                    } else if (contador == 1) {
                        if (strcmp(cut, "U") == 0)tipo = 1;
                    } else if (contador == 2) {
                        int e = 0;
                        for (int i = 0; i<sizeof (cut); i++) {
                            if (cut[i] == '\n') {

                            } else {
                                aux.gid[i] = cut[i];
                            }
                            e++;
                        }
                        for (int i = e; i < 10; i++) {
                            aux.gid[i] = '\0';
                        }

                    } else if (contador == 3) {
                        int e = 0;
                        for (int i = 0; i<sizeof (cut); i++) {
                            if (cut[i] == '\n') {

                            } else {
                                aux.nombre[i] = cut[i];
                            }
                            e++;
                        }
                        for (int i = e; i < 10; i++) {
                            aux.nombre[i] = '\0';
                        }
                    } else if (contador == 4) {
                        int e = 0;
                        for (int i = 0; i<sizeof (cut); i++) {
                            if (cut[i] == '\n') {

                            } else {
                                aux.pass[i] = cut[i];
                            }
                            e++;
                        }
                        for (int i = e; i < 10; i++) {
                            aux.pass[i] = '\0';
                        }
                    }
                    contador++;
                    cut = strtok(NULL, separador);
                }
                if (tipo == 1) {
                    cttt++;
                    if (strcmp(aux.nombre, usr) == 0) {
                        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!Error el usuario ya existe !!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
                        fclose(archivo);
                        return;
                    }
                    tipo = 0;
                } else {
                    if (recorreC(aux.gid, "\n") != 0) {
                        aux.gid[recorreC(aux.gid, "\n")] = '\0';
                    }
                    if (strcmp(aux.gid, grp) == 0) {
                        GExiste = 1;
                        if (aux.uid == 0) {
                            GExiste = 0;
                        }


                    }
                }
            }
            if (GExiste == 1) {
                fprintf(archivo, "\n%d,U,%s,%s,%s", cttt, grp, usr, pass);
                fclose(archivo);
                if (fjournal == 1) {
                    Journal* jusuario = new Journal();
                    jusuario->comandomkusr.nusuario = cttt;
                    for (int w = 0; w < 10; w++) {
                        jusuario->comandomkusr.usuario[w] = usr[w];
                        jusuario->comandomkusr.grupo[w] = grp[w];
                        jusuario->comandomkusr.pass[w] = pass[w];
                    }
                    strncpy(jusuario->tipo_operacion, "mkusr", 6);
                    strcpy(jusuario->nombre, usr);
                    strcpy(jusuario->contenido, "-------");
                    strcpy(jusuario->tipo, "..");
                    jusuario->fecha = time(NULL);

                    idjournal++;
                    EscribirJournal(actual.particion, idjournal, jusuario);

                }
                cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>El usuario se creó exitosamente\n";
                return;
            } else {

                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!Error no se puede crear el usuario porque el grupo no existe !!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
                fclose(archivo);
                return;
            }




        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!Error solo el usuario root puede crear grupos !!!!!!!!!!!!!!!!!!!!!!!!!!!\n";

        }
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error falta un parametro en el comando mkusr !!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
    }
}

void analizamkusr(char comando[10000]) {
    int bande1 = 0;
    int bande2 = 0;
    int bande3 = 0;
    char *cut = strtok(comando, " ");
    cut = strtok(NULL, " =");
    char *grp;
    char *pwd;
    char *usr;
    while (cut != NULL) {
        if (strcasecmp(cut, "-grp") == 0) {
            cut = strtok(NULL, "= ");
            bande1 = 1;
            grp = cut;
        } else if (strcasecmp(cut, "-usr") == 0) {
            cut = strtok(NULL, "= ");
            bande2 = 1;
            usr = cut;
        } else if (strcasecmp(cut, "-pwd") == 0) {
            cut = strtok(NULL, "= ");
            bande3 = 1;
            pwd = cut;
        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Existe un error en el comando mkusr !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            return;
        }
        cut = strtok(NULL, "= ");
    }
    mkusr(usr, pwd, grp, bande1, bande2, bande3);
}

void mkfs(char id[16], char type[4], char fs[3], int bande) {
    if (bande == 1) {
        Mount *AuxNode;
        jprimero = NULL;
        jultimo = NULL;
        bool encontro = false;
        if (primero != NULL) {
            AuxNode = primero;
            while (AuxNode != NULL) {
                if (strcasecmp(AuxNode->id, id) == 0) {
                    encontro = true;
                    break;
                }
                AuxNode = AuxNode->next;
            }

            if (!encontro) {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!! Error No se encontró el id buscado, no formateó!!!!!!!!!!!!!!!!!!!!!!!! \n";
                return;
            } else {
                int psize = 0;
                if (AuxNode->part_type == 'E' || AuxNode->part_type == 'e') {
                    cout << "!!!!!!!!!!!!!!!!!!!!!!!!! Error la partición debe ser primaria o logica, no formateó!!!!!!!!!!!!!!!!!!!!!!!! \n";
                    return;
                } else {
                    int iniciop = AuxNode->part_start;
                    psize = AuxNode->part_size;
                    int numInodos;
                    int numBloques;
                    int inicioJournal;
                    int bmInodoInicio;
                    int fsi;
                    if (AuxNode->part_type == 'l' || AuxNode->part_type == 'L') {
                        psize = psize - sizeof (EBR);
                        iniciop = iniciop + sizeof (EBR);
                    }
                    if (fs[0] == '2') {
                        numInodos = floor(((double) (psize - sizeof (SuperBloque)) / ((double) (1 + 3 + sizeof (Inodo) + 3 * 64))));
                        numBloques = 3 * numInodos;
                        bmInodoInicio = iniciop + sizeof (SuperBloque);
                        fsi = 2;
                    } else {
                        numInodos = floor(((double) (psize - sizeof (SuperBloque)) / ((double) (1 + sizeof (Journal) + 3 + sizeof (Inodo) + 3 * 64))));
                        numBloques = 3 * numInodos;
                        inicioJournal = iniciop + sizeof (SuperBloque);
                        bmInodoInicio = inicioJournal + numInodos * (sizeof (Journal));
                        fsi = 3;
                        fjournal = 1;
                    }
                    int bmBloqueInicio = bmInodoInicio + numInodos;
                    int InicioInodo = bmBloqueInicio + numBloques;
                    int InicioBloque = InicioInodo + sizeof (Inodo) * numInodos;
                    time_t times;
                    times = time(NULL);
                    struct tm *tm;
                    tm = localtime(&times);
                    //SuperBloque(int s_filesystem_type_, int s_inodes_count_, int s_blocks_count_, int s_free_blocks_count_, int s_free_inodes_count_, time_t s_mtime,time_t s_umtime,int s_mnt_count_,int s_magic_, int s_inode_size_,int s_block_size_,int s_first_ino_,int s_first_blo_,int s_inode_start_,int s_block_start_);
                    SuperBloque *nuevo = new SuperBloque(fsi, numInodos, numBloques, numBloques, numInodos, times, 0, 1, "0xEF53", sizeof (Inodo), 64, 0, 0, bmInodoInicio, bmBloqueInicio, InicioInodo, InicioBloque);
                    nuevo->s_journal_start = inicioJournal;
                    //if (EscribirArchivo(AuxNode->path, iniciop, nuevo, sizeof (SuperBloque)) == 1)
                    if (EscribirArchivo(AuxNode->path, iniciop, nuevo, sizeof (SuperBloque)) == 0) {
                        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no fue posible escribir el SuperBloque en el mkfs.!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
                        return;
                    }


                    vaciarArchivo(AuxNode->path, bmInodoInicio, InicioInodo, 8000);

                    if (strcasecmp(type, "full") == 0) {
                        vaciarArchivo(AuxNode->path, InicioInodo, InicioBloque + numBloques * 64, 8000);
                    }

                    Bitmap *BmInodos = new Bitmap(numInodos, 0);
                    if (EscribirArchivo(AuxNode->path, bmInodoInicio, BmInodos, BmInodos->size) == 0) {
                        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no fue posible escribir el BMInodos en el mkfs.!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
                        return;
                    }

                    Bitmap *BmBloque = new Bitmap(numBloques, 1);
                    if (EscribirArchivo(AuxNode->path, bmBloqueInicio, BmBloque, BmBloque->size) == 0) {
                        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no fue posible escribir el BMBloques en el mkfs.!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
                        return;
                    }
                    cout << "######################################## Datos de Formateo ####################################### \n";
                    cout << "Tamaño de particion: " << AuxNode->part_size << "\n";
                    cout << "Inicio de particion: " << AuxNode->part_start << "\n";
                    cout << "Inicio de SuperBloque: " << AuxNode->part_start << "\n";
                    cout << "Fin de Superbloque: " << AuxNode->part_start + sizeof (SuperBloque) - 1 << "\n";
                    cout << "Tamaño de SuperBloque: " << sizeof (SuperBloque) << "\n";
                    if (fsi == 3) {
                        cout << "Inicio Journal: " << inicioJournal << "\n";
                        cout << "Fin Journal: " << inicioJournal + numInodos * (sizeof (Journal)) - 1 << "\n";
                        cout << "Tamaño de Journal: " << sizeof (Journal) << "\n";
                    }
                    cout << "Inicio BitMap Inodos: " << bmInodoInicio << "\n";
                    cout << "Fin BitMap Inodos: " << bmBloqueInicio - 1 << "\n";
                    cout << "Inicio BitMap Bloques: " << bmBloqueInicio << "\n";
                    cout << "Fin BitMap Bloques: " << InicioInodo - 1 << "\n";
                    cout << "Inicio de Inodos: " << InicioInodo << "\n";
                    cout << "Fin de Inodos: " << InicioBloque - 1 << "\n";
                    cout << "Número de Inodos: " << numInodos << "\n";
                    cout << "Tamaño de Inodo: " << sizeof (Inodo) << "\n";
                    cout << "Inicio de Bloques: " << InicioBloque << "\n";
                    cout << "Fin de Bloques: " << InicioBloque + numBloques * 64 << "\n";
                    cout << "Número de Blques: " << numBloques << "\n";
                    cout << "Tamaño de Bloque: " << sizeof (BloqueApuntador) << "\n";
                    cout << "Fin de Particion: " << AuxNode->part_start + AuxNode->part_size - 1 << "\n";
                    cout << "-------------------------------------------------------------------------------------------- \n";

                    //crea carpeta root
                    idjournal = 0;
                    Inodo* raiz = new Inodo(1, "root", 0, time(0), time(0), time(0), 0, 664);
                    if (EscribirInodo(raiz, AuxNode, 0) == 0) {
                        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no fue posible crear el Inodo de la carpeta raiz !!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
                        return;
                    }
                    Journal* tp = new Journal();
                    strcpy(tp->comandomkcarpeta.path, "/");
                    strcpy(tp->tipo_operacion, "mkdir");
                    strcpy(tp->tipo, "Carpeta");
                    strcpy(tp->nombre, "/");
                    strcpy(tp->contenido, "...");
                    tp->fecha = time(NULL);
                    EscribirJournal(AuxNode, idjournal, tp);

                    crearBloqueCarpeta(AuxNode, 0, "/");

                    Inodo* usuarios = new Inodo(1, "root", 0, time(0), time(0), time(0), 1, 664);
                    if (EscribirInodo(usuarios, AuxNode, 1) == 0) {
                        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no fue posible crear el Inodo de la carpeta raiz !!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
                        return;
                    }
                    char nusr[12];
                    nusr[0] = 'u';
                    nusr[1] = 's';
                    nusr[2] = 'e';
                    nusr[3] = 'r';
                    nusr[4] = 's';
                    nusr[5] = '.';
                    nusr[6] = 't';
                    nusr[7] = 'x';
                    nusr[8] = 't';
                    nusr[9] = '\0';
                    nusr[10] = '\0';
                    nusr[11] = '\0';
                    CrearArchivo(AuxNode, 0, nusr, 1, 0);
                    BloqueArchivos* usroot = new BloqueArchivos();
                    char user[64] = "1,G,root\n1,U,root,root,123\n";
                    strncpy(usroot->b_content, user, 64);
                    int tama = strlen(user);
                    if (tama > 64) {
                        usuarios->i_size += 64;
                    } else {
                        usuarios->i_size += tama;
                    }

                    Journal* tp1 = new Journal();
                    strcpy(tp1->comandomkarchivo.path, "/users.txt");
                    strcpy(tp1->tipo_operacion, "mkfile");
                    strcpy(tp1->tipo, "Archivo");
                    strcpy(tp1->nombre, "Users.txt");
                    strcpy(tp1->contenido, user);
                    tp1->fecha = time(NULL);
                    idjournal++;
                    EscribirJournal(AuxNode, idjournal, tp1);

                    EscribirContenido(AuxNode, 1, user);
                    FILE * arc = fopen("Users.txt", "w+");
                    char ct[] = "1,G,root\n1,U,root,root,123";
                    fputs(ct, arc);
                    fclose(arc);
                }
            }
        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!! Error No existe ningun Mount Todavía !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
        }
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error falta un parametro obligatorio en Mkfs !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";

    }

}

void analizamkfs(char comando[10000]) {
    int bande1 = 0;
    char *cut = strtok(comando, " ");
    cut = strtok(NULL, " =");
    char *id;
    char *type;
    char *fs = "2fs";
    while (cut != NULL) {
        if (strcasecmp(cut, "-id") == 0) {
            cut = strtok(NULL, "= ");
            bande1 = 1;
            id = cut;
        } else if (strcasecmp(cut, "-type") == 0) {
            cut = strtok(NULL, "= ");
            if ((strcasecmp(cut, "full") == 0) || (strcasecmp(cut, "fast") == 0)) {
                type = cut;
            } else {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error el tipo en mkfs no esta segun lo establecido !!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
            }
        } else if (strcasecmp(cut, "-fs") == 0) {
            cut = strtok(NULL, "= ");
            if ((strcasecmp(cut, "3fs") == 0) || (strcasecmp(cut, "2fs") == 0)) {
                fs = cut;
            } else {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error el tipo de archivo en mkfs no esta segun lo establecido !!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
            }
        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Existe un error en el comando mkfs !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            return;
        }
        cut = strtok(NULL, "= ");
    }
    mkfs(id, type, fs, bande1);

}

void mkgrp(char name[10], int bande) {
    if (bande == 1) {
        if (actual.uid == 1) {

            char linea[1024];
            FILE *archivo = fopen("Users.txt", "r+");
            if (archivo == NULL) {
                perror("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se pudo abrir users.txt !!!!!!!!!!!!!!!!!!!!!!!!\n");
                return;
            }
            int contador = 0;
            int tipo = 0;

            int vv = 0;
            while (fgets(linea, 1024, (FILE*) archivo)) {
                const char separador[2] = ",";
                char *cut = (char*) malloc(4 * sizeof (char*));
                cut = strtok(linea, separador);
                Usuario aux;
                for (int i = 0; i < 10; i++) {
                    aux.nombre[i] = '\0';
                    aux.pass[i] = '\0';
                }
                aux.uid = 0;
                while (cut != NULL) {
                    if (contador == 0) {
                        aux.uid = atoi(cut);
                    } else if (contador == 1) {
                        if (strcmp(cut, "G") == 0)tipo = 1;
                    } else if (contador == 2) {
                        int e = 0;
                        for (int i = 0; i<sizeof (cut); i++) {
                            if (cut[i] == '\n') {

                            } else {
                                aux.gid[i] = cut[i];
                            }
                            e++;
                        }
                        for (int i = e; i < 10; i++) {
                            aux.gid[i] = '\0';
                        }

                    } else if (contador == 3) {
                        int e = 0;
                        for (int i = 0; i<sizeof (cut); i++) {
                            if (cut[i] == '\n') {

                            } else {
                                aux.nombre[i] = cut[i];
                            }
                            e++;
                        }
                        for (int i = e; i < 10; i++) {
                            aux.nombre[i] = '\0';
                        }
                    } else if (contador == 4) {
                        int e = 0;
                        for (int i = 0; i<sizeof (cut); i++) {
                            if (cut[i] == '\n') {

                            } else {
                                aux.pass[i] = cut[i];
                            }
                            e++;
                        }
                        for (int i = e; i < 10; i++) {
                            aux.pass[i] = '\0';
                        }
                    }
                    contador++;
                    cut = strtok(NULL, separador);
                }
                if (tipo == 1) {
                    vv++;
                    if (strcmp(aux.gid, name) == 0) {
                        if (aux.uid != 0) {
                            cout << "!!!!!!!!!!!!!!!!!!!!!Error el grupo ya existe !!!!!!!!!!!!!!!!!!!!!\n";
                            fclose(archivo);
                            return;
                        }
                    }
                }
            }
            fprintf(archivo, "\n%d,G,%s", vv, name);
            fclose(archivo);
            cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>El grupo se creó exitosamente\n";
            if (fjournal == 1) {
                Journal* tempo = new Journal();
                strcpy(tempo->comandomkgroup.name, name);
                strcpy(tempo->tipo_operacion, "mkgrp");
                strcpy(tempo->tipo, "..");
                strcpy(tempo->contenido, "-----");
                strcpy(tempo->nombre, name);
                tempo->fecha = time(NULL);
                idjournal++;
                EscribirJournal(actual.particion, idjournal, tempo);
            }
            return;

        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!Error solo el usuario root puede crear grupos !!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        }
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!! Error falta un parametro obligatorio en mkgrp !!!!!!!!!!!!!!!!!!!!!!!!!!\n";
    }

}

void analizamkgrp(char comando[10000]) {
    int bande1 = 0;
    char *cut = strtok(comando, " ");
    cut = strtok(NULL, " =");
    char *id;
    while (cut != NULL) {
        if (strcasecmp(cut, "-name") == 0) {
            cut = strtok(NULL, "= ");
            bande1 = 1;
            id = cut;
        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Existe un error en el comando mkgrp !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            return;
        }
        cut = strtok(NULL, "= ");
    }
    mkgrp(id, bande1);
}

void rmgrp(char name[10], int bande) {
    if (bande == 1) {
        if (actual.uid == 1) {
            char linea[1024];
            FILE *archivo = fopen("Users.txt", "r+");
            if (archivo == NULL) {
                perror("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se pudo abrir users.txt !!!!!!!!!!!!!!!!!!!!!!!!\n");
                return;
            }
            int contador = 0;
            int tipo = 0;

            while (fgets(linea, 1024, (FILE*) archivo)) {
                int tlinea = strlen(linea);
                const char splt[2] = ",";
                char *cut = (char*) malloc(4 * sizeof (char*));
                cut = strtok(linea, splt);
                contador = 0;
                tipo = 0;
                Usuario aux;
                for (int i = 0; i < 10; i++) {
                    aux.nombre[i] = '\0';
                    aux.pass[i] = '\0';
                }
                aux.uid = 0;
                while (cut != NULL) {
                    if (contador == 0) {
                        aux.uid = atoi(cut);
                    } else if (contador == 1) {
                        if (strcmp(cut, "G") == 0)tipo = 1;
                    } else if (contador == 2) {
                        int e = 0;
                        for (int i = 0; i<sizeof (cut); i++) {
                            if (cut[i] == '\n') {

                            } else {
                                aux.gid[i] = cut[i];
                            }
                            e++;
                        }
                        for (int i = e; i < 10; i++) {
                            aux.gid[i] = '\0';
                        }

                    } else if (contador == 3) {
                        int e = 0;
                        for (int i = 0; i<sizeof (cut); i++) {
                            if (cut[i] == '\n') {

                            } else {
                                aux.nombre[i] = cut[i];
                            }
                            e++;
                        }
                        for (int i = e; i < 10; i++) {
                            aux.nombre[i] = '\0';
                        }
                    } else if (contador == 4) {
                        int e = 0;
                        for (int i = 0; i<sizeof (cut); i++) {
                            if (cut[i] == '\n') {

                            } else {
                                aux.pass[i] = cut[i];
                            }
                            e++;
                        }
                        for (int i = e; i < 10; i++) {
                            aux.pass[i] = '\0';
                        }
                    }
                    contador++;
                    cut = strtok(NULL, splt);
                }
                if (tipo == 1) {
                    if (strcmp(aux.gid, name) == 0) {
                        int posA = ftell(archivo);

                        fseek(archivo, posA - tlinea, SEEK_SET);
                        fputc('0', archivo);
                        fclose(archivo);
                        cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>El grupo se eliminó exitosamente\n";
                        if (fjournal == 1) {
                            Journal* tempo = new Journal();
                            strcpy(tempo->comandormgroup.name, name);
                            strcpy(tempo->tipo_operacion, "rmusr");
                            strcpy(tempo->contenido, "-----");
                            strcpy(tempo->tipo, "..");
                            strcpy(tempo->nombre, name);
                            tempo->fecha = time(NULL);
                            idjournal++;
                            EscribirJournal(actual.particion, idjournal, tempo);
                        }
                        return;
                    }
                }
            }
            cout << "!!!!!!!!!!!!!!!!!!!!!Error no se encontró el grupo que se desea eliminar !!!!!!!!!!!!!!!!!!!!!\n";
            fclose(archivo);
            return;

        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!Error solo el usuario root puede eliminar grupos !!!!!!!!!!!!!!!!!!!!!!!!!!!\n";

        }
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!! Error falta un parametro obligatorio en rmgrp !!!!!!!!!!!!!!!!!!!!!!!!!!\n";
    }
}

void analizarmgrp(char comando[10000]) {
    int bande1 = 0;
    char *cut = strtok(comando, " ");
    cut = strtok(NULL, " =");
    char *id;
    while (cut != NULL) {
        if (strcasecmp(cut, "-name") == 0) {
            cut = strtok(NULL, "= ");
            bande1 = 1;
            id = cut;
        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Existe un error en el comando rmgrp !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            return;
        }
        cut = strtok(NULL, "= ");
    }
    rmgrp(id, bande1);
}

void login(char usr[10], char pwd[10], char id[4], int bande1, int bande2, int bande3) {
    if (bande1 == 1 && bande2 == 1 && bande3 == 1) {
        Mount *AuxNode;
        bool encontro = false;
        if (primero != NULL) {
            AuxNode = primero;
            while (AuxNode != NULL) {
                if (strcasecmp(AuxNode->id, id) == 0) {
                    encontro = true;
                    break;
                }
                AuxNode = AuxNode->next;
            }

            if (!encontro) {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!! Error No se encontró el id de particion buscado, para iniciar sesion!!!!!!!!!!!!!!!!!!!!!!!! \n";
                return;
            } else {
                if (actual.uid != 0) {
                    cout << "!!!!!!!!!!!!!!!!!!!!!!!!! Error ya tiene iniciada sesión !!!!!!!!!!!!!!!!!!!!!!!! \n";
                    return;
                } else {

                    actual.particion = AuxNode;
                    char linea[1024];
                    FILE *archivo = fopen("Users.txt", "r");
                    if (archivo == NULL) {
                        perror("Error al abrir fichero");
                        return;
                    }
                    int contador = 0;
                    int tipo = 0;

                    while (fgets(linea, 1024, (FILE*) archivo)) {

                        const char splt[2] = ",";
                        char *cut = (char*) malloc(4 * sizeof (char*));
                        cut = strtok(linea, splt);
                        contador = 0;
                        tipo = 0;
                        Usuario aux;
                        for (int i = 0; i < 10; i++) {
                            aux.nombre[i] = '\0';
                            aux.gid[i] = '\0';
                            aux.pass[i] = '\0';
                        }
                        aux.particion = AuxNode;
                        aux.uid = 0;
                        while (cut != NULL) {
                            if (contador == 0) {
                                aux.uid = atoi(cut);
                            } else if (contador == 1) {
                                if (strcmp(cut, "U") == 0)tipo = 1;
                            } else if (contador == 2) {
                                int e = 0;
                                for (int i = 0; i<sizeof (cut); i++) {
                                    if (cut[i] == '\n') {

                                    } else {
                                        aux.gid[i] = cut[i];
                                    }
                                    e++;
                                }
                                for (int i = e; i < 10; i++) {
                                    aux.gid[i] = '\0';
                                }

                            } else if (contador == 3) {
                                int e = 0;
                                for (int i = 0; i<sizeof (cut); i++) {
                                    if (cut[i] == '\n') {

                                    } else {
                                        aux.nombre[i] = cut[i];
                                    }
                                    e++;
                                }
                                for (int i = e; i < 10; i++) {
                                    aux.nombre[i] = '\0';
                                }
                            } else if (contador == 4) {
                                int e = 0;
                                for (int i = 0; i<sizeof (cut); i++) {
                                    if (cut[i] == '\n') {

                                    } else {
                                        aux.pass[i] = cut[i];
                                    }
                                    e++;
                                }
                                for (int i = e; i < 10; i++) {
                                    aux.pass[i] = '\0';
                                }
                            }
                            contador++;
                            cut = strtok(NULL, splt);
                        }

                        if (tipo == 1) {
                            if (strcmp(aux.nombre, usr) == 0) {
                                if (strcmp(aux.pass, pwd) == 0) {
                                    actual = aux;
                                    cout << ">>>>>>>>>>>>>>>>>>>>>> Se inició sesión con éxito \n";
                                    fclose(archivo);
                                    return;
                                }
                            }
                        }
                    }
                    cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error Usuario o contraseña incorrectos!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
                    fclose(archivo);
                    return;
                }
            }
        }
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error falta un parametro obligatorio !!!!!!!!!!!!!!!!!!!!!!!!";
    }
}

void analizaLogin(char comando[10000]) {
    int bande1 = 0;
    int bande2 = 0;
    int bande3 = 0;
    char *cut = strtok(comando, " ");
    cut = strtok(NULL, " =");
    char *id;
    char *pwd;
    char *usr;
    while (cut != NULL) {
        if (strcasecmp(cut, "-id") == 0) {
            cut = strtok(NULL, "= ");
            bande1 = 1;
            id = cut;
        } else if (strcasecmp(cut, "-usr") == 0) {
            cut = strtok(NULL, "= ");
            bande2 = 1;
            usr = cut;
        } else if (strcasecmp(cut, "-pwd") == 0) {
            cut = strtok(NULL, "= ");
            bande3 = 1;
            pwd = cut;
        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Existe un error en el comando Login !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            return;
        }
        cut = strtok(NULL, "= ");
    }
    login(usr, pwd, id, bande1, bande2, bande3);
}

void RBArchivo(Mount* particion, int id, FILE * repo) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinicio = sb->s_block_start + id * sizeof (BloqueArchivos);
    BloqueArchivos *bloque = (BloqueArchivos*) readDisk(particion->path, pinicio, sizeof (BloqueArchivos));
    char *txto = (char*) malloc(sizeof (char)*64);
    strncpy(txto, bloque->b_content, 64);
    fprintf(repo, "%.64s", txto);
}

void FileSimple(Mount* particion, int id, FILE * repo) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pbloque = sb->s_block_start + id * sizeof (BloqueApuntador);
    BloqueApuntador *bloque = (BloqueApuntador*) readDisk(particion->path, pbloque, sizeof (BloqueApuntador));

    for (int i = 0; i < 16; i++) {
        if (bloque->b_pointers[i] > -1) {
            RBArchivo(particion, bloque->b_pointers[i], repo);
        }
    }
}

void FileDoble(Mount* particion, int id, FILE * repo) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pbloque = sb->s_block_start + id * sizeof (BloqueApuntador);
    BloqueApuntador *bloque = (BloqueApuntador*) readDisk(particion->path, pbloque, sizeof (BloqueApuntador));

    for (int i = 0; i < 16; i++) {
        if (bloque->b_pointers[i] > -1) {
            FileSimple(particion, bloque->b_pointers[i], repo);
        }
    }
}

void FileTriple(Mount* particion, int id, FILE * repo) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pbloque = sb->s_block_start + id * sizeof (BloqueApuntador);
    BloqueApuntador *bloque = (BloqueApuntador*) readDisk(particion->path, pbloque, sizeof (BloqueApuntador));

    for (int i = 0; i < 16; i++) {
        if (bloque->b_pointers[i] > -1) {
            FileTriple(particion, bloque->b_pointers[i], repo);
        }
    }
}

void imprimecat(Mount *particion, int id, FILE * archivo) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinodo = sb->s_inode_start + id * sizeof (Inodo);
    Inodo *inodoArchivo = (Inodo*) readDisk(particion->path, pinodo, sizeof (Inodo));

    for (int i = 0; i < 12; i++) {
        if (inodoArchivo->i_block[i] == -1)
            continue;
        RBArchivo(particion, inodoArchivo->i_block[i], archivo);
    }
    for (int i = 12; i < 15; i++) {
        if (inodoArchivo->i_block[i] == -1) {
            continue;
        }
        if (i == 12) {
            FileSimple(particion, inodoArchivo->i_block[i], archivo);
        } else if (i == 13) {
            FileDoble(particion, inodoArchivo->i_block[i], archivo);
        } else if (i == 14) {
            FileTriple(particion, inodoArchivo->i_block[i], archivo);
        }
    }
}

void Cat(char path[500]) {
    int id = idCarpeta(actual.particion, path);
    if (id>-1) {
        SuperBloque* sb = BuscarSuperBloque(actual.particion);
        int pinodo = sb->s_inode_start + id * sizeof (Inodo);
        Inodo *inodoArchivo = (Inodo*) readDisk(actual.particion->path, pinodo, sizeof (Inodo));
        if (actual.uid == 1) {
            imprimecat(actual.particion, id, stdout);
            cout << "\n";
        } else {
            int inpermiso = inodoArchivo->i_perm;
            int pusuario = inpermiso / 100;
            int pgrupo = inpermiso / 10 % 10;
            int potros = inpermiso % 10;

            if (actual.uid == inodoArchivo->i_uid) {
                if (pusuario > 3) {
                    imprimecat(actual.particion, id, stdout);
                    cout << "\n";
                }
            } else if (strcasecmp(actual.gid, inodoArchivo->i_gid) == 0) {
                if (pgrupo > 3) {
                    imprimecat(actual.particion, id, stdout);
                    cout << "\n";
                }
            } else {
                if (potros > 3) {
                    imprimecat(actual.particion, id, stdout);
                    cout << "\n";
                }
            }
        }
    }
}

void analizaCat(char comando[10000]) {
    int bande1 = 0;
    char *cut = strtok(comando, " ");
    cut = strtok(NULL, " =");
    char *file;
    while (cut != NULL) {
        if (strcasecmp(cut, "-file") == 0) {
            cut = strtok(NULL, "= ");
            bande1 = 1;
            if (*cut != '\"') {
                file = cut;
            } else {
                file = quitaCm(cut);
            }
        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Existe un error en el comando Login !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            return;
        }
        cut = strtok(NULL, "= ");
    }
    Cat(file);
}

void Loss(char idp[10]) {
    Mount *TempoNode;
    TempoNode = primero;
    while (TempoNode != NULL) {
        if (strcasecmp(TempoNode->id, idp) == 0) {
            break;
        }
        TempoNode = TempoNode->next;
    }

    if (TempoNode != NULL) {
        mkfs(TempoNode->id, "fast", "3", 1);
        cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>> Se realizó la simulación de pérdida";

    }

}

void analizaLoss(char comando[10000]) {
    int bande1 = 0;
    char *cut = strtok(comando, " ");
    cut = strtok(NULL, " =");
    char *idp;
    while (cut != NULL) {
        if (strcasecmp(cut, "-id") == 0) {
            cut = strtok(NULL, "= ");
            idp = cut;
        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Existe un error en el comando Loss !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            return;
        }
        cut = strtok(NULL, "= ");
    }
    Loss(idp);
}

void Recovery(char idp[10]) {
    Mount *TempoNode;
    TempoNode = primero;
    while (TempoNode != NULL) {
        if (strcasecmp(TempoNode->id, idp) == 0) {
            break;
        }
        TempoNode = TempoNode->next;
    }


    if (TempoNode != NULL) {
        SuperBloque* sb = BuscarSuperBloque(TempoNode);
        int idactual = 2;
        int pjournal = sb->s_journal_start + idactual * sizeof (Journal);
        Journal *aux = (Journal*) readDisk(TempoNode->path, pjournal, sizeof (Journal));
        while (aux->tipo_operacion[0] != '\0')
            if (strcasecmp(aux->tipo_operacion, "mkusr") == 0) {
                mkusr(aux->comandomkusr.usuario, aux->comandomkusr.pass, aux->comandomkusr.grupo, 1, 1, 1);
                idactual++;
                pjournal = sb->s_journal_start + idactual * sizeof (Journal);
                aux = (Journal*) readDisk(TempoNode->path, pjournal, sizeof (Journal));
            } else if (strcasecmp(aux->tipo_operacion, "rmusr") == 0) {
                rmusr(aux->comandormuser.usuario, 1);
                idactual++;
                pjournal = sb->s_journal_start + idactual * sizeof (Journal);
                aux = (Journal*) readDisk(TempoNode->path, pjournal, sizeof (Journal));
            } else if (strcasecmp(aux->tipo_operacion, "mkgrp") == 0) {
                mkgrp(aux->comandomkgroup.name, 1);
                idactual++;
                pjournal = sb->s_journal_start + idactual * sizeof (Journal);
                aux = (Journal*) readDisk(TempoNode->path, pjournal, sizeof (Journal));
            } else if (strcasecmp(aux->tipo_operacion, "rmgrp") == 0) {
                rmgrp(aux->comandormgroup.name, 1);
                idactual++;
                pjournal = sb->s_journal_start + idactual * sizeof (Journal);
                aux = (Journal*) readDisk(TempoNode->path, pjournal, sizeof (Journal));
            } else if (strcasecmp(aux->tipo_operacion, "mkdir") == 0) {
                EjecutaMkdir(aux->comandomkcarpeta.path, 1, aux->comandomkcarpeta.p);
                idactual++;
                pjournal = sb->s_journal_start + idactual * sizeof (Journal);
                aux = (Journal*) readDisk(TempoNode->path, pjournal, sizeof (Journal));
            } else if (strcasecmp(aux->tipo_operacion, "mkfile") == 0) {
                mkfile(aux->comandomkarchivo.path, aux->comandomkarchivo.size, aux->comandomkarchivo.cont, 1, aux->comandomkarchivo.p);
                idactual++;
                pjournal = sb->s_journal_start + idactual * sizeof (Journal);
                aux = (Journal*) readDisk(TempoNode->path, pjournal, sizeof (Journal));
            }

    }
}

void analizaRecovery(char comando[10000]) {
    int bande1 = 0;
    char *cut = strtok(comando, " ");
    cut = strtok(NULL, " =");
    char *idp;
    while (cut != NULL) {
        if (strcasecmp(cut, "-id") == 0) {
            cut = strtok(NULL, "= ");
            idp = cut;
        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Existe un error en el comando Loss !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            return;
        }
        cut = strtok(NULL, "= ");
    }
    Recovery(idp);
}

int existeDisco(char path[1000]) {
    FILE *fp = NULL;
    if ((fp = fopen(path, "r"))) {
        fclose(fp);
        return 1;
    } else {
        return 0;
    }
}

int llenarArchivo(char ruta[500], int tamanio, int bff) {

    FILE *disco = NULL;
    disco = fopen(ruta, "w+b");
    if (!existeDisco(ruta)) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error No se puede abrir el disco " << ruta << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
        return 0;
    }
    if (disco == NULL) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! No fue posible crear el archivo en " << ruta << " !!!!!!!!!!!!!!!!!!!!!!!!\n";
        return 0;
    }

    char *data;
    int bf = 0;

    //por bloques de buffer
    while (tamanio > 0) {
        bf = tamanio;
        if (bf <= bff) {
            data = (char*) malloc(bf);
            tamanio = 0;
        } else {
            data = (char*) malloc(bff);
            bf = bff;
            tamanio = tamanio - bff;
        }
        for (int i = 0; i < bf; i++) {
            data[i] = '\0';
        }
        fwrite(data, bf, 1, disco);
        free(data);
    }/*
    char data = '\0';
    for (int i = 0; i < tamanio; i++) {

        fwrite(&data, 1, 1, disco);
    }
    // fwrite(d, bf, 1, disco);*/

    fclose(disco);
    return 1;
}

int signa = 0;

void crear(char dir[500], int vsize, char vfit, char dir2 [500]) {

    time_t times;
    times = time(NULL);
    struct tm *tm;
    tm = localtime(&times);
    signa++;
    char date[16] = "";
    MBR auxmbr;

    if (existeDisco(dir)) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error ya existe un disco con ese nombre !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
        return;
    }
    auxmbr.mbr_tamano = vsize;
    auxmbr.mbr_disk_signature = signa;
    auxmbr.disk_fit = vfit;
    //Empieza la parti

    auxmbr.array_particion[0].estado = 'I'; //inactiva a= activa
    auxmbr.array_particion[0].inicio = -1;
    auxmbr.array_particion[0].tamano = 0;
    char dia[2];
    sprintf(dia, "%d", tm->tm_mday);
    strcat(auxmbr.mbr_fecha_creacion, dia);
    strcat(auxmbr.mbr_fecha_creacion, "/");
    strcat(auxmbr.mbr_fecha_creacion, "2");
    strcat(auxmbr.mbr_fecha_creacion, "/");
    strcat(auxmbr.mbr_fecha_creacion, "2020");
    strcat(auxmbr.mbr_fecha_creacion, " Hora: ");
    char hora[2];
    sprintf(hora, "%d", tm->tm_hour);
    strcat(auxmbr.mbr_fecha_creacion, hora);

    auxmbr.array_particion[1].estado = 'I';
    auxmbr.array_particion[1].inicio = -1;
    auxmbr.array_particion[1].tamano = 0;

    auxmbr.array_particion[2].estado = 'I';
    auxmbr.array_particion[2].inicio = -1;
    auxmbr.array_particion[2].tamano = 0;


    auxmbr.array_particion[3].estado = 'I';
    auxmbr.array_particion[3].inicio = -1;
    auxmbr.array_particion[3].tamano = 0;


    if (sizeof (MBR) <= vsize) {
        llenarArchivo(dir, vsize, 80000);
        llenarArchivo(dir2, vsize, 80000);
        if (EscribirArchivo(dir, 0, &auxmbr, sizeof (MBR)) == 1) {
            if (EscribirArchivo(dir2, 0, &auxmbr, sizeof (MBR))) {

                cout << "######################################## Disco Nuevo ####################################### \n";
                cout << "Se ha creado el disco en " << dir << "\n";
                cout << "Tamaño en Bytes: " << vsize << "\n";
                cout << "Tipo de Ajuste: " << vfit << "\n";
                cout << "Disk Signature: " << signa << "\n";
                cout << "-------------------------------------------------------------------------------------------- \n";

            }
        }
    }
}

void existeCarpeta(char path[1000]) {
    int contador = 0;
    char *directorio = dirname(strdup(path));
    DIR *carpeta = opendir(directorio);
    size_t len;
    if (carpeta) {
        closedir(carpeta);
    } else if (ENOENT == errno) {
        char tempo[500];
        char *p = NULL;
        snprintf(tempo, sizeof (tempo), "%s", directorio);
        len = strlen(tempo);
        //se separa por slash
        if (tempo[len - 1] == '/') {
            tempo[len - 1] = 0;
        }
        for (p = tempo + 1; *p; p++) {
            if (*p == '/') {
                *p = 0;
                if (mkdir(tempo, S_IRWXU) == 0) {
                    //cout<<"Se creó la carpeta "<<tempo<<"\n";
                }
                *p = '/';
            }
        }
        if (mkdir(tempo, S_IRWXU) == 0) {
            cout << ">>>Se creó el la carpeta exitosamente " << tempo << "\n";
        }
    }
}

void crearDisco(int vsize, char vunit, char vfit, char vpath[500], int bande1, int bande2) {

    int varsize = 0;
    char temporarl[500] = "";
    strncpy(temporarl, vpath, 500);
    char tem[50] = "";
    if (bande1 == 1 && bande2 == 1) {
        if (vunit == 'k') {
            varsize = vsize * 1024;
        } else {
            varsize = vsize * 1024 * 1024;
        }
        existeCarpeta(temporarl);
        char *tempo1 = strdup(vpath);
        char *tempo2 = strdup(vpath);
        char *pathc = dirname(tempo2);
        char *NDisco = basename(tempo1);
        char aux1[50] = "";
        strcat(aux1, NDisco);
        char *auxDisco = strtok(aux1, ".");
        char raidn[500] = "";
        strcat(raidn, pathc);
        strcat(raidn, "/");
        strcat(raidn, aux1);
        strcat(raidn, "_v1.dk");
        crear(temporarl, varsize, vfit, raidn);
        //aux1="";
        //  cout<<"el nombre del disco es "<<auxnombre<<"\n";
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error falta un parametro obligatorio en Mkdisk !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
    }
}

void analizaMkdisk(char comando[10000]) {
    int bande1 = 0;
    int bande2 = 0;
    char *cut = strtok(comando, " ");
    cut = strtok(NULL, " =");
    int varsize = 0;
    char varunit = 'M';
    char varfit = 'F';
    char* varpath = "";
    while (cut != NULL) {
        if (strcasecmp(cut, "-size") == 0) {
            cut = strtok(NULL, "= ");
            bande1 = 1;
            varsize = atoi(cut);
        } else if (strcasecmp(cut, "-u") == 0) {
            cut = strtok(NULL, "= ");
            if (strcasecmp(cut, "K") == 0) {
                varunit = 'k';
            } else if (strcasecmp(cut, "m") == 0) {
                varunit = 'M';
            } else {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error en el tipo de dato \"unit\" en el comando Mkdisk !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
                return;
            }
        } else if (strcasecmp(cut, "-f") == 0) {
            cut = strtok(NULL, "= ");
            varfit = cut[0];
        } else if (strcasecmp(cut, "-path") == 0) {
            cut = strtok(NULL, "= ");
            bande2 = 1;
            if (*cut != '\"') {
                varpath = cut;
            } else {
                varpath = quitaCm(cut);
            }
        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Existe un error en el comando mkdisk !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            return;
        }
        cut = strtok(NULL, "= ");
    }
    crearDisco(varsize, varunit, varfit, varpath, bande1, bande2);
}

void analizaRmdisk(char comando[10000]) {
    char *cut = strtok(comando, " ");
    cut = strtok(NULL, " =");
    if (strcasecmp(cut, "-path") == 0) {
        cut = strtok(NULL, "= ");
        char aux[1000];
        if (*cut != '\"') {
            //cout<<"La ruta del path es: "<<cut<<"\n";
            if (!existeDisco(cut)) {
                cout << "!!!!!!!!!!!!!!!!!!!! Error el disco " << cut << " no existe no se puede ejecutar el comando rmdisk !!!!!!!!!!!!!!!!!!";
                return;
            }
            char vpath[500] = "";
            strcat(vpath, cut);
            char *tempo1 = strdup(vpath);
            char *tempo2 = strdup(vpath);
            char *pathc = dirname(tempo2);
            char *NDisco = basename(tempo1);
            char aux1[50] = "";
            strcat(aux1, NDisco);
            char *auxDisco = strtok(aux1, ".");
            char raidn[500] = "";
            strcat(raidn, pathc);
            strcat(raidn, "/");
            strcat(raidn, aux1);
            strcat(raidn, "_v1.dk");
            char cadena[3] = "";
            cout << "Desea eliminar el disco " << NDisco << " (y / n) \n";
            scanf("%c", cadena);
            while (getchar() != '\n');
            if (cadena[0] == 'y' || cadena[0] == 'Y') {
                if (remove(cut) == 0) {
                    if (remove(raidn) == 0) {
                        cout << "\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ El disco " << NDisco << " se elimino con éxito \\\\\\\\\\\\\\\\\\\\\\\\\" \n";
                    } else {
                        cout << "!!!!!!!! Error no se pudo eliminar el raid de " << NDisco << " !!!!!!!!!!!!!!!!!!!! \n";
                    }
                } else {
                    cout << "!!!!!!!!!!!! Error no se pudo eliminar el disco !!!!!!!!!!!!!!!!!!!!!!! \n";
                }
            } else {
                cout << ">>>>>>>>>>>>>>>>>>>>>>Se canceló la eliminación del disco " << NDisco << " \n";
            }
        } else {
            char *tempo;
            tempo = quitaCm(cut);
            char vpath[500] = "";
            strcat(vpath, tempo);
            char *tempo1 = strdup(vpath);
            char *tempo2 = strdup(vpath);
            char *pathc = dirname(tempo2);
            char *NDisco = basename(tempo1);
            char aux1[50] = "";
            strcat(aux1, NDisco);
            char *auxDisco = strtok(aux1, ".");
            char raidn[500] = "";
            strcat(raidn, pathc);
            strcat(raidn, "/");
            strcat(raidn, aux1);
            strcat(raidn, "_v1.dk");
            if (remove(tempo) == 0) {
                if (remove(raidn) == 0) {
                    cout << "\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ El disco " << NDisco << " se elimino con éxito \\\\\\\\\\\\\\\\\\\\\\\\\" \n";
                } else {
                    cout << "!!!!!!!! Error no se pudo eliminar el raid de " << NDisco << " !!!!!!!!!!!!!!!!!!!! \n";
                }
            } else {
                cout << "!!!!!!!!!!!! Error no se pudo eliminar el disco !!!!!!!!!!!!!!!!!!!!!!! \n";
            }
        }
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!! Error no viene el parametro path, en el comando Rmdisk !!!!!!!!!!!!!!!!!! \n";
    }
}

int peorAjuste(MBR *mbr, int tamano) {
    int pinicio = -1;
    int ptama = 0;
    int res = 0;
    int iniciotempo = sizeof (MBR);
    int minicio = -1;
    int res2 = -1;

    int contador = 0;
    for (int i = 0; i < 4; i++) {
        if (mbr->array_particion[i].inicio > -1) {
            contador++;
        }
    }

    for (int i = 0; i < 4; i++) {
        if (mbr->array_particion[i].estado == 'A') {
            pinicio = mbr->array_particion[i].inicio;
            ptama = mbr->array_particion[i].tamano;
            res = pinicio - iniciotempo;

            if (res >= tamano) {
                if (i == 0) {
                    minicio = iniciotempo;
                    res2 = res;
                } else {
                    if (res2 == -1 || res > res2) {
                        minicio = iniciotempo;
                        res2 = res;
                    }
                }
            }

            iniciotempo = pinicio + ptama;
        }
    }

    res = mbr->mbr_tamano - iniciotempo;

    if ((res2 == -1 && res >= tamano) || (res >= tamano && res >= res2)) {
        minicio = iniciotempo;
        res2 = res;
    }

    if (res2 > -1) {
        return minicio;
    } else {
        return -1;
    }
}

int primerAjuste(MBR *mbr, int tamano) {
    int pinicio = -1;
    int ptama = 0;
    int iniciotempo = sizeof (MBR);

    int contador = 0;

    for (int i = 0; i < 4; i++) {
        if (mbr->array_particion[i].estado == 'A') {
            pinicio = mbr->array_particion[i].inicio;
            ptama = mbr->array_particion[i].tamano;

            if ((pinicio - iniciotempo) >= tamano) {
                break;
            }
            iniciotempo = pinicio + ptama;
        }
    }

    if (iniciotempo <= (mbr->mbr_tamano - tamano)) {
        return iniciotempo;
    } else {
        return -1;
    }
}

int mejorAjuste(MBR *mbr, int tamano) {
    int res = 0;
    int res2 = -1;
    int iniciotempo = sizeof (MBR);
    int auxtempo = -1;
    int pinicio = -1;
    int ptama = 0;

    for (int i = 0; i < 4; i++) {

        if (mbr->array_particion[i].estado == 'A') {
            pinicio = mbr->array_particion[i].inicio;
            ptama = mbr->array_particion[i].tamano;

            res = pinicio - iniciotempo;

            if (res >= tamano) {
                if (i == 0) {
                    auxtempo = iniciotempo;
                    res2 = res;
                } else {
                    if (res2 == -1 || res <= res2) {
                        auxtempo = iniciotempo;
                        res2 = res;
                    }
                }
            }

            iniciotempo = pinicio + ptama;
        }
    }

    res = mbr->mbr_tamano - iniciotempo;

    if ((res2 == -1 && res >= tamano) || (res >= tamano && res <= res2)) {
        auxtempo = iniciotempo;
        res2 = res;
    }

    if (res2 > -1) {
        return auxtempo;
    } else {
        return -1;
    }
}

int getPosicion(MBR* mbr, int tamano, char fit) {
    if ((mbr->mbr_tamano - (signed)sizeof (MBR)) < tamano)
        return -1;
    if (fit == 'w' || fit == 'W') {
        return peorAjuste(mbr, tamano);
    } else if (fit == 'f' || fit == 'F') {
        return primerAjuste(mbr, tamano);
    } else {
        return mejorAjuste(mbr, tamano);
    }
    return -2;
}

void reordenarPart(MBR * mbr) {
    Parti aux;
    for (int i = 1; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            if (mbr->array_particion[j + 1].inicio>-1) {

                if (mbr->array_particion[j].inicio > mbr->array_particion[j + 1].inicio) {
                    aux = mbr->array_particion[j];
                    mbr->array_particion[j] = mbr->array_particion[j + 1];
                    mbr->array_particion[j + 1] = aux;
                }
            }
        }
    }
}

int agregarMBR(MBR *mbr, Parti * partition) {
    for (int i = 0; i < 4; i++) {
        if (mbr->array_particion[i].inicio == -1) {
            mbr->array_particion[i] = *partition;
            reordenarPart(mbr);
            return 1;
        }
    }
    return 0;
}

void crearParticionPrimaria(char ruta [500], MBR *tempo, Parti *Particion, MBR *traid, char ruta2[500]) {
    int inicio = getPosicion(tempo, Particion->tamano, tempo->disk_fit);

    if (inicio == -1) {
        cout << "!!!!!!!!!!!!!!!!! Error No se encontró un espacio libre del tamaño de la partición dentro del disco!\n";
        return;
    }
    Parti *tempo2 = new Parti('A', Particion->tipo, Particion->fit, inicio, Particion->tamano, Particion->pname);

    agregarMBR(tempo, tempo2);
    agregarMBR(traid, tempo2);
    if (EscribirArchivo(ruta, 0, tempo, sizeof (MBR)) == 1) {
        if (EscribirArchivo(ruta2, 0, traid, sizeof (MBR)) == 1) {
            cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>> La partición primaria " << Particion->pname << " fue creada con éxito! \n";
            return;
        }
    } else {
        cout << "!!!!!!! Error: No fue posible guardar la particion " << Particion->pname << " en el disco! !!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
    }
}

int EscribirEBR(MBR *mbr, EBR *ebr, char path[500], int inicio, Parti * extendida) {

    if (inicio < extendida->inicio) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error Se intentó escribir el EBR antes del inicio de la partición extendida " << extendida->pname << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
        return 0;
    }

    int fin = extendida->inicio + extendida->tamano - 1;
    int prueba;
    if (fin < mbr->mbr_tamano) {
        prueba = fin;
    } else {
        prueba - 1;
    }

    if (inicio > prueba) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error Se intentó escribir el EBR después del final de la partición extendida " << extendida->pname << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
        return 0;
    }
    if (EscribirArchivo(path, inicio, ebr, sizeof (EBR)) == 1) {
        return 1;
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error No fue posible guardar el EBR en el disco! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        return 0;
    }
}

void crearParticionExtendida(char ruta [500], MBR *tempo, Parti *Particion, MBR* traid, char ruta2[500]) {
    int inicio = getPosicion(tempo, Particion->tamano, tempo->disk_fit);

    if (inicio == -1) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!! Error No se encontró un espacio libre del tamaño de la partición dentro del disco !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        return;
    }
    Parti *tempo2 = new Parti('A', Particion->tipo, Particion->fit, inicio, Particion->tamano, Particion->pname);

    agregarMBR(tempo, tempo2);
    agregarMBR(traid, tempo2);
    EBR *ebr = new EBR('A', Particion->fit, inicio, 0, -1, Particion->pname);
    if (EscribirArchivo(ruta, 0, tempo, sizeof (MBR)) == 1) {
        if (EscribirArchivo(ruta2, 0, traid, sizeof (MBR)) == 1) {
            if (EscribirEBR(tempo, ebr, ruta, inicio, tempo2)) {
                if (EscribirEBR(traid, ebr, ruta2, inicio, tempo2)) {
                    cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>> La partición extendida fue creada con éxito! \n";
                    return;
                }
            } else {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error No fue posible guardar el EBR de la partición extendida en el disco !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
                return;
            }
        }
    }
}

EBR * siguienteEBR(char ruta_disk[500], EBR * previousEBR) {
    if (previousEBR == NULL || previousEBR->next == -1) {
        return NULL;
    }
    return (EBR*) readDisk(ruta_disk, previousEBR->next, sizeof (EBR));
}

EBR * primerEBR(char ruta_disk[500], Parti * extendida) {
    EBR *ebr = (EBR*) readDisk(ruta_disk, extendida->inicio, sizeof (EBR));
    if (ebr == NULL) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se puede recuperar el primer EBR de " << extendida->pname << " !!!!!!!!!!!!!!!!!!!!!!!!!!!";
        return NULL;
    }
    return ebr;
}

EBR * ultimoEBR(EBR *primero, char ruta[500], Parti * extendida) {
    EBR *anterior = primero;
    EBR *siguiente = NULL;
    while ((siguiente = siguienteEBR(ruta, anterior)) != NULL) {
        anterior = siguiente;
    }
    return anterior;
}

EBR * anteriorEBR(char ruta[500], int inicio, EBR * primero) {
    if (inicio == primero->inicio) {
        return NULL;
    }
    EBR *anterior = primero;
    EBR *siguiente = NULL;
    while (((siguiente = siguienteEBR(ruta, anterior)) != NULL) && siguiente->inicio < inicio) {
        anterior = siguiente;
    }
    return anterior;
}

EBR * buscarEBR(char ruta[500], EBR* primero, char *nombre) {

    EBR *anterior = primero;
    EBR *siguiente = NULL;

    if (strcasecmp(primero->lname, nombre) == 0)
        return primero;

    while ((siguiente = siguienteEBR(ruta, anterior)) != NULL) {
        if (strcasecmp(siguiente->lname, nombre) == 0) {
            return siguiente;
        }
        anterior = siguiente;
    }
    return siguiente;

}

int peorAjusteL(char ruta[500], Parti *extendida, int tamano) {

    int extendidaEnd = extendida->inicio + extendida->tamano;
    int iniciotempo = extendida->inicio;

    int res3 = -1;
    int pinicio = -1;
    int ptama = 0;
    int res = 0;
    int res2 = -1;

    EBR *anterior = primerEBR(ruta, extendida);
    if (anterior->tamano > (signed) sizeof (EBR) && anterior->next >= -1)
        iniciotempo = iniciotempo + anterior->tamano;

    EBR *siguiente = NULL;
    int i = 0;
    while ((siguiente = siguienteEBR(ruta, anterior)) != NULL) {
        pinicio = siguiente->inicio;
        ptama = siguiente->tamano;
        res = pinicio - iniciotempo;
        if (res >= tamano) {
            if (i == 0) {
                res3 = iniciotempo;
                res2 = res;
            } else {
                if (res2 == -1 || res >= res2) {
                    res3 = iniciotempo;
                    res2 = res;
                }
            }
        }
        iniciotempo = pinicio + ptama;
        anterior = siguiente;
        i++;
    }

    res = extendidaEnd - iniciotempo;

    if ((res2 == -1 && res >= tamano) || (res >= tamano && res >= res2)) {
        res3 = iniciotempo;
        res2 = res;
    }

    if (res2 > -1) {
        return res3;
    } else {
        return -1;
    }

}

int primerAjusteL(char ruta[500], Parti *extendida, int tamano) {
    int pinicio = -1;
    int ptama = 0;
    int extfin = extendida->inicio + extendida->tamano;
    int iniciotempo = extendida->inicio;

    EBR* anterior = primerEBR(ruta, extendida);
    if (anterior->tamano > (signed) sizeof (EBR) && anterior->next >= -1)
        iniciotempo = iniciotempo + anterior->tamano;

    EBR* siguiente = NULL;

    while ((siguiente = siguienteEBR(ruta, anterior)) != NULL) {

        pinicio = siguiente->inicio;
        ptama = siguiente->tamano;

        if ((pinicio - iniciotempo) >= tamano) {
            break;
        }
        iniciotempo = pinicio + ptama;
        anterior = siguiente;
    }

    if (iniciotempo <= (extfin - tamano)) {
        return iniciotempo;
    } else {
        return -1;
    }

}

int mejorAjusteL(char ruta[500], Parti *extendida, int nuevoT) {
    int pinicio = -1;
    int ptama = 0;
    int extfin = extendida->inicio + extendida->tamano;
    int iniciotempo = extendida->inicio;
    int res3 = -1;
    int res = 0;
    int res2 = -1;

    EBR* anterior = primerEBR(ruta, extendida);
    if (anterior->tamano > (signed) sizeof (EBR) && anterior->next >= -1) {
        iniciotempo = iniciotempo + anterior->tamano;
    }

    EBR* siguiente = NULL;
    int i = 0;
    while ((siguiente = siguienteEBR(ruta, anterior)) != NULL) {
        pinicio = siguiente->inicio;
        ptama = siguiente->tamano;
        res = pinicio - iniciotempo;
        if (res >= nuevoT) {
            if (i == 0) {
                res3 = iniciotempo;
                res2 = res;
            } else {
                if (res2 == -1 || res <= res2) {
                    res3 = iniciotempo;
                    res2 = res;
                }
            }
        }
        iniciotempo = pinicio + ptama;
        anterior = siguiente;
        i++;
    }

    res = extfin - iniciotempo;

    if ((res2 == -1 && res >= nuevoT) || (res >= nuevoT && res <= res2)) {
        res3 = iniciotempo;
        res2 = res;
    }

    if (res2 > -1) {
        return res3;
    } else {
        return -1;
    }

}

int getPosicionL(char ruta[500], Parti *extendida, int tamano, char fit) {

    if ((extendida->tamano) < tamano) {
        return -1;
    }

    int iniciop = -1;
    if (fit == 'w' || fit == 'W') {
        iniciop = peorAjusteL(ruta, extendida, tamano);
    }
    if (fit == 'f' || fit == 'F') {
        iniciop = primerAjusteL(ruta, extendida, tamano);
    }
    if (fit == 'b' || fit == 'B') {
        iniciop = mejorAjusteL(ruta, extendida, tamano);
    }
    return iniciop;

}

void crearLogica(char ruta [500], MBR *tempo, Parti *Particion, Parti *Extendida, MBR *traid, char ruta2[500]) {
    EBR* primero = (EBR*) readDisk(ruta, Extendida->inicio, sizeof (EBR));
    if (primero == NULL) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se puede recuperar el primer EBR de " << Extendida->pname << " !!!!!!!!!!!!!!!!!!!!!!!!!!!";
        return;
    }
    int inicio = 0;
    int creado = 0;

    if (buscarEBR(ruta, primero, Particion->pname) != NULL) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error la partición lógica con nombre " << Particion->pname << " ya existe !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
        return;
    }

    inicio = getPosicionL(ruta, Extendida, Particion->tamano, tempo->disk_fit);
    if (inicio == -1) {
        cout << "!!!!!!!!!!!!!!!!!!! Error ya no hay un espacio en la partición extendida para guardar la partición lógica " << Particion->pname << " !!!!!!!!!!!!!\n";
        return;
    }

    EBR *anterior = anteriorEBR(ruta, inicio, primero);
    EBR *siguiente = siguienteEBR(ruta, anterior);

    int siguienteInicio = -1;

    if (primero->inicio == inicio)
        anterior = primero;

    if (anterior != NULL) {
        siguienteInicio = anterior->next;
        anterior->next = inicio;
        creado = EscribirEBR(tempo, anterior, ruta, anterior->inicio, Extendida);
        EscribirEBR(traid, anterior, ruta2, anterior->inicio, Extendida);
    }

    if (siguiente != NULL) {
        siguienteInicio = siguiente->inicio;
    }

    EBR *nuevoEBR = new EBR(Particion->estado, Particion->fit, inicio, Particion->tamano, siguienteInicio, Particion->pname);


    if (EscribirEBR(tempo, nuevoEBR, ruta, nuevoEBR->inicio, Extendida) == 1) {
        if (EscribirEBR(traid, nuevoEBR, ruta2, nuevoEBR->inicio, Extendida) == 1) {
            cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>> La partición lógica " << nuevoEBR->lname << " fue creada con éxito! \n";
            return;
        }
    }
}

void reducirLogica(MBR *mbr, EBR *ebr, Parti *extendida, char ruta[500], int tamano, char ruta2[500], MBR * mbr2) {

    if (ebr->tamano - tamano <= (signed) sizeof (EBR)) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error el espacio después de reducir debe ser mayor al EBR" << sizeof (EBR) << " bytes) !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        return;
    }

    ebr->tamano = ebr->tamano - tamano;
    if (EscribirEBR(mbr, ebr, ruta, ebr->inicio, extendida) == 1) {
        if (EscribirEBR(mbr2, ebr, ruta2, ebr->inicio, extendida) == 1) {
            cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Se ha reducido la partición " << ebr->lname << " en " << tamano << " bytes\n";
            return;
        }
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error ocurrió un error al guardar el EBR de la partición !!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        return;
    }

}

void reducirParticion(char ruta[500], char nombre[16], int tamano, char ruta2[500]) {
    MBR *mbr = (MBR*) readDisk(ruta, 0, sizeof (MBR));
    MBR *mbr2 = (MBR*) readDisk(ruta2, 0, sizeof (MBR));
    int posicion = -1;
    for (int i = 0; i < 4; i++) {
        if (strncmp(mbr->array_particion[i].pname, nombre, 16) == 0) {
            posicion = i;
        }
    }

    Parti *extendida = NULL;
    for (int i = 0; i < 4; i++) {
        if ((mbr->array_particion[i].tipo == 'E') || (mbr->array_particion[i].tipo == 'e')) {
            extendida = &mbr->array_particion[i];
        }
    }

    if (extendida != NULL) {
        EBR *primero = primerEBR(ruta, extendida);
        EBR *ebr = buscarEBR(ruta, primero, nombre);
        if (ebr != NULL) {
            reducirLogica(mbr, ebr, extendida, ruta, tamano, ruta2, mbr2);
            return;
        }
    }

    if (posicion == -1) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se encontró la partición con el nombre " << nombre << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        return;
    }


    Parti *particion = &mbr->array_particion[posicion];
    if (particion->tamano > tamano) {
        if (particion->tipo == 'e' || particion->tipo == 'E') {
            EBR *primeroebr = primerEBR(ruta, particion);
            EBR *ultimoebr = ultimoEBR(primeroebr, ruta, particion);
            if (ultimoebr->inicio + ultimoebr->tamano > particion->tamano - tamano) {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!! Error el tamaño de la partición después de la reducción debe ser mayor al ocupado por las particiones lógicas !!!!!!!!!!!!!!!!!!!!!!! \n";
                return;
            }
        }
        particion->tamano = particion->tamano + tamano;
        mbr->array_particion[posicion].tamano = particion->tamano;
        mbr2->array_particion[posicion].tamano = particion->tamano;
        if (EscribirArchivo(ruta, 0, mbr, sizeof (MBR)) == 1) {
            if (EscribirArchivo(ruta2, 0, mbr2, sizeof (MBR)) == 1) {
                cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Se  ha reducido la partición " << nombre << " en " << tamano << " bytes\n";
            }
        }
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error el tamaño de la partición después de la reducción debe ser mayor a 0 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        return;
    }
}

void aumentarEspacioLogica(MBR* mbr, EBR *ebr, Parti *extendida, int tamano, char ruta[500], char ruta2[500], MBR * mbr2) {

    int libre = 0;
    EBR* siguiente = siguienteEBR(ruta, ebr);
    if (siguiente == NULL) {
        libre = extendida->tamano - (ebr->inicio + ebr->tamano);
    } else {
        libre = siguiente->inicio - (ebr->inicio + ebr->tamano);
    }

    if (libre > 0 && libre >= tamano) {
        ebr->tamano = ebr->tamano + tamano;
        if (EscribirEBR(mbr, ebr, ruta, ebr->inicio, extendida) == 1) {
            if (EscribirEBR(mbr2, ebr, ruta2, ebr->inicio, extendida) == 1) {
                cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Se ha aumentado la partición " << ebr->lname << " en " << tamano << " bytes\n";
                return;
            }
        }
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no existe suficiente espacio contiguo a la partición para aumentarla en " << tamano << " !!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        return;
    }
}

void agregarEspacio(char ruta[500], char nombre[16], int tamano, char ruta2[500]) {
    MBR *mbr = (MBR*) readDisk(ruta, 0, sizeof (MBR));
    MBR *mbr2 = (MBR*) readDisk(ruta2, 0, sizeof (MBR));
    int posicion = -1;
    for (int i = 0; i < 4; i++) {
        if (strncmp(mbr->array_particion[i].pname, nombre, 16) == 0) {
            posicion = i;
        }
    }
    Parti *extendida = NULL;
    for (int i = 0; i < 4; i++) {
        if ((mbr->array_particion[i].tipo == 'E') || (mbr->array_particion[i].tipo == 'e')) {
            extendida = &mbr->array_particion[i];
        }
    }

    if (extendida != NULL) {
        EBR *primero = primerEBR(ruta, extendida);
        EBR *ebr = buscarEBR(ruta, primero, nombre);
        if (ebr != NULL) {
            aumentarEspacioLogica(mbr, ebr, extendida, tamano, ruta, ruta2, mbr2);
        }
    }

    if (posicion == -1) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se encontró la partición con el nombre " << nombre << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        return;
    }

    Parti *particion = &mbr->array_particion[posicion];
    int libre = 0;
    int pfinal;
    int end = particion->inicio + particion->tamano - 1;
    if (end < mbr->mbr_tamano) {
        pfinal = end;
    } else {
        pfinal = -1;
    }
    Parti *siguiente = &mbr->array_particion[posicion + 1];
    if (siguiente->inicio > -1) {
        libre = siguiente->inicio - pfinal - 1;
    } else {
        libre = mbr->mbr_tamano - pfinal - 1;
    }
    if (libre >= tamano) {
        particion->tamano += tamano;
        if (EscribirArchivo(ruta, 0, mbr, sizeof (MBR)) == 1) {
            if (EscribirArchivo(ruta2, 0, mbr, sizeof (MBR)) == 1) {
                cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>> Se ha aumentado la partición " << nombre << " en " << tamano << " bytes\n";
            }
        }
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no existe suficiente espacio contiguo a la partición para aumentarla en " << tamano << " !!!!!!!!!!!!!!!!!!!!!! \n";
        return;
    }
}

void eliminarPLogica(char ruta[500], MBR* mbr, EBR *ebr, Parti *extendida, char *tipo, char ruta2[500]) {

    EBR* primero = primerEBR(ruta, extendida);
    EBR* siguiente = siguienteEBR(ruta, ebr);
    EBR* anterior = anteriorEBR(ruta, ebr->inicio, primero);

    int iniciae = 0;
    int tamano = 0;

    if (ebr->inicio == primero->inicio) { // Si es la primera partición lógica.
        char lim[16] = "";
        primero = new EBR('I', 'f', ebr->inicio, 0, ebr->next, lim);
        EscribirEBR(mbr, primero, ruta, primero->inicio, extendida);
        EscribirEBR(mbr, primero, ruta2, primero->inicio, extendida);
        iniciae = extendida->inicio + sizeof (EBR);
        tamano = ebr->tamano - sizeof (EBR);
    } else if (ebr->next == -1) { // Si es la última partición lógica
        anterior->next = -1;
        EscribirEBR(mbr, anterior, ruta, anterior->inicio, extendida);
        EscribirEBR(mbr, anterior, ruta2, anterior->inicio, extendida);
        iniciae = ebr->inicio;
        tamano = ebr->tamano;
    } else { // Si es una partición intermedia
        anterior->next = siguiente->inicio;
        EscribirEBR(mbr, anterior, ruta, anterior->inicio, extendida);
        EscribirEBR(mbr, anterior, ruta2, anterior->inicio, extendida);
        iniciae = ebr->inicio;
        tamano = ebr->tamano;
    }

    if (strcasecmp(tipo, "fast") == 0) {
        cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Se ha eliminado la partición lógica " << ebr->lname << ".\n";
        cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Tipo de Eliminación: Fast.\n";
        return;
    } else if (strcasecmp(tipo, "full") == 0) {
        if (vaciarArchivo(ruta, iniciae, iniciae + tamano - 1, 8000) == 1) {
            cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Se ha eliminado la partición lógica " << ebr->lname << ".\n";
            cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Tipo de Eliminación: Full.\n";
            return;
        } else {
            return;
        }
    }
}

void eliminarP(char ruta[500], char *name, char *tipo, char ruta2[500]) {
    MBR *mbr = (MBR*) readDisk(ruta, 0, sizeof (MBR));
    MBR *mbr2 = (MBR*) readDisk(ruta2, 0, sizeof (MBR));
    if (mbr == NULL) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error Ocurrio un error al recuperar el MBR del disco.!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
        return;
    }

    Parti *extendida;
    for (int i = 0; i < 4; i++) {
        if ((mbr->array_particion[i].tipo == 'E') || (mbr->array_particion[i].tipo == 'e')) {
            extendida = &mbr->array_particion[i];
        }
    }
    EBR* primero = primerEBR(ruta, extendida);
    EBR* logica = buscarEBR(ruta, primero, name);
    if (logica != NULL) {
        eliminarPLogica(ruta, mbr, logica, extendida, tipo, ruta2);
        return;
    }

    int pnumero = -1;
    for (int i = 0; i < 4; i++) {
        if (strncmp(mbr->array_particion[i].pname, name, 16) == 0) {
            pnumero = i;
        }
    }
    if (pnumero < 0) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!! Error no se encontró una partición con el nombre " << name << ".!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        return;
    }

    if ((mbr->array_particion[pnumero].tipo == 'E') || (mbr->array_particion[pnumero].tipo == 'e')) {
        primero = primerEBR(ruta, &mbr->array_particion[pnumero]);
        if (primero->tamano > (signed) sizeof (EBR) || primero->next != -1) {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no puede eliminar la partición extendida mientras existan lógicas dentro de ella. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
            return;
        }
    }

    int tamano = mbr->array_particion[pnumero].tamano;
    int pinicio = mbr->array_particion[pnumero].inicio;

    mbr->array_particion[pnumero].estado = 'I'; //inactiva a= activa
    mbr->array_particion[pnumero].inicio = -1;
    mbr->array_particion[pnumero].tamano = 0;
    mbr->array_particion[pnumero].tipo = ' ';
    mbr->array_particion[pnumero].fit = ' ';

    for (int i = 0; i < 16; i++) {
        mbr->array_particion[pnumero].pname[i] = '\0';
    }

    ///raid
    mbr2->array_particion[pnumero].estado = 'I'; //inactiva a= activa
    mbr2->array_particion[pnumero].inicio = -1;
    mbr2->array_particion[pnumero].tamano = 0;
    mbr->array_particion[pnumero].tipo = ' ';
    mbr->array_particion[pnumero].fit = ' ';

    for (int i = 0; i < 16; i++) {
        mbr2->array_particion[pnumero].pname[i] = '\0';
    }

    if (EscribirArchivo(ruta, 0, mbr, sizeof (MBR)) == 0) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error ocurrió un problema al guardar el MBR !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        return;
    }
    //escribiendo en raid
    EscribirArchivo(ruta2, 0, mbr2, sizeof (MBR));

    if (strcasecmp(tipo, "fast") == 0) {
        cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>> Se ha eliminado la partición " << name << ".\n";
        cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>> Tipo de Eliminación: Fast.\n";
    } else if (strcasecmp(tipo, "full") == 0) {
        if (vaciarArchivo(ruta, pinicio, pinicio + tamano - 1, 8000) == 1) {
            if (vaciarArchivo(ruta2, pinicio, pinicio + tamano - 1, 8000) == 1) {
                cout << ">>>>>>>>>>>>>>>>>>>>>>>>>> Se ha eliminado la partición " << name << ".\n";
                cout << ">>>>>>>>>>>>>>>>>>>>>>>>>> Tipo de Eliminación: Full.\n";
            }
        }
    }
}

int contap = 0;
int cdel = 0;

void EjecutarFdisk(int vsize, char vunit, char vpath[500], char vtype, char vfit, char vdeletem[5], char vname[16], int vadd, int bande1, int bande2, int bande3, int vdel) {
    if ((bande1 == 1)&&(bande2 == 1)&&(bande3 == 1)) {
        if (!existeDisco(vpath)) {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error el disco " << vpath << " no existe no se pueden hacer las particiones  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
            return;
        }
        if (vunit == 'k' || vunit == 'K') {
            vsize = vsize * 1024;
        } else if (vunit == 'm' || vunit == 'M') {
            vsize = vsize * 1024 * 1024;
        }

        //Empieza la parti
        char auxpath[500] = "";
        char auxpath2[500] = "";
        char auxpath3[500] = "";
        char auxpath4[500] = "";
        strncpy(auxpath, vpath, 500);
        strncpy(auxpath2, vpath, 500);
        strncpy(auxpath3, vpath, 500);
        strncpy(auxpath4, vpath, 500);
        char *tempo1 = strdup(auxpath);
        char *tempo2 = strdup(auxpath);
        char *pathc = dirname(tempo2);
        char *NDisco = basename(tempo1);
        char aux1[50] = "";
        strcat(aux1, NDisco);
        char *auxDisco = strtok(aux1, ".");
        char raidn[500] = "";
        strcat(raidn, pathc);
        strcat(raidn, "/");
        strcat(raidn, aux1);
        strcat(raidn, "_v1.disk");
        char auxraid[500] = "";
        strncpy(auxraid, raidn, 500);
        MBR *tempo = (MBR*) readDisk(auxpath4, 0, sizeof (MBR));
        MBR *traid = (MBR*) readDisk(raidn, 0, sizeof (MBR));
        if (tempo != NULL) {
            Parti *tempoparti = new Parti();
            tempoparti->tamano = vsize;
            tempoparti->estado = 'A';
            tempoparti->fit = vfit;
            tempoparti->tipo = vtype;

            strncpy(tempoparti->pname, vname, 16);

            if (vsize <= (tempo->mbr_tamano - sizeof (MBR))) {
                int inicio = sizeof (MBR);
                int Activos = 0;
                //numActivos
                for (int i = 0; i < 4; i++) {
                    if (tempo->array_particion[i].estado == 'A') {
                        Activos++;
                    }
                }

                int partiPrim = 0;
                //numprimarias
                for (int i = 0; i < 4; i++) {
                    if ((tempo->array_particion[i].pname != NULL)&&(tempo->array_particion[i].estado == 'A')&&(tempo->array_particion[i].tipo == 'p' || tempo->array_particion[i].tipo == 'P')) {
                        partiPrim++;
                    }
                }

                int partiExt = 0;
                //numextendidas
                for (int i = 0; i < 4; i++) {
                    if ((tempo->array_particion[i].pname != NULL)&&(tempo->array_particion[i].estado == 'A')&&(tempo->array_particion[i].tipo == 'e' || tempo->array_particion[i].tipo == 'E')) {
                        partiExt++;
                        break;
                    }
                }

                bool repetido = false;
                //numrepedidas
                for (int i = 0; i < 4; i++) {
                    if ((tempo->array_particion[i].pname != NULL)&&(strcasecmp(tempo->array_particion[i].pname, vname)) == 0) {
                        repetido = true;
                        break;
                    }
                }
                int puntero = -1;
                if ((!repetido)) {

                    if (tempoparti->tipo == 'P' || tempoparti->tipo == 'p') {
                        if (partiPrim < 3) {
                            crearParticionPrimaria(auxpath2, tempo, tempoparti, traid, auxraid);
                        } else {
                            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error ya existen 3 particiones primarias. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
                        }
                    } else if (tempoparti->tipo == 'E' || tempoparti->tipo == 'e') {
                        if (partiExt == 0) {
                            crearParticionExtendida(auxpath2, tempo, tempoparti, traid, auxraid);

                        } else {
                            cout << "\n !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error ya existe una partición extendida !!!!!!!!!!!!!!!!!!!! \n \n";
                        }
                    } else if (tempoparti->tipo == 'L' || tempoparti->tipo == 'l') {
                        if ((partiExt == 1)) {
                            for (int i = 0; i <= 4; i++) {
                                if (tempo->array_particion[i].tipo == 'e' || tempo->array_particion[i].tipo == 'E') {
                                    if (tempoparti->tamano <= tempo->array_particion[i].tamano) {
                                        //crearLogica(char ruta [500], MBR *tempo, Parti *Particion, Parti *Extendida, MBR *traid, char ruta2[500])
                                        crearLogica(auxpath2, tempo, tempoparti, &tempo->array_particion[i], traid, auxraid);
                                        return;
                                    } else {
                                        cout << "\n !!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error el tamaño de la particion logica " << tempoparti->pname << " no puede ser más gran que la Extendida \n";
                                    }
                                }
                            }
                        } else {
                            cout << "!!!!!!!!!!!!!!!!!!! Error No se han completado las primarias o no existe la extendida !!!!!!!!!!!!!!!!!! \n";
                        }
                    }

                }
            } else {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!Error El tamaño de la partición es muy grande !!!!!!!!!!!!!!!!!!!! \n";
            }
            ///pruebas
            //recorriendo particiones
            cout << "\n";
            cout << "Recorriendo particiones \n";
            for (int i = 0; i < 4; i++) {
                if (tempo->array_particion[i].estado == 'I') {
                    cout << "-Nombre:   libre   -Tamaño:  0        -Estado:   I \n";
                } else {
                    cout << "-Nombre:   " << tempo->array_particion[i].pname << "   -Tamaño:  " << tempo->array_particion[i].tamano << "   -Estado:   " << tempo->array_particion[i].estado << "  -Tipo   " << tempo->array_particion[i].tipo << "\n";
                }
            }
            free(tempo);
        } else {
            cout << "!!!!!!!!!!!Error no se encontró el disco !!!!!!!!!!!!!!!!!!  \n";
            return;
        }
    } else if ((bande2 == 1) && (bande3 == 1)) {
        if (vdel == 1) {
            cdel++;
            char cadena[3] = "";
            cout << "Desea eliminar la partición " << vname << " (y / n)";
            cout << "\n";
            scanf("%c", cadena);
            while (getchar() != '\n');
            if (cadena[0] == 'y' || cadena[0] == 'Y') {
                char vname2[16] = "";
                strncpy(vname2, vname, 16);
                char auxpath[500] = "";
                strncpy(auxpath, vpath, 500);
                char auxpath4[500] = "";
                strncpy(auxpath4, vpath, 500);
                char *tempo1 = strdup(auxpath);
                char *tempo2 = strdup(auxpath);
                char *pathc = dirname(tempo2);
                char *NDisco = basename(tempo1);
                char aux1[50] = "";
                strcat(aux1, NDisco);
                char *auxDisco = strtok(aux1, ".");
                char raidn[500] = "";
                strcat(raidn, pathc);
                strcat(raidn, "/");
                strcat(raidn, aux1);
                strcat(raidn, "_v1.disk");
                char auxraid[500] = "";
                strncpy(auxraid, raidn, 500);
                eliminarP(auxpath, vname2, vdeletem, auxraid);
            } else {
                cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Se canceló la eliminación de la partición" << vname << " \n";
            }
        } else {
            if (vadd < 0) {
                char vname2[16] = "";
                strncpy(vname2, vname, 16);
                char auxpath[500] = "";
                strncpy(auxpath, vpath, 500);
                //casteo
                if (vunit == 'k' || vunit == 'K') {
                    vadd = vadd * 1024;
                } else if (vunit == 'm' || vunit == 'M') {
                    vadd = vadd * 1024 * 1024;
                }
                //raid
                char auxpath4[500] = "";
                strncpy(auxpath4, vpath, 500);
                char *tempo1 = strdup(auxpath);
                char *tempo2 = strdup(auxpath);
                char *pathc = dirname(tempo2);
                char *NDisco = basename(tempo1);
                char aux1[50] = "";
                strcat(aux1, NDisco);
                char *auxDisco = strtok(aux1, ".");
                char raidn[500] = "";
                strcat(raidn, pathc);
                strcat(raidn, "/");
                strcat(raidn, aux1);
                strcat(raidn, "_v1.disk");
                char auxraid[500] = "";
                strncpy(auxraid, raidn, 500);
                reducirParticion(auxpath, vname2, vadd, auxraid);
            } else {
                char vname2[16] = "";
                strncpy(vname2, vname, 16);
                char auxpath[500] = "";
                strncpy(auxpath, vpath, 500);
                ///casteo
                if (vunit == 'k' || vunit == 'K') {
                    vadd = vadd * 1024;
                } else if (vunit == 'm' || vunit == 'M') {
                    vadd = vadd * 1024 * 1024;
                }

                //raid
                char auxpath4[500] = "";
                strncpy(auxpath4, vpath, 500);
                char *tempo1 = strdup(auxpath);
                char *tempo2 = strdup(auxpath);
                char *pathc = dirname(tempo2);
                char *NDisco = basename(tempo1);
                char aux1[50] = "";
                strcat(aux1, NDisco);
                char *auxDisco = strtok(aux1, ".");
                char raidn[500] = "";
                strcat(raidn, pathc);
                strcat(raidn, "/");
                strcat(raidn, aux1);
                strcat(raidn, "_v1.disk");
                char auxraid[500] = "";
                strncpy(auxraid, raidn, 500);
                agregarEspacio(auxpath, vname2, vadd, auxraid);
            }
        }
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error faltan parametros en fdisk !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
    }
}

void analizFdisk(char comando[10000]) {
    int bande1 = 0;
    int bande2 = 0;
    int bande3 = 0;
    int vdel = 0;
    char *cut = strtok(comando, " ");
    cut = strtok(NULL, " =");
    int vsize;
    char vunit = 'K';
    char *vpath = "";
    char vfit = 'w';
    char* vdeletem = "";
    char* vname = "";
    int vadd = 0;
    char vtype = 'p';
    while (cut != NULL) {
        if (strcasecmp(cut, "-size") == 0) {
            cut = strtok(NULL, "= ");
            bande1 = 1;
            vsize = atoi(cut);
        } else if (strcasecmp(cut, "-unit") == 0) {
            cut = strtok(NULL, "= ");
            if ((strcasecmp(cut, "b") == 0) || (strcasecmp(cut, "k") == 0) || (strcasecmp(cut, "m") == 0)) {
                vunit = cut[0];
            } else {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error el unit en fdisk no esta segun lo establecido !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
            }
        } else if (strcasecmp(cut, "-fit") == 0) {
            cut = strtok(NULL, "= ");
            if ((strcasecmp(cut, "ff") == 0) || (strcasecmp(cut, "bf") == 0) || (strcasecmp(cut, "wf") == 0)) {
                vfit = cut[0];
            } else {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error el fit en fdisk no esta segun lo establecido !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            }
        } else if (strcasecmp(cut, "-path") == 0) {
            cut = strtok(NULL, "= ");
            bande2 = 1;
            char aux[1000];
            if (*cut != '\"') {
                vpath = cut;
                //  cout<<"La ruta del path es: %s\n", cut;
            } else {
                char *tempo;
                vpath = quitaCm(cut);
                //cout<<"La ruta del path es: %s\n", vpath;
            }
        } else if (strcasecmp(cut, "-type") == 0) {
            cut = strtok(NULL, "= ");
            if ((strcasecmp(cut, "p") == 0) || (strcasecmp(cut, "e") == 0) || (strcasecmp(cut, "l") == 0)) {
                vtype = cut[0];
            } else {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error el type en fdisk no esta segun lo establecido !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            }
        } else if (strcasecmp(cut, "-delete") == 0) {
            cut = strtok(NULL, "= ");
            if ((strcasecmp(cut, "full") == 0) || (strcasecmp(cut, "fast") == 0)) {
                vdeletem = cut;
                vdel = 1;
            } else {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error el delete en fdisk no esta segun lo establecido !!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
            }
        } else if (strcasecmp(cut, "-name") == 0) {
            cut = strtok(NULL, "= ");
            bande3 = 1;
            if (*cut != '\"') {
                vname = cut;
            } else {
                char *tempo;
                vname = quitaCm(cut);
            }
        } else if (strcasecmp(cut, "-add") == 0) {
            cut = strtok(NULL, "= ");
            vadd = atoi(cut);
        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error comando desconocido en fdisk:" << cut << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
            break;
        }
        cut = strtok(NULL, "= ");
    }
    EjecutarFdisk(vsize, vunit, vpath, vtype, vfit, vdeletem, vname, vadd, bande1, bande2, bande3, vdel);
}

void EjecutaMount(char name[16], char path[500]) {
    char auxPath[500];
    strncpy(auxPath, path, 500);
    bool existe;
    bool encontro = false;
    bool letraNueva = true;
    MBR *tempo = (MBR*) readDisk(path, 0, sizeof (MBR));
    if (tempo != NULL) {

        for (int i = 0; i < 4; i++) {
            if ((tempo->array_particion[i].estado == 'A')&&(strcasecmp(tempo->array_particion[i].pname, name) == 0)) {
                Mount *NodoNuevo = new Mount(tempo->array_particion[i].tipo, path, "", tempo->array_particion[i].estado, tempo->array_particion[i].fit, tempo->array_particion[i].inicio, tempo->array_particion[i].tamano, tempo->array_particion[i].pname, NULL, NULL);
                encontro = true;

                //insertar
                if (primero == NULL) {
                    strcpy(NodoNuevo->id, "vda1");
                    NodoNuevo->letra = 97;
                    NodoNuevo->numero = 1;
                    primero = NodoNuevo;
                    ultimo = NodoNuevo;
                    cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>> El Mount de la particion: " << NodoNuevo->part_name << " se creó con éxito id: " << NodoNuevo->id << " \n";
                } else {
                    Mount *NodoTempo;
                    NodoTempo = primero;
                    int letActual;
                    int numActual;
                    int letActual2;
                    while (NodoTempo != NULL) {
                        if ((strcasecmp(NodoTempo->path, auxPath) == 0)) {
                            letActual2 = NodoTempo->letra;
                            numActual = NodoTempo->numero;
                            letraNueva = false;
                            if (strcasecmp(NodoTempo->part_name, tempo->array_particion[i].pname) == 0) {
                                existe = true;
                            }
                        }
                        letActual = NodoTempo->letra;
                        NodoTempo = NodoTempo->next;
                    }
                    if (!existe) {
                        if ((!letraNueva)) {
                            numActual++;
                            NodoNuevo->letra = (char) letActual2;
                            NodoNuevo->numero = numActual;
                            char idm = (char) letActual;
                            char idm2[16] = "";
                            idm2[0] = 'v';
                            idm2[1] = 'd';
                            idm2[2] = idm;
                            if (numActual == 1) {
                                idm2[3] = '1';
                            } else if (numActual == 2) {
                                idm2[3] = '2';
                            } else if (numActual == 3) {
                                idm2[3] = '3';
                            } else if (numActual == 4) {
                                idm2[3] = '4';
                            }
                            strncpy(NodoNuevo->id, idm2, 16);
                            ultimo->next = NodoNuevo;
                            NodoNuevo->prev = ultimo;
                            ultimo = NodoNuevo;
                            cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>> El Mount de la particion: " << NodoNuevo->part_name << " se creó con éxito id: " << NodoNuevo->id << " \n";
                        } else {
                            letActual++;
                            NodoNuevo->letra = letActual;
                            NodoNuevo->numero = 1;
                            char idm2[16] = "";
                            idm2[0] = 'v';
                            idm2[1] = 'd';
                            idm2[2] = (char) letActual;
                            idm2[3] = '1';
                            strncpy(NodoNuevo->id, idm2, 16);
                            ultimo->next = NodoNuevo;
                            NodoNuevo->prev = ultimo;
                            ultimo = NodoNuevo;
                            cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>> El Mount de la particion: " << NodoNuevo->part_name << " se creó con éxito id: " << NodoNuevo->id << " \n";
                        }
                    } else {
                        cout << "!!!!!!!!!!!!!!!Error Ya existe el Mount que desea agregar !!!!!!!!!!!!!!!!!!!!!!!!!!";
                    }
                }
            }
        }
        if (!encontro) {
            cout << "!!!!!!!!!!!!!!!!!!!!!! Error No se encontró la partición !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
        }
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!! Error No se encontró el disco en Ejecuta Mount !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
    }
    //Recorremos la lista
    Mount *AuxNode = primero;
    cout << "\n################################ Recorriendo lista Mount ################################### \n";
    while (AuxNode != NULL) {
        cout << "---Mount= " << AuxNode->id << " NombreParticion= " << AuxNode->part_name << " TipoPart= " << AuxNode->part_type << " --- \n";
        cout << "---AjustePart  = " << AuxNode->part_fit << " TamanoPart= " << AuxNode->part_size << "   Path= " << AuxNode->path << " \n";

        AuxNode = AuxNode->next;
    }
    cout << "-------------------------------------------------------------------------------------------- \n";
}

void analizaMount(char comando[10000]) {
    int bande1 = 0;
    int bande2 = 0;
    char *nombre;
    char *path;
    char *cut = strtok(comando, " ");
    cut = strtok(NULL, " =");
    while (cut != NULL) {
        if (strcasecmp(cut, "-name") == 0) {
            cut = strtok(NULL, "= ");
            bande1 = 1;
            nombre = cut;
        } else if (strcasecmp(cut, "-path") == 0) {
            cut = strtok(NULL, "= ");
            bande2 = 1;
            if (*cut != '\"') {
                path = cut;
                //cout<<"La ruta del path es: "<<cut<<"\n";
            } else {
                path = quitaCm(cut);
                //cout<<"La ruta del path es: "<<cut<<"\n";
            }
        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error comando desconocido de Mount: " << cut << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
            break;
        }
        cut = strtok(NULL, "= ");
    }
    EjecutaMount(nombre, path);
}

void EjecutaUnmount(char id[16]) {
    char idtempo[16];
    strncpy(idtempo, id, 16);
    Mount *AuxNode;
    bool encontro = false;
    if (primero != NULL) {
        AuxNode = primero;
        while (AuxNode != NULL) {
            if (strcasecmp(AuxNode->id, idtempo) == 0) {
                if (AuxNode == primero) {
                    Mount *libera = primero;
                    primero = primero->next;
                    free(primero);
                    cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>> Se desmontó exitosamente el mount \n";
                    encontro = true;
                    break;
                } else if (AuxNode == ultimo) {
                    Mount *libera = ultimo;
                    ultimo = ultimo->prev;
                    ultimo->next = NULL;
                    free(libera);
                    encontro = true;
                    cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>> Se desmontó exitosamente el mount \n";
                    break;
                } else {
                    Mount *libera = AuxNode;
                    AuxNode->prev->next = AuxNode->next;
                    AuxNode->next->prev = AuxNode->prev;
                    free(libera);
                    encontro = true;
                    cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>> Se desmontó exitosamente el mount \n";
                    break;
                }
            }
            AuxNode = AuxNode->next;
        }

        if (!encontro) {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!! Error No se encontró el id buscado, no desmontó!!!!!!!!!!!!!!!!!!!!!!!! \n";
        } else {
            //Recorremos la lista
            Mount *AuxNode2 = primero;
            cout << "\n################################ Recorriendo lista Mount ################################### \n";
            while (AuxNode2 != NULL) {
                cout << "---Mount= " << AuxNode2->id << " NombreParticion= " << AuxNode2->part_name << " TipoPart= " << AuxNode2->part_type << " --- \n";
                cout << "---AjustePart  = " << AuxNode2->part_fit << " TamanoPart= " << AuxNode2->part_size << "   Path= " << AuxNode2->path << " \n";

                AuxNode2 = AuxNode2->next;
            }
            cout << "-------------------------------------------------------------------------------------------- \n";

        }
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!! Error No existe ningun Mount Todavía !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    }
}

void analizaUnmount(char comando[10000]) {

    char *cut = strtok(comando, " ");
    cut = strtok(NULL, " =");
    if (strcasecmp(cut, "-id") == 0) {
        cut = strtok(NULL, "= ");
        EjecutaUnmount(cut);
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error comando desconocido en unmount !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
    }
}

void ReporteMBR(MBR *Repo, char vpath[500], char carpeDot[500], char nombre[25], char pathdisco[500], char nombre2[25]) {
    char terminal[1000] = "";
    char terminal2[1000] = "";
    char vdot[500] = "";
    strncpy(vdot, carpeDot, 500);
    strcat(vdot, "/");
    strcat(vdot, nombre);
    char auxpath[500] = "";
    char auxpath2[500] = "";
    strncpy(auxpath, vpath, 500);
    strncpy(auxpath2, pathdisco, 500);
    char *ndir = strdup(auxpath2);
    char *nombreD = basename(ndir);
    Parti *Extendida = new Parti();
    FILE *nuevoRepo;
    nuevoRepo = fopen(vdot, "w+");
    fprintf(nuevoRepo, "digraph G{\n");
    fprintf(nuevoRepo, "MBR [\n");
    fprintf(nuevoRepo, "shape=plaintext\n");
    fprintf(nuevoRepo, "label=<\n");
    fprintf(nuevoRepo, "<table border='0' cellborder='1' cellspacing='0' cellpadding='10'>\n");
    fprintf(nuevoRepo, "<tr><td><b>   Nombre </b></td><td><b>  Valor  </b></td></tr> \n");

    fprintf(nuevoRepo, "<tr><td bgcolor='LightSeaGreen'><b>   Nombre Disco </b></td><td>  %s  </td></tr> \n", nombreD);
    fprintf(nuevoRepo, "<tr><td><b> Tamano MBR </b></td><td> %i </td></tr> \n", Repo->mbr_tamano);
    fprintf(nuevoRepo, "<tr><td><b> Fecha De creacion </b></td><td> 24/08/19 </td></tr> \n");
    fprintf(nuevoRepo, "<tr><td><b> MBR Disk Signature </b></td><td> %i </td></tr> \n", Repo->mbr_disk_signature);
    fprintf(nuevoRepo, "<tr><td><b> Disk Fit </b></td><td>  First Fit </td></tr> \n");
    for (int i = 0; i < 4; i++) {
        if (Repo->array_particion[i].estado != 'I') {
            fprintf(nuevoRepo, "<tr><td bgcolor='LightSeaGreen'><b> Nombre Particion </b></td><td> %s </td></tr> \n", Repo->array_particion[i].pname);
            fprintf(nuevoRepo, "<tr><td><b> Estado Particion %s </b></td><td> Activa </td></tr> \n", Repo->array_particion[i].pname);
            fprintf(nuevoRepo, "<tr><td><b> Tipo Particion %s </b></td><td> %c </td></tr> \n", Repo->array_particion[i].pname, Repo->array_particion[i].tipo);
            fprintf(nuevoRepo, "<tr><td><b> Fit Particion %s </b></td><td> %c </td></tr> \n", Repo->array_particion[i].pname, Repo->array_particion[i].fit);
            fprintf(nuevoRepo, "<tr><td><b> Start Particion %s </b></td><td> %i </td></tr> \n", Repo->array_particion[i].pname, Repo->array_particion[i].inicio);
            fprintf(nuevoRepo, "<tr><td><b> Size Particion %s  </b></td><td> %i </td></tr> \n", Repo->array_particion[i].pname, Repo->array_particion[i].tamano);
        }
        if ((Repo->array_particion[i].tipo == 'e') || (Repo->array_particion[i].tipo == 'E')) {
            Extendida->estado = Repo->array_particion[i].estado;
            Extendida->fit = Repo->array_particion[i].fit;
            Extendida->inicio = Repo->array_particion[i].inicio;
            Extendida->tamano = Repo->array_particion[i].tamano;
            Extendida->tipo = Repo->array_particion[i].tipo;
            strncpy(Extendida->pname, Repo->array_particion[i].pname, 16);
        }
    }
    fprintf(nuevoRepo, "</table>\n");
    fprintf(nuevoRepo, ">\n");
    fprintf(nuevoRepo, "];\n");
    ////ebr
    int bande = 0;
    if (Extendida != NULL) {
        EBR* primero = (EBR*) readDisk(auxpath2, Extendida->inicio, sizeof (EBR));
        if (primero == NULL) {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se puede recuperar el primer EBR de %s !!!!!!!!!!!!!!!!!!!!!!!!!!!", Extendida->pname);
            return;
        }
        EBR *nodoaux;
        if (primero != NULL) {
            if (primero->tamano > 0) {
                fprintf(nuevoRepo, "EBR_%s [\n", primero->lname);
                fprintf(nuevoRepo, "shape=plaintext\n");
                fprintf(nuevoRepo, "label=<\n");
                fprintf(nuevoRepo, "<table border='0' cellborder='1' cellspacing='0' cellpadding='10' bgcolor='LightSeaGreen'>\n");
                fprintf(nuevoRepo, "<tr><td><b>   Nombre  </b></td><td>  Valor  </td></tr> \n");
                fprintf(nuevoRepo, "<tr><td><b>   Nombre EBR  </b></td><td>  %s  </td></tr> \n", primero->lname);
                fprintf(nuevoRepo, "<tr><td><b>   Estado  </b></td><td>  Activo  </td></tr> \n");
                fprintf(nuevoRepo, "<tr><td><b>   Fit  </b></td><td>  %c  </td></tr> \n", primero->fit);
                fprintf(nuevoRepo, "<tr><td><b>   Start  </b></td><td>  %i  </td></tr> \n", primero->inicio);
                fprintf(nuevoRepo, "<tr><td><b>   Tamano  </b></td><td>  %i  </td></tr> \n", primero->tamano);
                bande = 1;
            }
            if (primero == NULL || primero->next == -1) {
                nodoaux = NULL;
            } else {
                nodoaux = (EBR*) readDisk(auxpath2, primero->next, sizeof (EBR));
            }
            if (nodoaux != NULL) {
                if (bande == 1) {
                    fprintf(nuevoRepo, "<tr><td>   Siguiente  </td><td>  %i  </td></tr> \n", primero->next);
                    fprintf(nuevoRepo, "</table>\n");
                    fprintf(nuevoRepo, ">\n");
                    fprintf(nuevoRepo, "];\n");
                }
                while (nodoaux != NULL) {
                    if (nodoaux->tamano > 0) {
                        fprintf(nuevoRepo, "EBR_%s [\n", nodoaux->lname);
                        fprintf(nuevoRepo, "shape=plaintext\n");
                        fprintf(nuevoRepo, "label=<\n");
                        fprintf(nuevoRepo, "<table border='0' cellborder='1' cellspacing='0' cellpadding='10' bgcolor='LightSeaGreen'>\n");
                        fprintf(nuevoRepo, "<tr><td><b>   Nombre  </b></td><td>  Valor  </td></tr> \n");
                        fprintf(nuevoRepo, "<tr><td><b>   Nombre EBR  </b></td><td>  %s  </td></tr> \n", nodoaux->lname);
                        fprintf(nuevoRepo, "<tr><td><b>   Estado  </b></td><td>  Activo  </td></tr> \n");
                        fprintf(nuevoRepo, "<tr><td><b>   Fit  </b></td><td>  %c  </td></tr> \n", nodoaux->fit);
                        fprintf(nuevoRepo, "<tr><td><b>   Start  </b></td><td>  %i  </td></tr> \n", nodoaux->inicio);
                        fprintf(nuevoRepo, "<tr><td><b>   Tamano  </b></td><td>  %i  </td></tr> \n", nodoaux->tamano);
                        EBR*temporal = NULL;
                        if (nodoaux == NULL || nodoaux->next == -1) {
                            temporal = NULL;
                        } else {
                            temporal = (EBR*) readDisk(auxpath2, nodoaux->next, sizeof (EBR));
                        }
                        if (temporal != NULL) {
                            fprintf(nuevoRepo, "<tr><td><b>   Siguiente  </b></td><td>  %i  </td></tr> \n", nodoaux->next);
                            fprintf(nuevoRepo, "</table>\n");
                            fprintf(nuevoRepo, ">\n");
                            fprintf(nuevoRepo, "];\n");
                        } else {
                            fprintf(nuevoRepo, "</table>\n");
                            fprintf(nuevoRepo, ">\n");
                            fprintf(nuevoRepo, "];\n");
                        }
                    }
                    if (nodoaux == NULL || nodoaux->next == -1) {
                        nodoaux = NULL;
                    } else {
                        nodoaux = (EBR*) readDisk(auxpath2, nodoaux->next, sizeof (EBR));
                    }
                }

            } else {
                if (bande == 1) {
                    fprintf(nuevoRepo, "</table>\n");
                    fprintf(nuevoRepo, ">\n");
                    fprintf(nuevoRepo, "];\n");
                }
            }
        }
    }
    fprintf(nuevoRepo, "}");
    fclose(nuevoRepo);
    char ext[3] = "";

    strcpy(terminal, "dot ");
    for (int i = 0; i < 16; i++) {
        if (nombre2[i] == '.') {
            ext[0] = nombre2[i + 1];
            ext[1] = nombre2[i + 2];
            ext[2] = nombre2[i + 3];
            break;
        }
    }
    if (ext[0] == 'j' || ext[0] == 'J') {
        strcat(terminal, "-Tjpg ");
    } else {
        strcat(terminal, "-Tpdf ");
    }
    strcat(terminal, vdot);
    strcat(terminal, " -o ");
    strcat(terminal, auxpath);
    system(terminal);
    /*
    strcpy(terminal2, "xdg-open ");
    strcat(terminal2, auxpath);
    system(terminal2);*/

}

void ReporteDisco(MBR *Repo, char vpath[500], char carpeDot[500], char nombre[25], char pathdisco[500], char nombre2[500]) {
    char vdot[500] = "";
    strncpy(vdot, carpeDot, 500);
    strcat(vdot, "/");
    strcat(vdot, nombre);
    char terminal[1000] = "";
    char auxPath[500] = "";
    strncpy(auxPath, vpath, 500);
    char terminal2[1000] = "";
    double dataR = 0;
    int particion = 0;
    int espacioT = 0;
    int libre = 0;
    double porcentaje = 0.00;

    FILE *nuevoRepo;
    nuevoRepo = fopen(vdot, "w+");
    fprintf(nuevoRepo, "digraph {\n");
    fprintf(nuevoRepo, "Nodo [\n");
    fprintf(nuevoRepo, "shape=plaintext\n");
    fprintf(nuevoRepo, "label=<\n");
    fprintf(nuevoRepo, "<table border='0' cellborder='1' cellspacing='0' cellpadding='10'>\n");
    fprintf(nuevoRepo, "<tr>\n");

    espacioT = Repo->mbr_tamano;
    particion = sizeof (MBR);
    dataR = (double) ((double) particion / (double) espacioT)*100;
    fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='lightcoral'>MBR<br/></td>");
    int nextPos = sizeof (MBR);
    for (int i = 0; i < 4; i++) {

        libre = Repo->array_particion[i].inicio - nextPos;
        if (libre > 0) {
            porcentaje = ((double) libre / (double) espacioT)*100;
            if (porcentaje >= 0.01) {
                fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='white'>Libre<br/>%.2f%c<br/>%d bytes</td>", porcentaje, '%', libre);
            } else {
                fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='white'>Libre<br/>%d bytes</td>", libre);
            }

        }
        porcentaje = ((double) Repo->array_particion[i].tamano / (double) espacioT)*100;

        if ((Repo->array_particion[i].tipo == 'E') || (Repo->array_particion[i].tipo == 'e')) {

            fprintf(nuevoRepo, "<td border='1.5' width='50'  bgcolor='lightcoral' cellpadding='0'>\n");
            fprintf(nuevoRepo, "<table border='0' cellborder='1' cellspacing='0' cellpadding='10'>");

            if (porcentaje >= 0.01) {
                fprintf(nuevoRepo, "<tr><td colspan='100'>Particion %d<br/>%s<br/>%s<br/>%.2f%c<br/>%d bytes</td></tr>", i + 1, "Extendida", Repo->array_particion[i].pname, porcentaje, '%', Repo->array_particion[i].tamano);
            } else {
                fprintf(nuevoRepo, "<tr><td colspan='100'>Particion %d<br/>%s<br/>%s<br/>%d bytes</td></tr>", i + 1, "Extendida", Repo->array_particion[i].pname, Repo->array_particion[i].tamano);
            }

            fprintf(nuevoRepo, "<tr>\n");
            Parti *aux = &Repo->array_particion[i];
            EBR *primero = (EBR*) readDisk(pathdisco, aux->inicio, sizeof (EBR));
            if (primero == NULL) {
                printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se puede recuperar el primer EBR de %s !!!!!!!!!!!!!!!!!!!!!!!!!!!", aux->pname);
                return;
            }
            double ebrp = ((double) sizeof (EBR) / (double) espacioT)*100;
            if (ebrp >= 0.01) {
                fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='lightcoral'>EBR<br/>%.2f%c<br/>%ld bytes</td>", ebrp, '%', sizeof (EBR));
            } else {
                fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='lightcoral'>EBR<br/>%ld bytes</td>", sizeof (EBR));
            }

            if (primero->tamano > (signed) sizeof (EBR)) {
                porcentaje = ((double) primero->tamano / (double) espacioT)*100;
                if (porcentaje >= 0.01) {
                    fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='lightcoral'>Logica<br/>%s<br/>%.2f%c<br/>%ld bytes</td>", primero->lname, porcentaje, '%', primero->tamano);
                } else {
                    fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='lightcoral'>Logica<br/>%s<br/>%ld bytes</td>", primero->lname, primero->tamano);
                }
            }

            EBR *siguiente = NULL;
            if (primero == NULL || primero->next == -1) {
                siguiente = NULL;
            } else {
                siguiente = (EBR*) readDisk(pathdisco, primero->next, sizeof (EBR));
            }
            EBR *anterior = primero;
            while (siguiente != NULL) {
                libre = siguiente->inicio - anterior->inicio - anterior->tamano;
                if (libre > 0) {
                    porcentaje = ((double) libre / (double) espacioT)*100;
                    if (porcentaje >= 0.01) {
                        fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='white'>Libre<br/>%.2f%c<br/>%d bytes</td>", porcentaje, '%', libre);
                    } else {
                        fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='white'>Libre<br/>%d bytes</td>", libre);
                    }
                }

                ebrp = ((double) sizeof (EBR) / (double) espacioT)*100;
                if (ebrp >= 0.01) {
                    fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='lightcoral'>EBR<br/>%.2f%c<br/>%ld bytes</td>", ebrp, '%', sizeof (EBR));
                } else {
                    fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='lightcoral'>EBR<br/>%ld bytes</td>", sizeof (EBR));
                }

                porcentaje = ((double) siguiente->tamano / (double) espacioT)*100;
                if (porcentaje >= 0.01) {
                    fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='lightcoral'>Logica<br/>%s<br/>%.2f%c<br/>%ld bytes</td>", siguiente->lname, porcentaje, '%', siguiente->tamano);
                } else {
                    fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='lightcoral'>Logica<br/>%s<br/>%ld bytes</td>", siguiente->lname, siguiente->tamano);
                }

                anterior = siguiente;
                if (anterior == NULL || anterior->next == -1) {
                    siguiente = NULL;
                } else {
                    siguiente = (EBR*) readDisk(pathdisco, anterior->next, sizeof (EBR));
                }
            }

            libre = Repo->array_particion[i].inicio + Repo->array_particion[i].tamano - anterior->inicio - anterior->tamano;

            if (libre > 0) {
                porcentaje = ((double) libre / (double) espacioT)*100;
                if (porcentaje >= 0.01) {
                    fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='white'>Libre<br/>%.2f%c<br/>%d bytes</td>", porcentaje, '%', libre);
                } else {
                    fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='white'>Libre<br/>%d bytes</td>", libre);
                }
            }

            fprintf(nuevoRepo, "</tr>\n");
            fprintf(nuevoRepo, "</table>");
            fprintf(nuevoRepo, "</td>");
            nextPos = Repo->array_particion[i].inicio + Repo->array_particion[i].tamano;

        } else if ((Repo->array_particion[i].tipo == 'P') || (Repo->array_particion[i].tipo == 'p')) {
            if (porcentaje >= 0.01) {
                fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='lightcoral'>Particion %d<br/>%s<br/>%s<br/>%.2f%c<br/>%d bytes</td>", i + 1, "Primaria", Repo->array_particion[i].pname, porcentaje, '%', Repo->array_particion[i].tamano);
            } else {
                fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='lightcoral'>Particion %d<br/>%s<br/>%s<br/>%d bytes</td>", i + 1, "Primaria", Repo->array_particion[i].pname, Repo->array_particion[i].tamano);
            }
            nextPos = Repo->array_particion[i].inicio + Repo->array_particion[i].tamano;
        }
    }
    libre = Repo->mbr_tamano - nextPos;
    if ((libre != 0)) {
        porcentaje = ((double) libre / (double) espacioT)*100;
        if (porcentaje >= 0.01) {
            fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='white'>Libre<br/>%.2f%c<br/>%d bytes</td>", porcentaje, '%', libre);
        } else {
            fprintf(nuevoRepo, "<td border='1.5' height='80' width='30' bgcolor='white'>Libre<br/>%d bytes</td>", libre);
        }
    }

    fprintf(nuevoRepo, "</tr> \n");
    fprintf(nuevoRepo, "</table> \n");
    fprintf(nuevoRepo, ">]; } \n");
    fclose(nuevoRepo);

    char ext[3] = "";

    strcpy(terminal, "dot ");
    for (int i = 0; i < 16; i++) {
        if (nombre2[i] == '.') {
            ext[0] = nombre2[i + 1];
            ext[1] = nombre2[i + 2];
            ext[2] = nombre2[i + 3];
            break;
        }
    }
    if (ext[0] == 'j' || ext[0] == 'J') {
        strcat(terminal, "-Tjpg ");
    } else {
        strcat(terminal, "-Tpdf ");
    }
    strcat(terminal, vdot);
    strcat(terminal, " -o ");
    strcat(terminal, auxPath);
    system(terminal); /*
    strcpy(terminal2, "xdg-open ");
    strcat(terminal2, auxPath);
    system(terminal2);*/


}

void ReporteBMInode(Mount*particion, char nombre[25], char nombre2[25], char mm[500], char path[500]) {
    existeCarpeta(mm);
    FILE *repo = NULL;
    repo = fopen(path, "w");
    if (repo == NULL) {
        cout << "Error no se encontro el archivo para le pdf\n";
    }
    SuperBloque* sbloque = BuscarSuperBloque(particion);
    int inicio;
    inicio = sbloque->s_bm_inode_start;
    Bitmap* bm = (Bitmap*) readDisk(particion->path, inicio, sizeof (Bitmap));
    for (int i = 0; i < bm->size; i++) {

        if (bm->bitmaps[i] == '1') {
            fprintf(repo, "1 ");
        } else {
            fprintf(repo, "0 ");
        }
        if (i % 20 == 0 && i != 0) {
            fprintf(repo, "\n");
        }
    }

    fclose(repo);
    cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> El reporte " << nombre << " se creó con éxito \n";
}

void ReporteBMBlock(Mount*particion, char nombre[25], char nombre2[25], char mm[500], char path[500]) {
    existeCarpeta(mm);
    FILE *repo = NULL;
    repo = fopen(path, "w");
    if (repo == NULL) {
        cout << "Error no se encontro el archivo para le pdf\n";
    }
    SuperBloque* sbloque = BuscarSuperBloque(particion);
    int inicio;
    inicio = sbloque->s_bm_block_start;
    Bitmap* bm = (Bitmap*) readDisk(particion->path, inicio, sizeof (Bitmap));
    for (int i = 0; i < bm->size; i++) {

        if (bm->bitmaps[i] == '1') {
            fprintf(repo, "1 ");
        } else {
            fprintf(repo, "0 ");
        }
        if (i % 20 == 0 && i != 0) {
            fprintf(repo, "\n");
        }
    }

    fclose(repo);
    cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> El reporte " << nombre << " se creó con éxito \n";
}

char* formatohora(time_t * time) {
    char *str = (char*) malloc(20);
    strftime(str, 20, "%d/%m/%Y %H:%M:%S", localtime(time));
    return str;
}

void ReporteSb(Mount*particion, char nombre[25], char nombre2[25], char mm[500], char path[500]) {
    existeCarpeta(mm);
    char vdot[500] = "";
    strncpy(vdot, mm, 500);
    strcat(vdot, "/");
    strcat(vdot, nombre);
    char terminal[1000] = "";
    char auxPath[500] = "";
    strncpy(auxPath, path, 500);
    FILE *repo;
    repo = fopen(vdot, "w+");
    SuperBloque *superBloque = BuscarSuperBloque(particion);
    fprintf(repo, "digraph {\n");
    fprintf(repo, "graph[pad=\"0.5\", nodesep=\"0.5\", ranksep=\"2\"];\n");
    fprintf(repo, "node[shape=plain];\n");
    fprintf(repo, "rankdir=same;\n");

    fprintf(repo, "SB [\n");
    fprintf(repo, "shape=plaintext\n");
    fprintf(repo, "label=<\n");
    fprintf(repo, "<table border='0' cellborder='1' cellspacing='0' cellpadding='10'>\n");

    // Encabezado
    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td bgcolor='LightSeaGreen'><b>Nombre</b></td>\n");
    fprintf(repo, "<td bgcolor='LightSeaGreen'><b>Valor</b></td>\n");
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>s_inodes_count</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", superBloque->s_inodes_count);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>s_blocks_count</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", superBloque->s_blocks_count);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>s_free_blocks_count</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", superBloque->s_free_blocks_count);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>s_free_inodes_count</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", superBloque->s_free_inodes_count);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>s_mtime</b></td>\n");
    fprintf(repo, "<td>%s</td>\n", formatohora(&superBloque->s_mtime));
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>s_umtime</b></td>\n");
    if (superBloque->s_umtime == 0)
        fprintf(repo, "<td>%s</td>\n", "-");
    else
        fprintf(repo, "<td>%s</td>\n", formatohora(&superBloque->s_umtime));
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>s_mnt_count</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", superBloque->s_mnt_count);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>s_magic</b></td>\n");
    fprintf(repo, "<td>%s</td>\n", "0xEF53");
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>s_inode_size</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", superBloque->s_inode_size);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>s_block_size</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", superBloque->s_block_size);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>s_first_ino</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", superBloque->s_first_ino);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>s_first_blo</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", superBloque->s_first_blo);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>s_bm_inode_start</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", superBloque->s_bm_inode_start);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>s_bm_block_start</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", superBloque->s_bm_block_start);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>s_inode_start</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", superBloque->s_inode_start);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>s_block_start</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", superBloque->s_block_start);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "</table>\n");
    fprintf(repo, ">\n");
    fprintf(repo, "];\n");

    fprintf(repo, "}");
    fclose(repo);

    char ext[3] = "";
    strcpy(terminal, "dot ");
    for (int i = 0; i < 16; i++) {
        if (nombre2[i] == '.') {
            ext[0] = nombre2[i + 1];
            ext[1] = nombre2[i + 2];
            ext[2] = nombre2[i + 3];
            break;
        }
    }
    if (ext[0] == 'j' || ext[0] == 'J') {
        strcat(terminal, "-Tjpg ");
    } else {
        strcat(terminal, "-Tpdf ");
    }
    strcat(terminal, vdot);
    strcat(terminal, " -o ");
    strcat(terminal, auxPath);
    system(terminal);

}

void RecorreIReporte(Mount*particion, int id, FILE * repo) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinodo = sb->s_inode_start + id * sizeof (Inodo);
    Inodo *inodo = (Inodo*) readDisk(particion->path, pinodo, sizeof (Inodo));

    fprintf(repo, "INODO_%d", id);
    fprintf(repo, "[label=<\n");
    fprintf(repo, "<table border='0' cellborder='1' cellspacing='0'>\n");

    fprintf(repo, "<tr>\n"); // TITULO
    fprintf(repo, "<td COLSPAN='2' BGCOLOR='lightcoral'><b>INODO %d</b></td>\n", id);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>i_uid</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", inodo->i_uid);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>i_gid</b></td>\n");
    fprintf(repo, "<td>%s</td>\n", inodo->i_gid);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>i_size</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", inodo->i_size);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>i_atime</b></td>\n");
    fprintf(repo, "<td>%s</td>\n", formatohora(&inodo->i_atime));
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>i_ctime</b></td>\n");
    fprintf(repo, "<td>%s</td>\n", formatohora(&inodo->i_ctime));
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>i_mtime</b></td>\n");
    fprintf(repo, "<td>%s</td>\n", formatohora(&inodo->i_mtime));
    fprintf(repo, "</tr>\n");

    for (int i = 0; i < 15; i++) {
        fprintf(repo, "<tr>\n");
        fprintf(repo, "<td><b>i_block_%d</b></td>\n", i + 1);
        fprintf(repo, "<td port='%d'>%d</td>\n", i, inodo->i_block[i]);
        fprintf(repo, "</tr>\n");
    }

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>i_type</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", inodo->i_type);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td><b>i_perm</b></td>\n");
    fprintf(repo, "<td>%d</td>\n", inodo->i_perm);
    fprintf(repo, "</tr>\n");

    fprintf(repo, "</table>\n");
    fprintf(repo, ">];\n");
}

void ReporteInodo(Mount*particion, char nombre[25], char nombre2[25], char mm[500], char path[500]) {
    existeCarpeta(mm);
    char vdot[500] = "";
    strncpy(vdot, mm, 500);
    strcat(vdot, "/");
    strcat(vdot, nombre);
    char terminal[1000] = "";
    char auxPath[500] = "";
    strncpy(auxPath, path, 500);
    FILE *repo;
    repo = fopen(vdot, "w+");
    fprintf(repo, "digraph {\n");
    fprintf(repo, "graph[pad=\"0.5\", nodesep=\"0.5\", ranksep=\"2\"];\n");
    fprintf(repo, "node[shape=plain];\n");
    fprintf(repo, "rankdir=LR;\n");
    SuperBloque* sbloque = BuscarSuperBloque(particion);
    int inicio;
    inicio = sbloque->s_bm_inode_start;
    Bitmap* bm = (Bitmap*) readDisk(particion->path, inicio, sizeof (Bitmap));
    int lastID = -1;
    int currentID = 0;
    for (int i = 0; i < bm->size; i++) {
        if (bm->bitmaps[i] == '1') {
            if (lastID > -1) {
                fprintf(repo, "\n\nINODO_%d -> INODO_%d\n\n", lastID, currentID);
            }
            RecorreIReporte(particion, currentID, repo);
            lastID = currentID;
        }
        currentID++;
    }
    fprintf(repo, "}");
    fclose(repo);

    char ext[3] = "";
    strcpy(terminal, "dot ");
    for (int i = 0; i < 16; i++) {
        if (nombre2[i] == '.') {
            ext[0] = nombre2[i + 1];
            ext[1] = nombre2[i + 2];
            ext[2] = nombre2[i + 3];
            break;
        }
    }
    if (ext[0] == 'j' || ext[0] == 'J') {
        strcat(terminal, "-Tjpg ");
    } else {
        strcat(terminal, "-Tpdf ");
    }
    strcat(terminal, vdot);
    strcat(terminal, " -o ");
    strcat(terminal, auxPath);
    system(terminal);
}

void RecorreBArchivoReporte(Mount *particion, int id, FILE * repo) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pbloque = sb->s_block_start + id * sizeof (BloqueArchivos);
    BloqueArchivos *bloque = (BloqueArchivos*) readDisk(particion->path, pbloque, sizeof (BloqueArchivos));

    fprintf(repo, "BLOQUE_%d", id);
    fprintf(repo, "[label=<\n");
    fprintf(repo, "<table border='0' cellborder='1' cellspacing='0'>\n");
    fprintf(repo, "<tr>\n"); // TITULO
    fprintf(repo, "<td BGCOLOR='#ffcc00'><b>BLOQUE %d</b></td>\n", id);
    fprintf(repo, "</tr>\n");
    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td>%.64s</td>\n", bloque->b_content);
    fprintf(repo, "</tr>\n");
    fprintf(repo, "</table>\n");
    fprintf(repo, ">];\n");
}

void RecorreBCarpetasReporte(Mount *particion, int id, FILE * repo) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pbloque = sb->s_block_start + id * sizeof (BloqueCarpetas);
    BloqueCarpetas *bloque = (BloqueCarpetas*) readDisk(particion->path, pbloque, sizeof (BloqueCarpetas));

    fprintf(repo, "BLOQUE_%d", id);
    fprintf(repo, "[label=<\n");
    fprintf(repo, "<table border='0' cellborder='1' cellspacing='0'>\n");
    fprintf(repo, "<tr>\n"); // TITULO
    fprintf(repo, "<td COLSPAN='2' BGCOLOR='#47d147'><b>BLOQUE %d</b></td>\n", id);
    fprintf(repo, "</tr>\n");
    for (int i = 0; i < 4; i++) {
        fprintf(repo, "<tr>\n");
        if (i == 0 && (strncmp(bloque->b_content[1].b_name, "..", 2) == 0))
            fprintf(repo, "<td>%s</td>\n", ".");
        else
            fprintf(repo, "<td>%.12s</td>\n", bloque->b_content[i].b_name);
        fprintf(repo, "<td>%d</td>\n", bloque->b_content[i].b_inodo);
        fprintf(repo, "</tr>\n");
    }
    fprintf(repo, "</table>\n");
    fprintf(repo, ">];\n");

    for (int j = 0; j < 4; j++) {
        if ((strncmp(bloque->b_content[1].b_name, "..", 2) == 0) && (j == 0 || j == 1))
            continue;
        if (bloque->b_content[j].b_inodo > -1) {
            RecorreBReporte(particion, bloque->b_content[j].b_inodo, repo);
        }
    }
}

void RecorreBApuntadorReporte(Mount *particion, int id, FILE * repo) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pbloque = sb->s_block_start + id * sizeof (BloqueApuntador);
    BloqueApuntador *bloque = (BloqueApuntador*) readDisk(particion->path, pbloque, sizeof (BloqueApuntador));

    fprintf(repo, "BLOQUE_%d", id);
    fprintf(repo, "[label=<\n");
    fprintf(repo, "<table border='0' cellborder='1' cellspacing='0'>\n");
    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td BGCOLOR='#ff471a'><b>BLOQUE %d</b></td>\n", id);
    fprintf(repo, "</tr>\n");

    for (int i = 0; i < 16; i++) {
        fprintf(repo, "<tr>\n");
        fprintf(repo, "<td>%d</td>\n", bloque->b_pointers[i]);
        fprintf(repo, "</tr>\n");
    }

    fprintf(repo, "</table>\n");
    fprintf(repo, ">];\n");
}

void RBloqueSimple(Mount* particion, int id, FILE* repo, int tipo) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pbloque = sb->s_block_start + id * sizeof (BloqueApuntador);
    BloqueApuntador *btempo = (BloqueApuntador*) readDisk(particion->path, pbloque, sizeof (BloqueApuntador));

    RecorreBApuntadorReporte(particion, id, repo);

    for (int i = 0; i < 16; i++) {
        if (btempo->b_pointers[i] > -1) {
            if (tipo == 1) {
                RecorreBArchivoReporte(particion, btempo->b_pointers[i], repo);
            } else {
                RecorreBCarpetasReporte(particion, btempo->b_pointers[i], repo);
            }
        }
    }
}

void RecorreBReporte(Mount*particion, int id, FILE * repo) {

    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinodo = sb->s_inode_start + id * sizeof (Inodo);
    Inodo *inodo = (Inodo*) readDisk(particion->path, pinodo, sizeof (Inodo));

    for (int v = 0; v < 12; v++) {
        if (inodo->i_block[v] > -1) {
            if (inodo->i_type == 0) {
                RecorreBCarpetasReporte(particion, inodo->i_block[v], repo);
                continue;
            } else if (inodo->i_type == 1) {
                RecorreBArchivoReporte(particion, inodo->i_block[v], repo);
            }
        }
    }
    //aqui van los indirectos
    for (int x = 12; x < 15; x++) {
        if (inodo->i_block[x]>-1) {
            if (x == 12) {
                RBloqueSimple(particion, inodo->i_block[x], repo, inodo->i_type);
            }
        }
    }
}

void ReporteBloque(Mount*particion, char nombre[25], char nombre2[25], char mm[500], char path[500]) {
    existeCarpeta(mm);
    char vdot[500] = "";
    strncpy(vdot, mm, 500);
    strcat(vdot, "/");
    strcat(vdot, nombre);
    char terminal[1000] = "";
    char auxPath[500] = "";
    strncpy(auxPath, path, 500);
    FILE *repo;
    repo = fopen(vdot, "w+");
    fprintf(repo, "digraph {\n");
    fprintf(repo, "graph[pad=\"0.5\", nodesep=\"0.5\", ranksep=\"2\"];\n");
    fprintf(repo, "node[shape=plain];\n");
    fprintf(repo, "rankdir=same;\n");
    RecorreBReporte(particion, 0, repo);
    fprintf(repo, "}");
    fclose(repo);


    char ext[3] = "";
    strcpy(terminal, "dot ");
    for (int i = 0; i < 16; i++) {
        if (nombre2[i] == '.') {
            ext[0] = nombre2[i + 1];
            ext[1] = nombre2[i + 2];
            ext[2] = nombre2[i + 3];
            break;
        }
    }
    if (ext[0] == 'j' || ext[0] == 'J') {
        strcat(terminal, "-Tjpg ");
    } else {
        strcat(terminal, "-Tpdf ");
    }
    strcat(terminal, vdot);
    strcat(terminal, " -o ");
    strcat(terminal, auxPath);
    system(terminal);
}

void ReporteBArchivosTree(Mount* particion, FILE* repo, int id) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pbloque = sb->s_block_start + id * sizeof (BloqueArchivos);
    BloqueArchivos *bloque = (BloqueArchivos*) readDisk(particion->path, pbloque, sizeof (BloqueArchivos));

    fprintf(repo, "BLOQUE_%d", id);
    fprintf(repo, "[label=<\n");
    fprintf(repo, "<table border='0' cellborder='1' cellspacing='0'>\n");
    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td BGCOLOR='#ffcc00'><b>BLOQUE %d</b></td>\n", id);
    fprintf(repo, "</tr>\n");
    fprintf(repo, "<tr>\n");
    fprintf(repo, "<td>%.64s</td>\n", bloque->b_content);
    fprintf(repo, "</tr>\n");
    fprintf(repo, "</table>\n");
    fprintf(repo, ">];\n");
}

void ReporteBCarpetaTree(Mount* particion, FILE* archivo, int id) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinicio = sb->s_block_start + id * sizeof (BloqueCarpetas);
    BloqueCarpetas *bloque = (BloqueCarpetas*) readDisk(particion->path, pinicio, sizeof (BloqueCarpetas));

    fprintf(archivo, "BLOQUE_%d", id);
    fprintf(archivo, "[label=<\n");
    fprintf(archivo, "<table border='0' cellborder='1' cellspacing='0'>\n");
    fprintf(archivo, "<tr>\n"); // TITULO
    fprintf(archivo, "<td COLSPAN='2' BGCOLOR='%s'><b>BLOQUE %d</b></td>\n", "#47d147", id);
    fprintf(archivo, "</tr>\n");
    for (int i = 0; i < 4; i++) {
        fprintf(archivo, "<tr>\n");
        if (i == 0 && (strncmp(bloque->b_content[1].b_name, "..", 2) == 0))
            fprintf(archivo, "<td>%s</td>\n", ".");
        else
            fprintf(archivo, "<td>%.12s</td>\n", bloque->b_content[i].b_name);
        fprintf(archivo, "<td port='%d'>%d</td>\n", i, bloque->b_content[i].b_inodo);
        fprintf(archivo, "</tr>\n");
    }
    fprintf(archivo, "</table>\n");
    fprintf(archivo, ">];\n");

    for (int i = 0; i < 4; i++) {
        if ((strncmp(bloque->b_content[1].b_name, "..", 2) == 0) && (i == 0 || i == 1))
            continue;
        if (bloque->b_content[i].b_inodo > -1) {
            fprintf(archivo, "BLOQUE_%d:%d -> INODO_%d\n", id, i, bloque->b_content[i].b_inodo);
            ReporteInodotree(particion, archivo, bloque->b_content[i].b_inodo);
        }
    }
}

void ReporteBApuntadortree(Mount* particion, FILE* fp, int id, int nivel, int tipo) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinicio = sb->s_block_start + id * sizeof (BloqueApuntador);
    BloqueApuntador *bloque = (BloqueApuntador*) readDisk(particion->path, pinicio, sizeof (BloqueApuntador));
    fprintf(fp, "BLOQUE_%d", id);
    fprintf(fp, "[label=<\n");
    fprintf(fp, "<table border='0' cellborder='1' cellspacing='0'>\n");
    fprintf(fp, "<tr>\n"); // TITULO
    fprintf(fp, "<td BGCOLOR='#ff471a'><b>BLOQUE %d</b></td>\n", id);
    fprintf(fp, "</tr>\n");

    for (int i = 0; i < 16; i++) {
        fprintf(fp, "<tr>\n");
        fprintf(fp, "<td port='%d'>%d</td>\n", i, bloque->b_pointers[i]);
        fprintf(fp, "</tr>\n");
    }

    fprintf(fp, "</table>\n");
    fprintf(fp, ">];\n");

    for (int i = 0; i < 16; i++) {
        if (bloque->b_pointers[i] > -1) {
            fprintf(fp, "BLOQUE_%d:%d -> BLOQUE_%d\n", id, i, bloque->b_pointers[i]);
            switch (nivel) {
                case 1:
                    if (tipo == 1) {
                        ReporteBArchivosTree(particion, fp, bloque->b_pointers[i]);
                    } else if (tipo == 0) {
                        ReporteBCarpetaTree(particion, fp, bloque->b_pointers[i]);
                    }
                    break;
                case 2:
                case 3:
                    ReporteBApuntadortree(particion, fp, bloque->b_pointers[i], nivel - 1, tipo);
                default:
                    break;
            }
        }
    }
}

void ReporteInodotree(Mount* particion, FILE* archivo, int id) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinodo = sb->s_inode_start + id * sizeof (Inodo);
    Inodo *inodo = (Inodo*) readDisk(particion->path, pinodo, sizeof (Inodo));

    fprintf(archivo, "INODO_%d", id);
    fprintf(archivo, "[label=<\n");
    fprintf(archivo, "<table border='0' cellborder='1' cellspacing='0'>\n");

    fprintf(archivo, "<tr>\n"); // TITULO
    fprintf(archivo, "<td COLSPAN='2' BGCOLOR='%s'><b>INODO %d</b></td>\n", "#3399ff", id);
    fprintf(archivo, "</tr>\n");

    fprintf(archivo, "<tr>\n");
    fprintf(archivo, "<td><b>i_uid</b></td>\n");
    fprintf(archivo, "<td>%d</td>\n", inodo->i_uid);
    fprintf(archivo, "</tr>\n");

    fprintf(archivo, "<tr>\n");
    fprintf(archivo, "<td><b>i_gid</b></td>\n");
    fprintf(archivo, "<td>%s</td>\n", inodo->i_gid);
    fprintf(archivo, "</tr>\n");

    fprintf(archivo, "<tr>\n");
    fprintf(archivo, "<td><b>i_size</b></td>\n");
    fprintf(archivo, "<td>%d</td>\n", inodo->i_size);
    fprintf(archivo, "</tr>\n");

    fprintf(archivo, "<tr>\n");
    fprintf(archivo, "<td><b>i_atime</b></td>\n");
    fprintf(archivo, "<td>%s</td>\n", formatohora(&inodo->i_atime));
    fprintf(archivo, "</tr>\n");

    fprintf(archivo, "<tr>\n");
    fprintf(archivo, "<td><b>i_ctime</b></td>\n");
    fprintf(archivo, "<td>%s</td>\n", formatohora(&inodo->i_ctime));
    fprintf(archivo, "</tr>\n");

    fprintf(archivo, "<tr>\n");
    fprintf(archivo, "<td><b>i_mtime</b></td>\n");
    fprintf(archivo, "<td>%s</td>\n", formatohora(&inodo->i_mtime));
    fprintf(archivo, "</tr>\n");

    for (int i = 0; i < 15; i++) {
        fprintf(archivo, "<tr>\n");
        fprintf(archivo, "<td><b>i_block_%d</b></td>\n", i + 1);
        fprintf(archivo, "<td port='%d'>%d</td>\n", i, inodo->i_block[i]);
        fprintf(archivo, "</tr>\n");
    }

    fprintf(archivo, "<tr>\n");
    fprintf(archivo, "<td><b>i_type</b></td>\n");
    fprintf(archivo, "<td>%d</td>\n", inodo->i_type);
    fprintf(archivo, "</tr>\n");

    fprintf(archivo, "<tr>\n");
    fprintf(archivo, "<td><b>i_perm</b></td>\n");
    fprintf(archivo, "<td>%d</td>\n", inodo->i_perm);
    fprintf(archivo, "</tr>\n");

    fprintf(archivo, "</table>\n");
    fprintf(archivo, ">];\n");

    for (int i = 0; i < 12; i++) {
        if (inodo->i_block[i] > -1) {
            fprintf(archivo, "INODO_%d:%d -> BLOQUE_%d\n", id, i, inodo->i_block[i]);
            if (inodo->i_type == 0) {
                ReporteBCarpetaTree(particion, archivo, inodo->i_block[i]);
            } else if (inodo->i_type == 1) {
                ReporteBArchivosTree(particion, archivo, inodo->i_block[i]);
            }
        }
    }
    //indirectos
    int nivel = 1;
    for (int i = 12; i < 15; i++) {
        if (inodo->i_block[i] > -1) {
            fprintf(archivo, "INODO_%d:%d -> BLOQUE_%d\n", id, i, inodo->i_block[i]);
            ReporteBApuntadortree(particion, archivo, inodo->i_block[i], nivel, inodo->i_type);
        }
        nivel++;
    }
}

void ReporteTree(Mount*particion, char nombre[25], char nombre2[25], char mm[500], char path[500]) {
    existeCarpeta(mm);
    char vdot[500] = "";
    strncpy(vdot, mm, 500);
    strcat(vdot, "/");
    strcat(vdot, nombre);
    char terminal[1000] = "";
    char auxPath[500] = "";
    strncpy(auxPath, path, 500);
    FILE *fp;
    fp = fopen(vdot, "w+");
    fprintf(fp, "digraph {\n");
    fprintf(fp, "graph[pad=\"0.5\", nodesep=\"0.5\", ranksep=\"2\"];\n");
    fprintf(fp, "node[shape=plain];\n");
    fprintf(fp, "rankdir=LR;\n");
    ReporteInodotree(particion, fp, 0);
    fprintf(fp, "}");
    fclose(fp);

    char ext[3] = "";
    strcpy(terminal, "dot ");
    for (int i = 0; i < 16; i++) {
        if (nombre2[i] == '.') {
            ext[0] = nombre2[i + 1];
            ext[1] = nombre2[i + 2];
            ext[2] = nombre2[i + 3];
            break;
        }
    }
    if (ext[0] == 'j' || ext[0] == 'J') {
        strcat(terminal, "-Tjpg ");
    } else {
        strcat(terminal, "-Tpdf ");
    }
    strcat(terminal, vdot);
    strcat(terminal, " -o ");
    strcat(terminal, auxPath);
    system(terminal);
}

void ReporteFile(Mount* particion, char nombre[25], char nombre2[25], char mm[500], char path[500], char ruta [500]) {

    FILE *fp;
    fp = fopen(path, "w+");

    int id = idCarpeta(particion, ruta);
    if (id < 0) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no se encontro el inodo para el reporte file !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        return;
    }

    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinicio = sb->s_inode_start + id * sizeof (Inodo);
    Inodo* inodo = (Inodo*) readDisk(particion->path, pinicio, sizeof (Inodo));
    for (int i = 0; i < 12; i++) {
        if (inodo->i_block[i] == -1) {
            continue;
        }
        RBArchivo(particion, inodo->i_block[i], fp);
    }
    fclose(fp);
    //indirectos
    for (int i = 12; i < 15; i++) {
        if (inodo->i_block[i] > -1) {
            if (i == 12)
                FileSimple(particion, inodo->i_block[i], fp);
            else if (i == 13)
                FileDoble(particion, inodo->i_block[i], fp);
            else if (i == 14)
                FileTriple(particion, inodo->i_block[i], fp);
        }
    }
}

char *getPData(int permiso) {

    char *pdata = (char*) malloc(sizeof (char)*4);
    switch (permiso) {
        case 0:
            strncpy(pdata, "---", 3);
            for (int i = 3; i<sizeof (pdata); i++) {
                pdata[i] = '\0';
            }
            break;
        case 1:
            strncpy(pdata, "--x", 3);
            for (int i = 3; i<sizeof (pdata); i++) {
                pdata[i] = '\0';
            }
            break;
        case 2:
            strncpy(pdata, "-w-", 3);
            for (int i = 3; i<sizeof (pdata); i++) {
                pdata[i] = '\0';
            }
            break;
        case 3:
            strncpy(pdata, "-wx", 3);
            for (int i = 3; i<sizeof (pdata); i++) {
                pdata[i] = '\0';
            }
            break;
        case 4:
            strncpy(pdata, "r--", 3);
            for (int i = 3; i<sizeof (pdata); i++) {
                pdata[i] = '\0';
            }
            break;
        case 5:
            strncpy(pdata, "r-x", 3);
            for (int i = 3; i<sizeof (pdata); i++) {
                pdata[i] = '\0';
            }
            break;
        case 6:
            strncpy(pdata, "rw-", 3);
            for (int i = 3; i<sizeof (pdata); i++) {
                pdata[i] = '\0';
            }
            break;
        case 7:
            strncpy(pdata, "rwx", 3);
            for (int i = 3; i<sizeof (pdata); i++) {
                pdata[i] = '\0';
            }
            break;
            break;

    }
    return pdata;
}

char *getPermisos(int permiso) {
    int usuario = permiso / 100;
    int grupo = permiso / 10 % 10;
    int otros = permiso % 10;
    char *vmp = (char*) malloc(sizeof (char)*11);
    sprintf(vmp, "-%s %s %s", getPData(usuario), getPData(grupo), getPData(otros));
    vmp[10] = '\0';
    return vmp;
}

void TBloque(Mount* particion, int id, FILE * fp) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinodo = sb->s_block_start + id * sizeof (BloqueCarpetas);
    BloqueCarpetas *bloque = (BloqueCarpetas*) readDisk(particion->path, pinodo, sizeof (BloqueCarpetas));
    for (int i = 0; i < 4; i++) {
        if ((bloque->b_content[i].b_inodo == -1) || ((strncmp(bloque->b_content[1].b_name, "..", 2) == 0) && (i == 0 || i == 1))) {
            continue;
        }
        int idnuevo = bloque->b_content[i].b_inodo;
        int pinodo = sb->s_inode_start + idnuevo * sizeof (Inodo);
        Inodo *inodoArchivo = (Inodo*) readDisk(particion->path, pinodo, sizeof (Inodo));
        fprintf(fp, "<tr>\n");
        fprintf(fp, "<td>%s</td>\n", getPermisos(inodoArchivo->i_perm));
        fprintf(fp, "<td>%d</td>\n", inodoArchivo->i_uid);
        fprintf(fp, "<td>%s</td>\n", inodoArchivo->i_gid);
        fprintf(fp, "<td>%d</td>\n", inodoArchivo->i_size);
        fprintf(fp, "<td>%s</td>\n", formatohora(&inodoArchivo->i_ctime));
        fprintf(fp, "<td>%s</td>\n", formatohora(&inodoArchivo->i_mtime));
        if (inodoArchivo->i_type == 1) {
            fprintf(fp, "<td>%s</td>\n", "Archivo");
        } else {
            fprintf(fp, "<td>%s</td>\n", "Carpeta");
        }
        fprintf(fp, "<td>%s</td>\n", bloque->b_content[i].b_name);
        fprintf(fp, "</tr>\n");
    }
}

void TSimple(Mount* particion, int id, FILE * fp) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinodo = sb->s_block_start + id * sizeof (BloqueApuntador);
    BloqueApuntador *bloque = (BloqueApuntador*) readDisk(particion->path, pinodo, sizeof (BloqueApuntador));
    for (int i = 0; i < 16; i++) {
        if (bloque->b_pointers[i] > -1) {
            TBloque(particion, bloque->b_pointers[i], fp);
        }
    }
}

void TDoble(Mount* particion, int id, FILE * fp) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinodo = sb->s_block_start + id * sizeof (BloqueApuntador);
    BloqueApuntador *bloque = (BloqueApuntador*) readDisk(particion->path, pinodo, sizeof (BloqueApuntador));
    for (int i = 0; i < 16; i++) {
        if (bloque->b_pointers[i] > -1) {
            TSimple(particion, bloque->b_pointers[i], fp);
        }
    }
}

void TTriple(Mount* particion, int id, FILE * fp) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinodo = sb->s_block_start + id * sizeof (BloqueApuntador);
    BloqueApuntador *bloque = (BloqueApuntador*) readDisk(particion->path, pinodo, sizeof (BloqueApuntador));
    for (int i = 0; i < 16; i++) {
        if (bloque->b_pointers[i] > -1) {
            TDoble(particion, bloque->b_pointers[i], fp);
        }
    }
}

void TInodo(Mount* particion, int id, FILE * fp) {
    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinodo = sb->s_inode_start + id * sizeof (Inodo);
    Inodo *inodo = (Inodo*) readDisk(particion->path, pinodo, sizeof (Inodo));
    if (inodo->i_type == 1) {
        return;
    }

    for (int i = 0; i < 12; i++) {
        if (inodo->i_block[i] != -1) {
            TBloque(particion, inodo->i_block[i], fp);
        }
    }
    //indirectos
    for (int i = 12; i < 15; i++) {
        if (inodo->i_block[i] == -1)
            continue;
        if (i == 12) {
            TSimple(particion, inodo->i_block[i], fp);
        }
        if (i == 13) {
            TDoble(particion, inodo->i_block[i], fp);
        }
        if (i == 14) {
            TTriple(particion, inodo->i_block[i], fp);
        }
    }
}

void TablaLS(Mount* particion, char ruta[500], FILE * fp) {
    int inodoID = idCarpeta(particion, ruta);
    if (inodoID == -1) {
        printf("---> Error, no se encontró %s\n", ruta);
        return;
    }

    SuperBloque* sb = BuscarSuperBloque(particion);
    int pinodo = sb->s_inode_start + inodoID * sizeof (Inodo);
    Inodo *inodo = (Inodo*) readDisk(particion->path, pinodo, sizeof (Inodo));

    if (inodo->i_type == 1) {
        printf("---> Error, se esperaba la ruta de una carpeta\n");
        return;
    }
    fprintf(fp, "LS [\n");
    fprintf(fp, "shape=plaintext\n");
    fprintf(fp, "label=<\n");
    fprintf(fp, "<table border='0' cellborder='1' cellspacing='0' cellpadding='10'>\n");

    fprintf(fp, "<tr>\n");
    fprintf(fp, "<td><b>Permisos</b></td>\n");
    fprintf(fp, "<td><b>IdUsuario</b></td>\n");
    fprintf(fp, "<td><b>Grupo</b></td>\n");
    fprintf(fp, "<td><b>Size (bytes)</b></td>\n");
    fprintf(fp, "<td><b>Fecha y Hora (Creación)</b></td>\n");
    fprintf(fp, "<td><b>Fecha y Hora (Modificación)</b></td>\n");
    fprintf(fp, "<td><b>Tipo</b></td>\n");
    fprintf(fp, "<td><b>Name</b></td>\n");
    fprintf(fp, "</tr>\n");

    TInodo(particion, inodoID, fp);
    fprintf(fp, "</table>\n");
    fprintf(fp, ">\n");
    fprintf(fp, "];\n");
}

void ReporteLS(Mount* particion, char nombre[25], char nombre2[25], char mm[500], char path[500], char ruta [500]) {
    existeCarpeta(mm);
    char vdot[500] = "";
    strncpy(vdot, mm, 500);
    strcat(vdot, "/");
    strcat(vdot, nombre);
    char terminal[1000] = "";
    char auxPath[500] = "";
    strncpy(auxPath, path, 500);
    FILE *fp;
    fp = fopen(vdot, "w+");
    fprintf(fp, "digraph {\n");
    fprintf(fp, "graph[pad=\"0.5\", nodesep=\"0.5\", ranksep=\"2\"];\n");
    fprintf(fp, "node[shape=plain];\n");
    TablaLS(particion, ruta, fp);
    fprintf(fp, "}");
    fclose(fp);
    char ext[3] = "";
    strcpy(terminal, "dot ");
    for (int i = 0; i < 16; i++) {
        if (nombre2[i] == '.') {
            ext[0] = nombre2[i + 1];
            ext[1] = nombre2[i + 2];
            ext[2] = nombre2[i + 3];
            break;
        }
    }
    if (ext[0] == 'j' || ext[0] == 'J') {
        strcat(terminal, "-Tjpg ");
    } else {
        strcat(terminal, "-Tpdf ");
    }
    strcat(terminal, vdot);
    strcat(terminal, " -o ");
    strcat(terminal, auxPath);
    system(terminal);

}

void ReporteJournal(Mount*particion, char nombre[25], char nombre2[25], char mm[500], char path[500]) {
    existeCarpeta(mm);
    char vdot[500] = "";
    strncpy(vdot, mm, 500);
    strcat(vdot, "/");
    strcat(vdot, nombre);
    char terminal[1000] = "";
    char auxPath[500] = "";
    strncpy(auxPath, path, 500);
    FILE *fp;
    fp = fopen(vdot, "w+");
    fprintf(fp, "digraph {\n");
    fprintf(fp, "graph[pad=\"0.5\", nodesep=\"0.5\", ranksep=\"2\"];\n");
    fprintf(fp, "node[shape=plain];\n");
    fprintf(fp, "Journaling [\n");
    fprintf(fp, "shape=plaintext\n");
    fprintf(fp, "label=<\n");
    fprintf(fp, "<table border='0' cellborder='1' cellspacing='0' cellpadding='10'>\n");
    // Encabezado
    fprintf(fp, "<tr>\n");
    fprintf(fp, "<td BGCOLOR='lightcoral'><b>Tipo Operación</b></td>\n");
    fprintf(fp, "<td BGCOLOR='lightcoral'><b>Tipo</b></td>\n");
    fprintf(fp, "<td BGCOLOR='lightcoral'><b>Nombre</b></td>\n");
    fprintf(fp, "<td BGCOLOR='lightcoral'><b>Contenido</b></td>\n");
    fprintf(fp, "<td BGCOLOR='lightcoral'><b>Fecha</b></td>\n");
    fprintf(fp, "</tr>\n");
    SuperBloque* sb = BuscarSuperBloque(particion);
    int idactual = 0;
    int pjournal = sb->s_journal_start + idactual * sizeof (Journal);
    Journal *aux = (Journal*) readDisk(particion->path, pjournal, sizeof (Journal));
    while (aux->tipo_operacion[0] != '\0') {
        fprintf(fp, "<tr>\n");
        fprintf(fp, "<td>%s</td>\n", aux->tipo_operacion);
        if (aux->tipo != NULL) {
            fprintf(fp, "<td>%s</td>\n", aux->tipo);
        }
        fprintf(fp, "<td>%s</td>\n", aux->nombre);
        if (strlen(aux->contenido) == 0)
            fprintf(fp, "<td>%s</td>\n", "---");
        else
            fprintf(fp, "<td>%s</td>\n", aux->contenido);

        fprintf(fp, "<td>%s</td>\n", formatohora(&aux->fecha));
        fprintf(fp, "</tr>\n");

        idactual++;
        pjournal = sb->s_journal_start + idactual * sizeof (Journal);
        aux = (Journal*) readDisk(particion->path, pjournal, sizeof (Journal));
    }
    fprintf(fp, "</table>\n");
    fprintf(fp, ">\n");
    fprintf(fp, "];\n");
    fprintf(fp, "}");
    fclose(fp);
    char ext[3] = "";
    strcpy(terminal, "dot ");
    for (int i = 0; i < 16; i++) {
        if (nombre2[i] == '.') {
            ext[0] = nombre2[i + 1];
            ext[1] = nombre2[i + 2];
            ext[2] = nombre2[i + 3];
            break;
        }
    }
    if (ext[0] == 'j' || ext[0] == 'J') {
        strcat(terminal, "-Tjpg ");
    } else {
        strcat(terminal, "-Tpdf ");
    }
    strcat(terminal, vdot);
    strcat(terminal, " -o ");
    strcat(terminal, auxPath);
    system(terminal);
}

void CreandoReporte(char name[16], char id[16], char vpath[500], char carpeDot[500], char nombre [25], char nombre2[25], char rutals[500]) {
    char vname[16] = "";
    strncpy(vname, name, 16);
    char vid[16] = "";
    strncpy(vid, id, 16);
    char varpath[500] = "";
    strncpy(varpath, vpath, 500);
    char vcarpeDot[500] = "";
    strncpy(vcarpeDot, carpeDot, 500);
    Mount *TempoNode;
    TempoNode = primero;
    while (TempoNode != NULL) {
        if (strcasecmp(TempoNode->id, vid) == 0) {
            break;
        }
        TempoNode = TempoNode->next;
    }

    if (TempoNode != NULL) {
        MBR *Reporte1 = (MBR*) readDisk(TempoNode->path, 0, sizeof (MBR));
        if (Reporte1 != NULL) {
            if (strcasecmp(vname, "disk") == 0) {
                ReporteDisco(Reporte1, varpath, vcarpeDot, nombre, TempoNode->path, nombre2);
            } else if (strcasecmp(vname, "mbr") == 0) {
                ReporteMBR(Reporte1, varpath, vcarpeDot, nombre, TempoNode->path, nombre2);
            } else if (strcasecmp(vname, "bm_inode") == 0) {
                ReporteBMInode(TempoNode, nombre, nombre2, vcarpeDot, varpath);
            } else if (strcasecmp(vname, "bm_block") == 0) {
                ReporteBMBlock(TempoNode, nombre, nombre2, vcarpeDot, varpath);
            } else if (strcasecmp(vname, "sb") == 0) {
                ReporteSb(TempoNode, nombre, nombre2, vcarpeDot, varpath);
            } else if (strcasecmp(vname, "inode") == 0) {
                ReporteInodo(TempoNode, nombre, nombre2, vcarpeDot, varpath);
            } else if (strcasecmp(vname, "block") == 0) {
                ReporteBloque(TempoNode, nombre, nombre2, vcarpeDot, varpath);
            } else if (strcasecmp(vname, "file") == 0) {
                ReporteFile(TempoNode, nombre, nombre2, vcarpeDot, varpath, rutals);
            } else if (strcasecmp(vname, "ls") == 0) {
                ReporteLS(TempoNode, nombre, nombre2, vcarpeDot, varpath, rutals);
            } else if (strcasecmp(vname, "tree") == 0) {
                ReporteTree(TempoNode, nombre, nombre2, vcarpeDot, varpath);
            } else if (strcasecmp(vname, "journaling") == 0) {
                ReporteJournal(TempoNode, nombre, nombre2, vcarpeDot, varpath);
            } else {
                //                generarTreeReport(TempoNode, varpath, vcarpeDot, nombre, TempoNode->path, nombre2);
            }
        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!1 Error no se encontró el disco para el reporte !!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
        }
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error No se encontró el montaje para el reporte !!!!!!!!!!!!!!!!!!!!!!!!!!!!!11 \n";
    }
}

void Reporte(char name2[16], char id[16], char varpath[500], char ruta[500], int bande1, int bande2, int bande3) {
    if (bande1 == 1 && bande2 == 1 && bande3 == 1) {
        existeCarpeta(varpath);
        char auxpath4[500] = "";
        strncpy(auxpath4, varpath, 500);
        char *tempo1 = strdup(auxpath4);
        char *tempo2 = strdup(auxpath4);
        char *pathc = dirname(tempo2);
        char *NDisco = basename(tempo1);
        char name[25] = "";
        char name3[25] = "";
        strncpy(name3, NDisco, 25);
        strncpy(name, NDisco, 25);
        char *cut = strtok(name, ".");
        strcat(name, ".dot");
        CreandoReporte(name2, id, varpath, pathc, name, name3, ruta);
    } else {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!Error falta un parametro obligarotio en el comando rep !!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
    }
}

void analizaRep(char comando[10000]) {
    int bande1 = 0;
    int bande2 = 0;
    int bande3 = 0;
    char *varpath;
    char *id;
    char *name;
    char *ruta;
    char *cut = strtok(comando, " ");
    cut = strtok(NULL, " =");
    while (cut != NULL) {
        if (strcasecmp(cut, "-id") == 0) {
            cut = strtok(NULL, "= ");
            bande1 = 1;
            id = cut;
        } else if (strcasecmp(cut, "-name") == 0) {
            cut = strtok(NULL, "= ");
            if ((strcasecmp(cut, "mbr") == 0) || (strcasecmp(cut, "disk") == 0) || (strcasecmp(cut, "tree") == 0) || (strcasecmp(cut, "inode") == 0) || (strcasecmp(cut, "block") == 0) || (strcasecmp(cut, "journaling") == 0) || (strcasecmp(cut, "bm_inode") == 0) || (strcasecmp(cut, "bm_block") == 0) || (strcasecmp(cut, "sb") == 0) || (strcasecmp(cut, "file") == 0) || (strcasecmp(cut, "ls") == 0)) {
                bande3 = 1;
                name = cut;
            } else {
                cout << "!!!!!!!!!!!!!!!!!!!! Error debe llevar mbr o disk !!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            }
        } else if (strcasecmp(cut, "-path") == 0) {
            cut = strtok(NULL, "= ");
            bande2 = 1;
            char aux[1000];
            if (*cut != '\"') {
                varpath = cut;
                //cout<<"La ruta del path es: "varpath"\n";
            } else {
                varpath = quitaCm(cut);
                //cout<<"La ruta del path es: "varpath"\n";
            }

        } else if (strcasecmp(cut, "-ruta") == 0) {
            cut = strtok(NULL, "= ");
            ruta = cut;
        } else {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!! existe un error comando desconocido de Rep " << cut << " !!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";

            break;
        }
        cut = strtok(NULL, " =");
    }
    Reporte(name, id, varpath, ruta, bande1, bande2, bande3);
}

void analizador2(char cadena3[10000]) {
    char cadenaux[10000] = "";
    if (strcasecmp(cadena3, "\n") == 0 || (cadena3[0] == ' ') || (cadena3 == NULL) || cadena3[0] == NULL) {
    } else {
        strcpy(cadenaux, cadena3);
        cout << "\n";
        char *cut;
        cut = strtok(cadena3, " ");

        if (strcasecmp(cadena3, "Mkdisk") == 0) {
            analizaMkdisk(cadenaux);

        } else if (strcasecmp(cadena3, "Rmdisk") == 0) {
            analizaRmdisk(cadenaux);

        } else if (strcasecmp(cadena3, "fdisk") == 0) {
            analizFdisk(cadenaux);
        } else if (strcasecmp(cadena3, "mount") == 0) {
            analizaMount(cadenaux);
        } else if (strcasecmp(cadena3, "unmount") == 0) {
            analizaUnmount(cadenaux);
        } else if (strcasecmp(cadena3, "rep") == 0) {
            analizaRep(cadenaux);
        } else if (strcasecmp(cadena3, "mkfs") == 0) {
            analizamkfs(cadenaux);
        } else if (strcasecmp(cadena3, "login") == 0) {
            analizaLogin(cadenaux);
        } else if (strcasecmp(cadena3, "logout") == 0) {
            if (actual.uid != 0) {
                actual.uid = 0;

                cout << ">>>>>>>>>>>>>>>>>>>>>>>>>> Se cerro sesión correctamente \n";
            } else {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!! Error no hay ninguna sesión iniciada !!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            }

        } else if (strcasecmp(cadena3, "mkgrp") == 0) {
            analizamkgrp(cadenaux);
        } else if (strcasecmp(cadena3, "rmgrp") == 0) {
            analizarmgrp(cadenaux);
        } else if (strcasecmp(cadena3, "mkusr") == 0) {
            analizamkusr(cadenaux);

        } else if (strcasecmp(cadena3, "rmusr") == 0) {
            analizarmusr(cadenaux);
        } else if (strcasecmp(cadena3, "mkfile") == 0) {
            analizamkfile(cadenaux);
        } else if (strcasecmp(cadena3, "mkdir") == 0) {
            analizamkdir(cadenaux);
        } else if (strcasecmp(cadena3, "cat") == 0) {
            analizaCat(cadenaux);
        } else if (strcasecmp(cadena3, "pause") == 0) {
            char cc[3] = "";
            cout << "Presione cualquier tecla para continuar \n";
            cout << "\n";
            scanf("%c", cc);
        } else if (strcasecmp(cadena3, "loss") == 0) {
            analizaLoss(cadenaux);
        } else if (strcasecmp(cadena3, "recovery") == 0) {
            analizaRecovery(cadenaux);
        } else {

            cout << "!!!!!!!!!!!!!!!!!!!!!!!!Error el comando ingresado no es correcto !!!!!!!!!!!!!!!!!!!!!!!!! \n";
        }
    }

}

void CargarArchivo(char varpath[500]) {
    char auxpath[500] = "";
    char concatenar[100000] = "";
    int contador = 0;
    strncpy(auxpath, varpath, 500);
    FILE *archivo;
    archivo = fopen(varpath, "r");
    char carActual;
    bool comentarioLinea = false;
    bool comentarioMulti = false;
    bool bandemulti = false;
    bool bandemulti2 = false;
    int i = 0;
    bool pase = true;
    int pase2 = 0;
    if (archivo != NULL) {
        while ((carActual = fgetc(archivo)) != EOF) {
            if (carActual == '\n') {
                analizador2(concatenar);
                // cout<<"comando "<<concatenar;
                char Limpi[100000];
                strncpy(concatenar, Limpi, 100000);
                contador = 0;
            } else {
                if (carActual == '\r' || carActual == '\t' || carActual == '\r\n') {
                } else {
                    concatenar[contador] = carActual;
                    contador++;
                }
            }

        }
        fclose(archivo);
    } else {

        cout << "no se encontro el archivo";
    }
}

void comandoExec(char comando[10000]) {
    char *cut = strtok(comando, " ");
    char *varpath;
    cut = strtok(NULL, " =");
    if (strcasecmp(cut, "-path") == 0) {
        cut = strtok(NULL, "= ");
        char aux[1000];
        if (*cut != '\"') {
            // cout<<"La ruta del path es: "<<cut<<"\n";
            varpath = cut;
        } else {
            varpath = quitaCm(cut);
        }
    } else {

        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Error no viene el parametro path, en el comando Exec !!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
    }
    CargarArchivo(varpath);
}

void analizador(char cadena3[10000]) {
    char cadenaux[10000] = "";
    if (strcasecmp(cadena3, "\n") == 0 || (cadena3[0] == ' ') || (cadena3 == NULL) || cadena3[0] == NULL) {
    } else {
        strcpy(cadenaux, cadena3);
        cout << "\n";
        char *cut;
        cut = strtok(cadena3, " ");

        if (strcasecmp(cadena3, "Mkdisk") == 0) {
            analizaMkdisk(cadenaux);

        } else if (strcasecmp(cadena3, "Rmdisk") == 0) {
            analizaRmdisk(cadenaux);

        } else if (strcasecmp(cadena3, "fdisk") == 0) {
            analizFdisk(cadenaux);
        } else if (strcasecmp(cadena3, "mount") == 0) {
            analizaMount(cadenaux);
        } else if (strcasecmp(cadena3, "unmount") == 0) {
            analizaUnmount(cadenaux);
        } else if (strcasecmp(cadena3, "rep") == 0) {
            analizaRep(cadenaux);
        } else if (strcasecmp(cadena3, "exec") == 0) {
            comandoExec(cadenaux);
        } else if (strcasecmp(cadena3, "mkfs") == 0) {
            analizamkfs(cadenaux);
        } else if (strcasecmp(cadena3, "login") == 0) {
            analizaLogin(cadenaux);
        } else if (strcasecmp(cadena3, "logout") == 0) {
            if (actual.uid != 0) {
                actual.uid = 0;
                cout << ">>>>>>>>>>>>>>>>>>>>>>>>>> Se inicio sesión correctamente \n";
            } else {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!! Error no hay ninguna sesión iniciada !!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            }
        } else if (strcasecmp(cadena3, "mkgrp") == 0) {
            analizamkgrp(cadenaux);
        } else if (strcasecmp(cadena3, "rmgrp") == 0) {
            analizarmgrp(cadenaux);
        } else if (strcasecmp(cadena3, "mkusr") == 0) {
            analizamkusr(cadenaux);
        } else if (strcasecmp(cadena3, "rmusr") == 0) {
            analizarmusr(cadenaux);
        } else if (strcasecmp(cadena3, "mkfile") == 0) {
            analizamkfile(cadenaux);
        } else if (strcasecmp(cadena3, "mkdir") == 0) {
            analizamkdir(cadenaux);
        } else if (strcasecmp(cadena3, "cat") == 0) {
            analizaCat(cadenaux);
        } else if (strcasecmp(cadena3, "pause") == 0) {
            char cc[3] = "";
            cout << "Presione cualquier tecla para continuar \n";
            cout << "\n";
            scanf("%c", cc);
        } else {

            cout << "!!!!!!!!!!!!!!!!!!!!!!!!Error el comando ingresado no es correcto !!!!!!!!!!!!!!!!!!!!!!!!! \n";
        }
    }

}

int main(int argc, char** argv) {
    cout << "\n Nombre: Josseline Suseth Godinez Garcia";
    cout << "\n Carne : 201503841";
    cout << "\n Curso : Manejo E Implementación de Archivos";
    cout << "\n Sección : A-";
    cout << "\n-----------------------------------------------------------------\n\n";

    int op = 0;
    while (op != 2) {
        char cadena2[1000] = "";
        cout << "1. Ingresar Comando \n";
        cout << "2. Salir \n\n";
        scanf("%d", &op);
        switch (op) {
            case 1:
                cout << "\n ************ Bienvenido ingrese un comando para iniciar ************* \n";
                cout << "\n";
                cin.ignore();
                cin.getline(cadena2, 10000, '\n');
                analizador(cadena2);

                break;
        }
    }
    return 0;
}


