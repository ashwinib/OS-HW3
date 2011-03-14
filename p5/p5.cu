#include<stdio.h>
#include<stddef.h>
#include<search.h>
#define MAX_FILE_SIZE 200*sizeof(char)

__global__ void getWordCounts(char *fileArray,int *countArray,int *fileSize,int *hashtable){
  unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
  int ind,word_started =0 ,count =0;
  int hashvalue;
  char *ptr,*wptr;
  ptr = &fileArray[i*200];int  tempi=0;
  for(ind =0;ind<fileSize[i];ind++){
    if(ptr[ind]!=' '&&ptr[ind]!='.'&&ptr[ind]!='!')
      if(word_started!=1) {word_started = 1;hashvalue = (ptr[ind]>64&&ptr[ind]<91) ? ptr[ind]+32:ptr[ind];}
      else{
	hashvalue+= (ptr[ind]>64&&ptr[ind]<91) ? ptr[ind]+32:ptr[ind];
      }
    if(word_started)
      if(ptr[ind]==' '||ptr[ind]=='.'||ptr[ind]=='!'){
        word_started = 0;
	hashvalue = hashvalue % 100;
	count++;
	break;//temmporary for testing
      }
  }
  countArray[i] = hashvalue; 
}

int main(int argc,char **argv){
  char *filename=NULL;//Limiting no if files
  char *fileArray;
  char *dfileArray;
  int *countArray;
  int *dcountArray;
  int *fileSize;
  int *dfileSize;
  
  int noOfFiles=0;
  FILE *fp;

  char *temp;int itemp=0;
  filename =(char*) malloc (10*sizeof(char));
  fileArray=(char*) malloc(10*MAX_FILE_SIZE);
  countArray =(int*) malloc (10*sizeof(int));
  fileSize =(int*) malloc (10*sizeof(int));
  cudaMalloc((void**)&dfileArray,10*MAX_FILE_SIZE);
  cudaMalloc((void**)&dcountArray,10*sizeof(int));
  cudaMalloc((void**)&dfileSize,10*sizeof(int));
  cudaMemset(dcountArray,0,10*sizeof(int));
  
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
  getWordCounts<<<1,noOfFiles>>>(dfileArray,dcountArray,dfileSize);
  cudaThreadSynchronize();
  cudaMemcpy(countArray,dcountArray,10*sizeof(int),cudaMemcpyDeviceToHost);

  itemp=0;
  printf("\nNo Of Words : ");
  while(itemp<noOfFiles){
    printf("\t%d",countArray[itemp]);itemp++;
  }
  cudaFree(dfileArray);
  cudaFree(dcountArray);
  free(fileArray);
  free(countArray);
}
