/* ----------------------------------------------------------------------
   MR-MPI = MapReduce-MPI library
   http://www.cs.sandia.gov/~sjplimp/mapreduce.html
   Steve Plimpton, sjplimp@sandia.gov, Sandia National Laboratories

   Copyright (2009) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under 
   the modified Berkeley Software Distribution (BSD) License.

   See the README file in the top-level MapReduce directory.
------------------------------------------------------------------------- */

// MapReduce word frequency example in C++
// Syntax: wordfreq file1 dir1 file2 dir2 ...
// (1) read all files and files in dirs
// (2) parse into words separated by whitespace
// (3) count occurrence of each word in all files
// (4) print top 10 words

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include <iostream>
#include <istream>
#include <ostream>
#include <iterator>
#include <sstream>
#include <algorithm>

#include <sys/stat.h>
#include "mapreduce.h"
#include "keyvalue.h"

using namespace MAPREDUCE_NS;

void fileread(int, char *, KeyValue *, void *);
void unity(int, char *, int,KeyValue *, void *);
void sum(char *, int, char *, int, int *, KeyValue *, void *);
int ncompare(const void *,const void *);
void output(uint64_t, char *, int, char *, int, KeyValue *, void *);

struct Count {
  int n,limit,flag;
};

/* ---------------------------------------------------------------------- */

int main(int narg, char **args)
{
  MPI_Init(&narg,&args);

  int me,nprocs;
  MPI_Comm_rank(MPI_COMM_WORLD,&me);
  MPI_Comm_size(MPI_COMM_WORLD,&nprocs);

  if (narg <= 1) {
    if (me == 0) printf("Syntax: wordfreq file1 file2 ...\n");
    MPI_Abort(MPI_COMM_WORLD,1);
  }

  MapReduce *mr = new MapReduce(MPI_COMM_WORLD);
  mr->verbosity = 0;
  mr->timer = 1;
  //mr->memsize = 1;
  //mr->outofcore = 1;

  MPI_Barrier(MPI_COMM_WORLD);
  double tstart = MPI_Wtime();
  
  
  //int nwords = mr->map(narg-1,&args[1],0,1,0,fileread,NULL);
  int words= mr->map(1,1,(char **)&args[1],0,0,'\n',80,unity,NULL);
  printf("break point 1\n");

  int nfiles = mr->mapfilecount;
  mr->collate(NULL);
  int nunique = mr->reduce(sum,NULL);
  //extra rounds  
  int  local_result=0,global_result=1;
  while(global_result!=0)
	{
		mr->collate(NULL);
		mr->reduce(sum,(void*)&local_result);
		MPI_Allreduce(&local_result,&global_result,1,MPI_INT,MPI_SUM,MPI_COMM_WORLD);
	}
  //
  MPI_Barrier(MPI_COMM_WORLD);
  
  
  double tstop = MPI_Wtime();

  //mr->sort_values(&ncompare);
  mr->map(mr,output,&me);
  MPI_Barrier(MPI_COMM_WORLD);
  delete mr;

  if (me == 0) {
  //  printf("%d total words, %d unique words\n",nwords,nunique);
//    printf("Time to process %d files on %d procs = %g (secs)\n",
//	   nfiles,nprocs,tstop-tstart);
  }

  MPI_Finalize();
}

/* ----------------------------------------------------------------------
   read a file
   for each word in file, emit key = word, value = NULL
------------------------------------------------------------------------- */
void unity (int itask, char *ftext,int len, KeyValue *kv, void *ptr)
{
        
	char *whitespace = "\n";
 	char *word = strtok(ftext,whitespace);
 	while (word!=NULL) 
	{
   		//create string stream on the text
   		std:: stringstream strstr(word);
		// create a iterator on the string stream to iterate using tabs or white spaces
   		std:: istream_iterator<std::string> it(strstr);
                std:: istream_iterator<std::string> end;
               // dump the values into a container
                std:: vector<std::string> results(it, end);
                long key=atol((const char *)results[0].c_str());
                long value=atol((const char *)results[1].c_str());
                kv->add((char *)&key, (int)sizeof(key), (char *)&value, (int)sizeof(value));
                kv->add((char *)&value,(int)sizeof(value),(char*)&key,(int)sizeof(key));
                word = strtok(NULL,whitespace);

	}
}
void fileread(int itask, char *fname, KeyValue *kv, void *ptr)
{
  struct stat stbuf;
  int flag = stat(fname,&stbuf);
  if (flag < 0) {
    printf("ERROR: Could not query file size\n");
    MPI_Abort(MPI_COMM_WORLD,1);
  }
  int filesize = stbuf.st_size;

  FILE *fp = fopen(fname,"r");
  char *text = new char[filesize+1];
  int nchar = fread(text,1,filesize,fp);
  text[nchar] = '\0';
  fclose(fp) ;
  //Harish Testing
  //Testing
  //char *whitespace = " \t\n\f\r\0";
  char *whitespace = "\n";
  char *word = strtok(text,whitespace);
  while (word) {
   //create string stream on the text
   std:: stringstream strstr(word);
   // create a iterator on the string stream to iterate using tabs or white spaces
   std:: istream_iterator<std::string> it(strstr);
   std:: istream_iterator<std::string> end;
   // dump the values into a container
   std:: vector<std::string> results(it, end);   
   
   long key=atol((const char *)results[0].c_str());
   long value=atol((const char *)results[1].c_str()); 

    kv->add((char *)&key, (int)sizeof(key), (char *)&value, (int)sizeof(value));
    kv->add((char *)&value,(int)sizeof(value),(char*)&key,(int)sizeof(key));
    word = strtok(NULL,whitespace);
  }

  delete [] text;
}

