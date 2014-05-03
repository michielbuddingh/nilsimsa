/***************************************************************************
                         cluster.c  -  finds clusters
                             -------------------
    begin                : Mon May 14 2001
    copyright            : (C) 2001 by cmeclax
    email                : cmeclax@ixazon.dynip.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <math.h>
#include "nilsimsa.h"
#include "cluster.h"

extern struct nsrecord *selkarbi,terkarbi,*rules,gunma;
extern int nilselkarbi,nrules,comparethreshold,minclustersize,exhaustiveflag;
extern int debugflag;

int findclosepair(int n)
/* Finds a close pair and returns one of them, unless there
   are no pairs, in which case it returns -129. */
{int i,nlogn,nsmax,ns,posmax,offset;
 double slope;
 if (n>1)
    if (exhaustiveflag)
       nlogn=(n*n/2);
    else
       nlogn=n*log(n);
 else
    return -129;
 slope=((double)n/2+!exhaustiveflag)/nlogn;
 offset=n?(unsigned)time(NULL)%n:0;
 for (i=0,nsmax=posmax=-129;i<nlogn;i++)
     {/*if (debugflag)
         {printf("%4d %4d ",(i+offset)%n,(int)(i+offset+1+slope*i)%n);
          if ((i&7)==7)
             printf("\n");
          }*/
      if ((ns=nilsimsa(selkarbi+((i+offset)%n),
                       selkarbi+((int)(i+offset+1+slope*i)%n)))>nsmax)
         {nsmax=ns;
          posmax=(i+offset)%n;
          }
      }
 return posmax;
 }

int smimau(const struct nsrecord *a,const struct nsrecord *b)
{return b->nilsmi-a->nilsmi;
 }

void simsasort(int n)
{int i;
 for (i=0;i<n;i++)
     selkarbi[i].nilsmi=nilsimsa(selkarbi+i,&terkarbi);
 qsort(selkarbi,n,sizeof(struct nsrecord),(__compar_fn_t)smimau);
 }

int clustersize(int n)
{int i,gappos=0,gapsize;
 for (i=gapsize=0;i<n-1;i++)
     if (selkarbi[i].nilsmi-selkarbi[i+1].nilsmi>gapsize)
        {gapsize=selkarbi[i].nilsmi-selkarbi[i+1].nilsmi;
         gappos=i+1;
         }
 return gappos;
 }

int findcluster(int n)
/* Returns the size of a cluster if there is one, else 0.
   The average of the cluster is in gunma. */
{int csize,ccenter;
 ccenter=findclosepair(n);
 csize=0;
 if (comparethreshold<-128)
    comparethreshold=24;
 if (ccenter>=0)
    {terkarbi=selkarbi[ccenter];
     simsasort(n);
     aggregate(clustersize(n));
     terkarbi=gunma;
     simsasort(n);
     csize=clustersize(n);
     aggregate(csize);
     gunma.nilsmi=(selkarbi[csize].nilsmi+selkarbi[csize-1].nilsmi+1)/2;
     if (gunma.nilsmi<comparethreshold || csize<minclustersize) /* 24 is 3*sigma, and a set with <2 elements is not a cluster */
        csize=0;
     }
 gunma.filepos=-1 /* make writerule put it at the end */;
 return csize;
 }
