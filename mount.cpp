/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   Mount.cpp
 * Author: heidy
 *
 * Created on 22 de febrero de 2020, 19:23
 */

#include "mount.h"

Mount::Mount() {
}

Mount:: Mount(char part_type_,char path_[500], char name_[5],char part_status_, char part_fit_,int part_start_, int part_size_, char part_name_[16], Mount *next_, Mount *prev_){
    part_type=part_type_;
    part_status=part_status_;
    part_fit=part_fit_;
    part_start=part_start_;
    part_size=part_size_;
    next=next_;
    prev=prev_;
    int j;
    for(j=0;j<5;j++){
        name[j]=name_[j];
    }
    int i;
    for(i=0;i<500;i++){
        path[i]=path_[i];
    }
    int y;
    for(y=0;y<16;y++){
        part_name[y]=part_name_[y];
    }
}

Mount::Mount(const Mount& orig) {
}

Mount::~Mount() {
}

