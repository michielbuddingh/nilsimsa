/***************************************************************************
                             main.c  -  nilsimsa
                             -------------------
    begin                : Fri Mar 16 01:41:08 EST 2001
    copyright            : (C) 2001-2002 by cmeclax
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

/* Computes a nilsimsa code for a file fed to stdin. Files with
   similar content often have the same nilsimsa code.
   Options eventually to be implemented:
   --long-code		Instead of a 256-bit vector, write the entire histogram as a code.
			This will allow more accurate detection of certain kinds of viruses and worms.
   --code-list		Read a list of codes from a file.
   Anywhere a code is called for, a file may be given; if a code is
   intended and such a file exists, prepend a few zeros to the code.
   */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "nilsimsa.h"
#include "cluster.h"
#include "rules.h"

unsigned int acc[256],threshold;
unsigned char tran[256],popcount[256];
struct nsrecord *selkarbi,terkarbi,*rules,gunma;
int nilselkarbi,nrules;
char *comparestr,*rulefile,*checkrulefile;
int comparethreshold,minclustersize;
int helpflag,versionflag,aggregateflag,noheaderflag,verboseflag,catflag,mboxflag;
int exhaustiveflag,debugflag,recursiveflag;
int popterror;
int exitcode;

void clear(struct nsrecord *a)
{a->total=a->threshold=0;
 memset(a->acc,0,sizeof(a->acc));
 memset(a->code,0,sizeof(a->code));
 }

void filltran()
{int i,j,k;
 for (i=j=0;i<256;i++)
     {j=(j*53+1)&255;
      j+=j;
      if (j>255)
         j-=255;
      for (k=0;k<i;k++)
          if (j==tran[k])
             {j=(j+1)&255;
              k=0;
              }
      tran[i]=j;
      }
 }

void dumptran()
{int i;
 for (i=0;i<256;i++)
     {printf("%02x ",tran[i]);
      if ((i&15)==15)
         putchar('\n');
      }
 }

void fillpopcount()
{int i,j;
 memset(popcount,0,256);
 for (i=0;i<256;i++)
     for (j=0;j<8;j++)
         popcount[i]+=1&(i>>j);
 }

