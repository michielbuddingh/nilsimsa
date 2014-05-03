/***************************************************************************
                    rules.c  -  reads and writes rule file
                             -------------------
    begin                : Wed Jul 4 2001
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

#include "nilsimsa.h"
#include "rules.h"

extern struct nsrecord *selkarbi,terkarbi,*rules,gunma;
extern int nilselkarbi,nrules;

void readrules(char *rulefilename)
/* Format for a rule:
   0123456789abcdef0369cf258be147ad05af49e38d27c16b07e5c3a18f6d4b29  101 A 0 comment
   0123... is the nilsimsa code at the center of the cluster.
   101 is the nilsimsa threshold of the cluster (radius is 27).
   The extra space is for a minus sign.
   A is A for allow or D for deny.
   0 is the priority. There are two priorities. All priority 0 rules are checked, then
   all priority 1 rules. Rules added by nilsimsa are always priority 0.
   Priority 1 rules are used for default actions. */
{FILE *rulefile;
 char line[256],*fgotten,*thresh,*allow;
 long filepos;
 rulefile=fopen(rulefilename,"r");
 for (nrules=0,fgotten=line;rulefile && fgotten;nrules++)
     {filepos=ftell(rulefile);
      fgotten=fgets(line,256,rulefile);
      if (fgotten)
         {rules=realloc(rules,(nrules+1)*sizeof(struct nsrecord));
          rules[nrules].filepos=filepos;
          rules[nrules].flag=INVALID;
          thresh=strchr(line,' ');
          if (thresh)
             {*thresh++=0;
              while (isspace(*thresh))
                 thresh++;
              allow=strchr(thresh,' ');
              if (allow)
                 allow++;
              }
          }
      if (fgotten && thresh && allow)
         {if (*allow=='D')
             rules[nrules].flag=DENYRULE;
          if (*allow=='A')
             rules[nrules].flag=ALLOWRULE;
          rules[nrules].flag*=strtocode(line,rules+nrules);
          rules[nrules].nilsmi=atoi(thresh);
          rules[nrules].priority=atoi(allow+2);
          }
      }
 if (rulefile)
    {fclose(rulefile);
     nrules--;
     }
 }

void writerule(struct nsrecord *rule,char *rulefilename)
/* Write the rule to the file. If rule->filepos<0, it is written
   at the end of the file, and followed with a line feed; otherwise
   it is written at filepos and not followed with a linefeed. */
{FILE *rulefile;
 char str[65];
 rulefile=NULL;
 if (rulefilename)
    rulefile=fopen(rulefilename,"a+");
 if (rulefile)
    if (rule->filepos<0)
       fseek(rulefile,0,SEEK_END);
    else
       fseek(rulefile,rule->filepos,SEEK_SET);
 codetostr(rule,str);
 fprintf(rulefile?rulefile:stdout,"%s %4d %c %d ",str,rule->nilsmi,"ILFAD"[rule->flag],rule->priority);
 if (rule->filepos<0)
    fprintf(rulefile?rulefile:stdout,"\n");
 if (rulefile)
    fclose(rulefile);
 }

/* matches() is a macro in rules.h */

int matchany(struct nsrecord *a,struct nsrecord *rule)
/* Checks a against all rules, first the priority 0 rules,
   then the priority 1 rules. Returns ALLOWRULE or DENYRULE
   according to the type of the first matched rule.
   Copies the matched rule into rule, unless it's NULL. */
{int i,j,result;
 result=0;
 for (i=0;i<2;i++)
     for (j=0;j<nrules && result==0;j++)
         if ((rules[j].priority==i) && (matches(a,rules+j)))
            {result=rules[j].flag;
             if (rule)
                *rule=rules[j];
             }
 return result;
 }

int removematches(int n)
/* Puts all codes in selkarbi that match rules or are invalid at the end,
   and returns the number of codes that don't. */
{int i;
 struct nsrecord temp;
 for (i=0;i<n;i++)
     if (selkarbi[i].flag == INVALID || matchany(selkarbi+i,NULL))
        {temp=selkarbi[i];
         selkarbi[i--]=selkarbi[--n];
         selkarbi[n]=temp;
         }
 return n;
 }
