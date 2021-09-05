
#include "bitmap.h"
#include <cstdlib>

Bitmap::Bitmap() {
}

Bitmap::Bitmap(int size_, int tipo_){
    char *bitmaps_ = (char*)malloc(sizeof(char)*size_);
    for(int i = 0; i < size_; i++){
        bitmaps_[i] = '0';
    }
    bitmaps = bitmaps_;
    size = size_;
    tipo = tipo_;
}

Bitmap::Bitmap(const Bitmap& orig) {
}

Bitmap::~Bitmap() {
}
