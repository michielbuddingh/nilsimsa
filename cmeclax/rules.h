/***************************************************************************
                    rules.h  -  reads and writes rule file
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

void writerule(struct nsrecord *rule,char *rulefilename);
void readrules(char *rulefilename);
#define matches(a,rule) (nilsimsa((a),(rule))>(rule)->nilsmi)
int removematches(int n);
int matchany(struct nsrecord *a,struct nsrecord *rule);