int defromulate(FILE *file)
#define CANY 258
/* apply this line to the character in any, instead of reading one */
#define ANY 257
/* read a character into any, or emit any */
#define NONE 256
/* don't read a character, or don't emit one */
/* Reads a character from a mailbox file according to these rules:
   Ignore all lines up to and including the first one that begins
   with "From ".
   Return -2 on encountering "\nFrom ". Ignore that and the rest
   of the line.
   Return -3 if the file does not contain a "From " line (usually
   this means that the mailbox is empty).
   On seeing "\n>From ", return "\nFrom ". Generally, if "\n>"
   is followed by any number of greater-than signs and "From ",
   drop the last greater-than sign.

   State table:

   00 \n 0 01 F 0 03 r 0 06 o 0 10 m 0 15 bl 0 21 \n -2 01
    *       *      *      *      *      *       *
    *      \n     \n     \n     \n     \n       0
   00      02     04     07     11     16      21
                   0      0      0      0
                   F      F      F      F
                  05     08     12     17
                          0      0      0
                          r      r      r
                         09     13     18
                                 0      0
                                 o      o
    02,05,09,14,20 0 * 00       14     19
                                        0
                                        m
                                       20

   01 > \n 22 F 0 23 r 0 24 o 0 25 m 0 26 bl F 27 0 r 28 0 o 29 0 m 30 0 bl 0
            *      *      *      *      *
            >      >      >      >      >
           02     04     07     11     16

   22 > > 22

   * EOF EOF 31 F 0 32 r 0 33 o 0 34 m 0 35 bl 0 36 \n 0 01
              *      *      *      *      *       *
              *      *      *      *      *       *
             31     31     31     31     31      36
   */
{static const short statetable[][5][3]=
 {{{'\n',NONE, 1},{EOF ,EOF ,31},{ANY ,ANY , 0},{0   ,0   , 0},{0   ,0   , 0}}, /* 0*/
  {{'F' ,NONE, 3},{EOF ,EOF ,31},{'>' ,'\n',22},{'\n','\n',01},{ANY ,'\n',02}},
  {{NONE,ANY ,38},{NONE,ANY , 0},{NONE,ANY , 0},{NONE,ANY , 0},{NONE,ANY , 0}},
  {{'r' ,NONE, 6},{EOF ,EOF ,31},{ANY ,'\n', 4},{0   ,0   , 0},{0   ,0   , 0}},
  {{NONE,'F' , 5},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{NONE,ANY ,38},{NONE,ANY , 0},{NONE,ANY , 0},{NONE,ANY , 0},{NONE,ANY , 0}}, /* 5*/
  {{'o' ,NONE,10},{EOF ,EOF ,31},{ANY ,'\n', 7},{0   ,0   , 0},{0   ,0   , 0}},
  {{NONE,'F' , 8},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{NONE,'r' , 9},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{NONE,ANY ,38},{NONE,ANY , 0},{NONE,ANY , 0},{NONE,ANY , 0},{NONE,ANY , 0}},
  {{'m' ,NONE,15},{EOF ,EOF ,31},{ANY ,'\n',11},{0   ,0   , 0},{0   ,0   , 0}}, /*10*/
  {{NONE,'F' ,12},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{NONE,'r' ,13},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{NONE,'o' ,14},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{NONE,ANY ,38},{NONE,ANY , 0},{NONE,ANY , 0},{NONE,ANY , 0},{NONE,ANY , 0}},
  {{' ' ,NONE,21},{EOF ,EOF ,31},{ANY ,'\n',16},{0   ,0   , 0},{0   ,0   , 0}}, /*15*/
  {{NONE,'F' ,17},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{NONE,'r' ,18},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{NONE,'o' ,19},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{NONE,'m' ,20},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{NONE,ANY ,38},{NONE,ANY , 0},{NONE,ANY , 0},{NONE,ANY , 0},{NONE,ANY , 0}}, /*20*/
  {{'\n',-2  ,37},{EOF ,EOF ,31},{ANY ,NONE,21},{0   ,0   , 0},{0   ,0   , 0}},
  {{'>' ,'>' ,22},{'F' ,NONE,23},{ANY,'>'  , 2},{0   ,0   , 0},{0   ,0   , 0}},
  {{'r' ,NONE,24},{ANY,'>'  , 4},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{'o' ,NONE,25},{ANY,'>'  , 7},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{'m' ,NONE,26},{ANY,'>'  ,11},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}}, /*25*/
  {{' ' ,'F' ,27},{ANY,'>'  ,16},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{NONE,'r' ,28},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{NONE,'o' ,29},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{NONE,'m' ,30},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{NONE,' ' , 0},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0},{0   ,0   , 0}}, /*30*/
  {{'F' ,NONE,32},{EOF ,-3  ,31},{ANY ,NONE,31},{0   ,0   , 0},{0   ,0   , 0}},
  {{'r' ,NONE,33},{EOF ,-3  ,31},{ANY ,NONE,31},{0   ,0   , 0},{0   ,0   , 0}},
  {{'o' ,NONE,34},{EOF ,-3  ,31},{ANY ,NONE,31},{0   ,0   , 0},{0   ,0   , 0}},
  {{'m' ,NONE,35},{EOF ,-3  ,31},{ANY ,NONE,31},{0   ,0   , 0},{0   ,0   , 0}},
  {{' ' ,NONE,36},{EOF ,-3  ,31},{ANY ,NONE,31},{0   ,0   , 0},{0   ,0   , 0}}, /*35*/
  {{'\n',NONE,37},{EOF ,-3  ,31},{ANY ,NONE,36},{0   ,0   , 0},{0   ,0   , 0}},
  {{'F' ,NONE,40},{EOF ,EOF ,31},{'>' ,NONE,22},{ANY ,NONE, 2},{ANY ,NONE, 2}},
  {{CANY,NONE, 0},{'\n',NONE,39},{ANY ,NONE, 0},{0   ,0   , 0},{0   ,0   , 0}},
  {{'F' ,NONE, 3},{EOF ,EOF ,31},{'>' ,NONE,22},{'\n',NONE,01},{ANY ,NONE,02}},
  {{'r' ,NONE,41},{EOF ,EOF ,31},{ANY ,NONE, 4},{0   ,0   , 0},{0   ,0   , 0}}, /*40*/
  {{'o' ,NONE,42},{EOF ,EOF ,31},{ANY ,NONE, 7},{0   ,0   , 0},{0   ,0   , 0}},
  {{'m' ,NONE,43},{EOF ,EOF ,31},{ANY ,NONE,11},{0   ,0   , 0},{0   ,0   , 0}},
  {{' ' ,NONE,21},{EOF ,EOF ,31},{ANY ,NONE,16},{0   ,0   , 0},{0   ,0   , 0}}};
 static int any,state=31,ch,i;
 do {for (i=0,ch=NONE;;i++)
         {if (statetable[state][i][0]==NONE)
             break;
          if (statetable[state][i][0]==CANY)
             {ch=any;
              continue;
              }
          if (i==0)
             ch=getc(file);
          if (statetable[state][i][0]==ANY)
             any=ch;
          if (statetable[state][i][0]==ANY || statetable[state][i][0]==ch)
             break;
          }
     /*if (ch==NONE)
        printf("   ");
     else
        printf("%3x",ch&0xfff);*/
     ch=statetable[state][i][1];
     if (ch==ANY)
        ch=any;
     state=statetable[state][i][2];
     /*printf("%3d%3x %c\n",state,ch&0xfff,isprint(ch)?ch:' ');*/
     }
 while (ch==NONE);
 return ch;
 }