/* ----------------------------------------------------------------------
   count word occurrence
   emit key = word, value = # of multi-values
------------------------------------------------------------------------- */

void sum(char *key, int keybytes, char *multivalue,
	 int nvalues, int *valuebytes, KeyValue *kv, void *ptr) 
{
//  kv->add(key,keybytes,(char *) &nvalues,sizeof(int));
// extract the values into an array
	long k=*(long *)key;
	long *head=(long *)multivalue;
	std::string state="none";
	//int *local_state=(int*)ptr;
	ptr=malloc(sizeof(int));
	int *local_state=(int*)ptr;
   /*
   long min=head[0];
   long max=head[0];
   */
   /*
   for(int i=0;i<nvalues;i++){
     if(min>=head[i])min=head[i];
     if(max<=head[i])max=head[i];
   }
   */
	qsort(head,nvalues,sizeof(long),ncompare); 
	int cur=0;
	int next=1;
	while(next<nvalues)
	{
		if(head[cur]!=head[next])
		{
			head[++cur]=head[next];
		}
		next++;
	}	
	nvalues=cur+1;
	long min=head[0];
	long max=head[cur];	
	/* decide the state of the vertex */
	if(k<min) 
	{
	state="LM";
	*local_state=0;
	}
	else if(k>max) 
	{
	state="OM";
	*local_state=1;
	}
	else
	{ 
	state="SM";
	*local_state=1;
	}
	/* local_max state */
	if(state.compare("LM")==0)
	{
	// do nothing
		for(int i=0;i<nvalues;i++) 
		kv->add(key,keybytes, (char *)&head[i],(int)sizeof(head[i]));
	}
 
	/* standard_merge state */
	else if(state.compare("SM")==0)
	{
		kv->add(key,keybytes, (char *)&min,(int)sizeof(min));//emit forward edge
		for(int i=1;i<nvalues;i++)
		{
		kv->add((char *)&min,(int)sizeof(min),(char *)&head[i],(int)sizeof(head[i]));
		kv->add((char *)&head[i],(int)sizeof(head[i]),(char *)&min,(int)sizeof(min));
 		}

 	}

/* optimized_max state */
	else 
	{
		for(int i=1;i<nvalues;i++)
		{
			kv->add((char *)&min,(int)sizeof(min),(char *)&head[i],(int)sizeof(head[i]));
			kv->add((char *)&head[i],(int)sizeof(head[i]),(char *)&min,(int)sizeof(min));
 		}
	}
}

/* ----------------------------------------------------------------------
   compare two counts
   order values by count, largest first
------------------------------------------------------------------------- */

int ncompare(const void *p1,const void *p2)
{
  long i1 = *(long *) p1;
  long i2 = *(long*) p2;
  if (i1 > i2) return 1;
  else if (i1 < i2) return -1;
  else return 0;
}

/* ----------------------------------------------------------------------
   process a word and its count
   depending on flag, emit KV or print it, up to limit
------------------------------------------------------------------------- */

void output(uint64_t itask, char *key, int keybytes, char *value,
	    int valuebytes, KeyValue *kv, void *ptr)
{

 // int n = *(int *) value;
  long k=*((long *)key);
  long v=*((long *)value);
  printf("%ld %ld rank:%d\n",k,v,*(int*)ptr);

 //if (count->flag) printf("%s %ld\n",value,k);
// else kv->add(key,keybytes,(char *) &n,sizeof(int));
 
  //if(count->flag)printf("%s %s\n",key, value);
  //else kv->add(key,keybytes,value,sizeof(int));
}
void output1(uint64_t itask, char *key, int keybytes, char *value,
            int valuebytes, KeyValue *kv, void *ptr)
{

 // int n = *(int *) value;
   long k=*((long *)key);
   long v=*((long *)value);
   fprintf((FILE*)ptr,"%ld %ld",k,v);
 //
 //        //if (count->flag) printf("%s %ld\n",value,k);
 //        // else kv->add(key,keybytes,(char *) &n,sizeof(int));
 //
 //          //if(count->flag)printf("%s %s\n",key, value);
 //            //else kv->add(key,keybytes,value,sizeof(int));
}
 
