#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<limits.h>
#include<search.h>
#include"myatomic.h"

#define MAX_WORDS 100
#define MAX_FILES 10

pthread_mutex_t mtable = PTHREAD_MUTEX_INITIALIZER;

void pintf(char *buf){
int i=0;
while(buf[i]!='\0'){i++;}
write(1,buf,i);
}
void myprintf(char *s, int i) {
  char out[80];

  sprintf(out, s, i);
  write(1, out, strlen(out));

}

typedef struct count_node{
	char word[10];
	int count;
	struct count_node *next;
}cnode;

void* getWordCounts(char *filename){
	FILE *fp;
	char line[LINE_MAX] ;//2048
	char *copy;
	char delims[] = " .!";
	char *result = NULL,*saveptr;
	int temp,temp2;
	ENTRY item;
	ENTRY *found_item;
	cnode *cptr = NULL;

//	myprintf(filename,0);
	pintf("\nFile Read..");
	pintf(filename);

	fp = fopen(filename,"r");
	if(fp == NULL) {
	        perror("failed to open sample.txt");
        	return ;//EXIT_FAILURE;
	}
	while (fgets(line, LINE_MAX +1, fp)) {
	        //myprintf(line,1);
		pintf("\n");
		pintf(line);
	
		result = strtok_r( line, delims ,&saveptr);
		while( result != NULL ) {
			pintf("\nWord token:");
  			pintf( result );

			
			for( temp = 0; result[ temp ]; temp++)
			  result[ temp ] = toupper( result[ temp] );
			item.key = result;
			pintf("\nAcquired Lock");
			pthread_mutex_lock(&mtable);

			if((found_item = hsearch(item,FIND))==NULL){ //No entry at this hash
				cptr = (cnode*) malloc (sizeof(cnode));
				strcpy(cptr->word,result);
				cptr->count = 1;
				cptr->next = NULL;
				item.data = cptr;
				pintf("\nImhere");
				hsearch(item,ENTER);//Cannot use compare and swap here as its internal to Hash
				pintf("\nReleased Lock");
				pthread_mutex_unlock(&mtable);
			}
			else{	//Entry found 
				pintf("\nReleased Lock");
				pthread_mutex_unlock(&mtable);
				pintf("\n** ");
				pintf(result);
				pintf(" was already there.");

				//CHECK FOR COLLISION
				cnode *nptr ,*optr;
				nptr = (cnode *)found_item->data;
				optr=nptr;
				cptr = (cnode*) malloc (sizeof(cnode));
				strcpy(cptr->word,result);
				cptr->count = 1;
				cptr->next = NULL;
				pintf("\nGoing for collision check now.");
				while(strcmp(nptr->word,result)!=0 && 
					(optr=compare_and_swap_ptr(&(nptr->next),cptr,NULL)))//COLLISION CHECK
					nptr=optr;
				if(strcmp(nptr->word,result)==0 && optr!=NULL){//there was no new node added hence increment
					//USING COMPARE &SWAP to increment in nptr
					temp =2;temp2= 1;
					pintf("\nGonna simple icrement");
					while(temp2!=compare_and_swap(&(((cnode*)found_item->data)->count),temp,temp2)){//find the count and increment;
					  myprintf("\nItwas %d",temp2);
					  temp++;temp2++;
					}
					myprintf("\nIncremented to : %d",((cnode*)found_item->data)->count);
				}
				else
					pintf("\n**U are not supposed to be here**");
			}

			result = strtok_r( NULL, delims , &saveptr);
		}
    	}
	fclose(fp);
	free(filename);
	pintf("\nClosing File");
	return ;// EXIT_SUCCESS;
}

int main(int argc, char **argv){
	char *filename;
	int pi =0,i;
	pthread_t p[MAX_FILES];
	filename = (char*) malloc (LINE_MAX * sizeof(char));
	hcreate(MAX_WORDS);
	ENTRY item,*found_item;
	
	while(scanf("%s",filename)!=EOF){
		pintf("\nAbout to read ");
		pintf(filename);

		pthread_create(&p[pi],NULL,&getWordCounts,(void*)filename);
		filename = (char*) malloc (LINE_MAX * sizeof(char));
		pi++;

		//printf("\nFound %s",filename);
	}
	for(i =0 ;i<pi;i++)
	pthread_join(p[i],NULL);
	
	//Display Hash
	//TODO: In increasing order
	item.key = "This";
	found_item = hsearch(item,FIND);
	myprintf("\n\nCount of THis is : %d",((cnode*)found_item->data)->count);
}