int accfile(FILE *file,struct nsrecord *a,int mboxflag)
/* Returns -1 on reaching end of file, -2 on reaching end of message,
   -3 if the file contains no messages. */
{unsigned int chcount;
 int ch,lastch[4],hflag;
 lastch[0]=lastch[1]=lastch[2]=lastch[3]=-1;
 chcount=0;
 hflag=noheaderflag;
 do {ch=mboxflag?defromulate(file):getc(file);
     if (ch>=0)
        if (hflag)
           if ((lastch[0]=='\n' && lastch[1]=='\n')
             ||(lastch[0]=='\r' && lastch[1]=='\r')
             ||(lastch[0]=='\n' && lastch[1]=='\r' && lastch[2]=='\n' && lastch[3]=='\r'))
              {hflag=0;
               lastch[0]=lastch[1]=lastch[2]=lastch[3]=-1;
               }
           else
              ;
        else
           ;
        if (!hflag && ch>=0)
           {chcount++;
            if (catflag)
               putchar(ch);
            if (lastch[1]>=0)
               a->acc[tran3(ch,lastch[0],lastch[1],0)]++;
            if (lastch[2]>=0)
               {a->acc[tran3(ch,lastch[0],lastch[2],1)]++;
                a->acc[tran3(ch,lastch[1],lastch[2],2)]++;
                }
            if (lastch[3]>=0)
               {a->acc[tran3(ch,lastch[0],lastch[3],3)]++;
                a->acc[tran3(ch,lastch[1],lastch[3],4)]++;
                a->acc[tran3(ch,lastch[2],lastch[3],5)]++;
                a->acc[tran3(lastch[3],lastch[0],ch,6)]++;
                a->acc[tran3(lastch[3],lastch[2],ch,7)]++;
                }
            }
     lastch[3]=lastch[2];
     lastch[2]=lastch[1];
     lastch[1]=lastch[0];
     lastch[0]=ch;
     }
 while (ch>=0);
 switch (chcount)
   {case 0: ;
    case 1: ;
    case 2: break;
    case 3: a->total++;
            break;
    case 4: a->total+=4;
            break;
    default:a->total+=(8*chcount)-28;
    }
 a->threshold=(a->total)/256; /* round down because criterion is >threshold */
 return ch;
 }

void makecode(struct nsrecord *a)
{int i;
 memset(a->code,0,32);
 for (i=0;i<256;i++)
     {a->code[i>>3]+=((a->acc[i]>a->threshold)<<(i&7));
      }
 }

int nilsimsa(struct nsrecord *a,struct nsrecord *b)
{int i,bits;
 for (i=bits=0;i<32;i++)
     bits+=popcount[255&(a->code[i]^b->code[i])];
 return 128-bits;
 }

void codetostr(struct nsrecord *a,char *str)
{int i;
 for (i=0;i<32;i++)
     sprintf(str+i+i,"%02x",0xff&(a->code[31-i]));
 }

