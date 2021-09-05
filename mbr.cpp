
#include "mbr.h"

MBR::MBR() {
    mbr_tamano=0;
    mbr_disk_signature=0;
}

MBR::MBR(int mbr_tamano_, int mbr_disk_signature_, char disk_fit_) {
    mbr_tamano=mbr_tamano_;
    mbr_disk_signature=mbr_disk_signature_;
    disk_fit=disk_fit_;
}


MBR::~MBR() {
}

