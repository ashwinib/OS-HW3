#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<limits.h>
#include<search.h>

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

			item.key = result;
			pthread_mutex_lock(&mtable);

			if((found_item = hsearch(item,FIND))==NULL){ //No entry at this hash
				cptr = (cnode*) malloc (sizeof(cnode));
				strcpy(cptr->word,result);
				cptr->count = 1;
				cptr->next = NULL;
				item.data = cptr;
				pintf("\nImhere");
				hsearch(item,ENTER);
				pthread_mutex_unlock(&mtable);
			}
			else{	//Entry found 
				pthread_mutex_unlock(&mtable);
				pintf("\n** ");
				pintf(result);
				pintf(" was already there.");
				//CHECK FOR COLLISION
				
				cnode *nptr = ((struct cnode *)found_item->data)->next;
				while(nptr->next!=NULL || strcmp(nptr,result)!=0)//COLLISION CHECK
					nptr++;
				if(nptr->next == NULL && strcmp(nptr->data,result)!=0){
					cptr = (cnode*) malloc (sizeof(cnode));
					strcpy(cptr->word,result);
					cptr->count = 1;
					cptr->next = NULL;
					
					((struct cnode *)found_item->data)->next = cptr;
				}
				else{//USE COMPARE AND SWAP
				}
					
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
}