int strtocode(const char *str,struct nsrecord *a)
/* Returns 0 if error, or 1 if valid code. */
{unsigned int i,len,valid,byte;
 len=strlen(str);
 valid=(len>63) && (isxdigit(*str));
 a->total=0;
 if (len&1)
    str++;
 for (;*str;str+=2)
     {memmove(a->code+1,a->code,31);
      if (!isxdigit(str[0]) || !isxdigit(str[1]))
         valid=0;
      sscanf(str,"%2x",&byte);
      a->code[0]=byte;
      memmove(a->acc+8,a->acc,248*sizeof(int));
      for (i=0;i<8;i++)
          a->acc[i]=1&(byte>>i);
      }
 if (!valid)
    clear(a);
 for (i=0;i<256;i++)
     a->total+=a->acc[i];
 a->threshold=0;
 return valid;
 }

int codeorfile(struct nsrecord *a,char *str,int mboxflag)
/* Attempts to read the file named str. If that fails,
   attempts to convert str to a code. Returns 2 if directory, 1 if success,
   0 if failure, -1 if mboxflag is set and there are more messages,
   -2 if mbox file is empty (failure again, but different error message). */
{static FILE *file;
 struct stat statbuf;
 static int msgnum=0;
 int ret;
 if (!strcmp(str,"-"))
    {ret=accfile(stdin,a,mboxflag);
     file=stdin;
     a->name="";
     if (mboxflag)
        {a->name=malloc(24);
         sprintf(a->name,"#%u",msgnum);
         a->name=realloc(a->name,strlen(a->name)+1);
         }
     a->flag=FILECODE;
     msgnum++;
     if (ret!=-2)
        msgnum=0;
     }
 else
    {ret=stat(str,&statbuf);
     if (ret==0 && (statbuf.st_mode&S_IFMT)==S_IFDIR)
        return 2;
     if (msgnum==0 || !mboxflag)
        file=fopen(str,"rb");
     a->name=str;
     if (file)
        {ret=accfile(file,a,mboxflag);
         a->flag=FILECODE;
         if (mboxflag)
            {a->name=malloc(strlen(str)+24);
             sprintf(a->name,"%s#%u",str,msgnum);
             a->name=realloc(a->name,strlen(a->name)+1);
             }
         else
            a->name=strdup(str);
         msgnum++;
         if (ret!=-2)
            {fclose(file);
             msgnum=0;
             }
         }
     else
        {ret=strtocode(str,a);
         if (ret)
            a->flag=LITERALCODE;
         return ret;
         }
     }
 makecode(a);
 if (ret==-3)
    a->flag=INVALID;
 ret+=!++ret;
 return ret;
 }

int fileordirectory(char *str,int sofar)
/* Process one file or directory. Recursive if directory and -r. */
{int ret;
 DIR * dir;
 struct dirent *ent;
 char *file;
 file=NULL;
 ret=-1;
 while (ret==-1)
    {selkarbi=realloc(selkarbi,sizeof(terkarbi)*(sofar+1));
     clear(selkarbi+sofar);
     ret=codeorfile(selkarbi+sofar,str,mboxflag);
     if (debugflag)
        printf("%2d %s\n",ret,str);
     switch (ret)
        {case 0:   /* no such file, and not a valid code */
            fprintf(stderr,"nilsimsa: %s: invalid code or file name\n",str);
            break;
         case 1:   /* valid code or regular file */
            sofar++;
            break;
         case -2:  /* empty mbox file */
            fprintf(stderr,"nilsimsa: %s: empty mbox file\n",str);
            break;
         case -1:  /* message in mbox file */
            ++sofar;
            break;
         case 2:   /* directory */
            if (recursiveflag)
               {dir=opendir(str);
                if (dir)
		    for (ent=readdir(dir);ent;ent=readdir(dir))
		       {
			   file=realloc(file,strlen(str)+strlen(ent->d_name)+2);
			   strcpy(file,str);
			   strcat(file,"/");
			   strcat(file,ent->d_name);
			   if (strcmp(ent->d_name,".") && strcmp(ent->d_name,".."))
                               sofar=fileordirectory(file,sofar);
			}
                }
            else
               fprintf(stderr,"nilsimsa: %s: is a directory\n",str);
            break;
         }
     }
 free(file);
 return sofar;
 }

void aggregate(int n)
/* Aggregate the first n codes in selkarbi into gunma. */
{int i,j;
 clear(&gunma);
 for (i=0;i<n;i++)
     {gunma.total+=selkarbi[i].total;
      for (j=0;j<256;j++)
          gunma.acc[j]+=selkarbi[i].acc[j];
      }
 gunma.threshold=gunma.total/256;
 makecode(&gunma);
 }

