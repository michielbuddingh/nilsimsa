#pragma once

#include "nilsimsa.h"

void filltran(void);
void makecode(struct nsrecord *a);
void codetostr(struct nsrecord *a,char *str);
int codeorfile(struct nsrecord *a,char *str,int mboxflag);
int accfile(FILE *file,struct nsrecord *a,int mboxflag);
