#ifndef BITMAP_H
#define BITMAP_H

class Bitmap {
public:
    char     *bitmaps;
    int      size;
    int      tipo;
    Bitmap();
    Bitmap(int size_, int tipo_);
    Bitmap(const Bitmap& orig);
    virtual ~Bitmap();
private:

};

#endif /* BITMAP_H */