void dump1code(struct nsrecord *code)
{char str[65];
 codetostr(code,str);
 printf("%s %4d %c %d \n",str,code->nilsmi,"ILFAD"[code->flag],code->nilsmi);
 }

void dumpcodes(struct nsrecord *code,int n)
{int i;
 for (i=0;i<n;i++)
     dump1code(code+i);
 }

void version()
{printf("nilsimsa %s \n� 2001-2002 cmeclax\nDistributed under GNU GPL\n","0.2.4");
 }

void help()
{puts("\
nilsimsa [options] [files]\n\
--help\n\
		Prints this message.\n\
--version\n\
		Prints the version number.\n\
file1 file2 ...\n\
		Compute the nilsimsa codes of the files.\n\
-r					--recursive\n\
		Process all files inside listed directories.\n\
-c code file1 file2			--compare-to\n\
		Compare files to code.\n\
-c code -t 48 file			--threshold\n\
		Return true if file is more than 48 bits similar to code.\n\
-s					--skip-headers\n\
		file is a mail message; ignore the headers.\n\
--mbox\n\
		files are mailbox folders; take the code of each message.\n\
-a file1 file2 ...			--aggregate\n\
		Compute the aggregate nilsimsa code of the files.\n\
-C clusterlist file1 file2 ...		--find-clusters\n\
		Find a cluster of similar files.\n\
-H clusterlist file1 file2 ...		--check-clusters\n\
		Compare files to all priority 0 clusters in clusterlist,\n\
		then all priority 1 clusters. If the first match is an\n\
		accept cluster, return 0. If it is a deny cluster, return\n\
		1. If there is no match, return 0.\n\
-m n					--min-cluster-size\n\
		Set the minimum cluster size (default 3).\n\
-v					--verbose\n\
		In -C, lists all the codes, the ones which are in the\n\
		cluster first; in -H, lists which cluster matches.\n\
-x					--exhaustive\n\
		In -C, compare all pairs of codes, not just n*log(n) pairs.\n\
\n\
Any time a file is called for, a code may be given, and vice versa,\n\
except after -C and -H.");
 }

static struct poptOption options[]=
{{"compare-to",		'c',POPT_ARG_STRING,&comparestr,0},
 {"threshold",		't',POPT_ARG_INT,   &comparethreshold,0},
 {"min-cluster-size",	'm',POPT_ARG_INT,   &minclustersize,0},
 {"help",		'h',POPT_ARG_NONE,  &helpflag,0},
 {"version",		0,  POPT_ARG_NONE,  &versionflag,0},
 {"verbose",		'v',POPT_ARG_NONE,  &verboseflag,0},
 {"cat",		0,  POPT_ARG_NONE,  &catflag,0},
 {"debug",		0,  POPT_ARG_NONE,  &debugflag,0},
 {"mbox",		0,  POPT_ARG_NONE,  &mboxflag,0},
 {"aggregate",		'a',POPT_ARG_NONE,  &aggregateflag,0},
 {"skip-headers",	's',POPT_ARG_NONE,  &noheaderflag,0},
 {"find-clusters",	'C',POPT_ARG_STRING,&rulefile,0},
 {"check-clusters",	'H',POPT_ARG_STRING,&checkrulefile,0},
 {"exhaustive",		'x',POPT_ARG_NONE,  &exhaustiveflag,0},
 {"recursive",		'r',POPT_ARG_NONE,  &recursiveflag,0},
 {NULL,			0,  POPT_ARG_NONE,  NULL,0}};


