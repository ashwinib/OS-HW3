#include<stdio.h>
#include<stddef.h>
#include<search.h>
#include<device_functions.h>
#define MAX_FILE_SIZE 200*sizeof(char)

__global__ void getWordCounts(char *fileArray,int *countArray,int *fileSize,char *wordhashtable){
  unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
  int ind,word_started =0 ,count =0;
  int hashvalue;
  char *ptr,*wptr,*temp;
  ptr = &fileArray[i*200];int  tempi=0;
  for(ind =0;ind<fileSize[i];ind++){
    if(ptr[ind]!=' '&&ptr[ind]!='.'&&ptr[ind]!='!')
      if(word_started!=1) {
	word_started = 1;
	hashvalue = (ptr[ind]>64&&ptr[ind]<91) ? ptr[ind]+32:ptr[ind];//temp addition else do only assignemnt
	wptr = &ptr[ind];
      }
      else{//Middle of the word
	hashvalue+= (ptr[ind]>64&&ptr[ind]<91) ? ptr[ind]+32:ptr[ind];
      }
    if(word_started)
      if(ptr[ind]==' '||ptr[ind]=='.'||ptr[ind]=='!'){
        word_started = 0;
	hashvalue = hashvalue % 100;
	if(wordhashtable[hashvalue*20]=='\0'){
	temp = &wordhashtable[hashvalue*20];tempi =0;
	while(&wptr[tempi]!=&ptr[ind]){temp[tempi]=wptr[tempi];tempi++;}///TODO: MAKE ATOMIC!!!
	}
	atomicAdd(&countArray[hashvalue],1);
	//atomicExch(&countArray[hashvalue],hashvalue);
	count++;
	//break;//temmporary for testing
      }
  }
  //countArray[i] = hashvalue; 
}

int main(int argc,char **argv){
  char *filename=NULL;//Limiting no if files
  char *fileArray;
  char *dfileArray;
  int *countArray;
  int *dcountArray;
  int *fileSize;
  int *dfileSize;
  char *hashtable; 
  char *dhashtable; 
  int noOfFiles=0;
  FILE *fp;

  char *temp;int itemp=0;
  filename =(char*) malloc (10*sizeof(char));
  fileArray=(char*) malloc(10*MAX_FILE_SIZE);
  countArray =(int*) malloc (200*sizeof(int));//corresponding counts of words
  fileSize =(int*) malloc (10*sizeof(int));
  hashtable=(char*) malloc(20*200*sizeof(char));
  cudaMalloc((void**)&dfileArray,10*MAX_FILE_SIZE);
  cudaMalloc((void**)&dcountArray,200*sizeof(int));//corresponding counts of words
  cudaMalloc((void**)&dfileSize,10*sizeof(int));
  cudaMalloc((void**)&dhashtable,20*200*sizeof(char));//20-max word size 500-max words
  cudaMemset(dcountArray,0,200*sizeof(int));
  cudaMemset(dhashtable,'\0',20*200*sizeof(char));
  
  while(scanf("%s",filename)!=EOF){
    printf("\nAttempting to open %s",filename);
    fp = fopen(filename,"r");
    if(fp == NULL) {
	        perror("failed to open sample.txt");
        	exit(0) ;//EXIT_FAILURE;
    }
    fread(&fileArray[noOfFiles*200],MAX_FILE_SIZE,1,fp);
    fileSize[noOfFiles]=ftell(fp);
    fclose(fp);fp = NULL;
    noOfFiles++;
  }

  temp = fileArray;
  while(itemp<noOfFiles){
    printf("%s\n",temp);itemp++;
    temp+=200;
  }
  cudaMemcpy(dfileArray,fileArray,10*MAX_FILE_SIZE,cudaMemcpyHostToDevice);
  cudaMemcpy(dfileSize,fileSize,10*sizeof(int),cudaMemcpyHostToDevice);
  getWordCounts<<<1,noOfFiles>>>(dfileArray,dcountArray,dfileSize,dhashtable);
  cudaThreadSynchronize();
  cudaMemcpy(countArray,dcountArray,200*sizeof(int),cudaMemcpyDeviceToHost);
  cudaMemcpy(hashtable,dhashtable,20*200*sizeof(char),cudaMemcpyDeviceToHost);

  itemp=0;
  printf("\nNo Of Words : \n");
  while(itemp<200){
//    printf("\t%d",countArray[itemp]);itemp++;
    if(hashtable[itemp*20]!='\0')
      printf("%s:[%d]\n",&hashtable[itemp*20],countArray[itemp]);
    itemp++;
  }
  cudaFree(dfileArray);
  cudaFree(dcountArray);
  cudaFree(dhashtable);
  free(fileArray);
  free(countArray);
  free(hashtable);
}