/* int omain(int argc, char *argv[]) */
/* {int i,j,ret,matched; */
/*  char str[65]; */
/*  const char **poptargv; */
/*  int poptargc; */
/*  poptContext context; */
/*  filltran(); */
/*  /\*dumptran();*\/ */
/*  comparestr=exitcode=helpflag=versionflag=verboseflag= */
/*  noheaderflag=catflag=mboxflag=NULL; */
/*  comparethreshold=-999; */
/*  minclustersize=3; */
/*  context=poptGetContext("nilsimsa",argc,argv,options,0); */
/*  popterror=poptGetNextOpt(context); */
/*  poptargv=poptGetArgs(context); */
/*  for (poptargc=0;poptargv && poptargv[poptargc];poptargc++); */
/*  //if (popterror<-1) */
/*  //   die(poptStrerror(popterror),3); */
/*  fillpopcount(); */
/*  if (versionflag) */
/*     version(); */
/*  if (helpflag) */
/*     help(); */
/*  if (comparestr) */
/*     codeorfile(&terkarbi,comparestr,0); */
/*  if (aggregateflag && rulefile) */
/*     {fprintf(stderr,"nilsimsa: aggregate and find-clusters are incompatible"); */
/* 	aggregateflag=0; */
/* 	rulefile=NULL; */
/*      } */
/*  nilselkarbi=poptargc; */
/*  if (!nilselkarbi) /\* if there are no arguments, there is one file, stdin *\/ */
/*     {nilselkarbi++; */
/*      poptargv=realloc(poptargv,sizeof(char *)); */
/*      poptargv[0]="-"; */
/*      } */
/*  selkarbi=malloc(sizeof(terkarbi)*nilselkarbi); */
/*  if (helpflag || versionflag) */
/*     nilselkarbi=0; */
/*  for (i=j=0;i<nilselkarbi;i++) { */
/*       /\* j is the number of messages hashed; i is the number of arguments processed *\/ */
/*       j=fileordirectory(poptargv[i],j); */

/*  } */
/*  nilselkarbi=j; */
/*  aggregate(nilselkarbi); */
/*  if (rulefile) */
/*     {readrules(rulefile); */
/*      nilselkarbi=removematches(nilselkarbi); */
/*      if (verboseflag) */
/*         printf("%d left\n",nilselkarbi); */
/*      i=findcluster(nilselkarbi); */
/*      if (i) */
/*         {gunma.flag=DENYRULE; */
/*          writerule(&gunma,rulefile); */
/*          writerule(&gunma,NULL); /\* writes to stdout *\/ */
/*          if (verboseflag) */
/*             {printf("%d %d %d\n",poptargc,nilselkarbi,i); */
/*              dumpcodes(selkarbi,poptargc); */
/*              } */
/*          } */
/*      } */
/*  if (checkrulefile) */
/*     {readrules(checkrulefile); */
/*      if (aggregateflag) */
/*         {exitcode=(matched=matchany(&gunma,&terkarbi))==DENYRULE; */
/*          codetostr(&gunma,str); */
/*          printf("%d %s",exitcode,str); */
/*          if (matched&&verboseflag) */
/*             {putchar(' '); */
/*              writerule(&terkarbi,NULL); */
/*              } */
/*          printf("\n"); */
/*          } */
/*      else */
/*         for (i=0;i<nilselkarbi;i++) */
/*             if (selkarbi[i].flag) */
/*                {exitcode=(matched=matchany(selkarbi+i,&terkarbi))==DENYRULE; */
/*                 codetostr(selkarbi+i,str); */
/*                 printf("%d %s",exitcode,selkarbi[i].name); */
/*                 if (matched&&verboseflag) */
/*                    {putchar(' '); */
/*                     writerule(&terkarbi,NULL); */
/*                     } */
/*                 printf("\n"); */
/*                 } */
/*      } */
/*  if (aggregateflag && !checkrulefile) */
/*      if (comparestr) */
/*         if (comparethreshold>=-256) */
/*            printf("%d\n",exitcode=nilsimsa(&gunma,&terkarbi)>comparethreshold); */
/*         else */
/*            printf("%4d\n",nilsimsa(&gunma,&terkarbi)); */
/*      else */
/*         {codetostr(&gunma,str); */
/*          printf("%s\n",str); */
/*          } */
/*  else */
/*     if (!rulefile && !checkrulefile) */
/*        for (i=0;i<nilselkarbi;i++) */
/*            {if (selkarbi[i].flag) */
/*                {if (comparestr) */
/*                    if (comparethreshold>=-256) */
/*                       printf("%d %s\n",exitcode=nilsimsa(selkarbi+i,&terkarbi)>comparethreshold,selkarbi[i].name); */
/*                    else */
/*                       printf("%4d %s\n",nilsimsa(selkarbi+i,&terkarbi),selkarbi[i].name); */
/*                 else */
/*                    {codetostr(selkarbi+i,str); */
/*                     printf("%s %s\n",str,selkarbi[i].name); */
/*                     } */
/*                 } */
/*             } */
/*  poptFreeContext(context); */
/*  return exitcode; */
/*  } */
