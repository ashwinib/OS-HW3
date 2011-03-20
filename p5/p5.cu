#include<stdio.h>
#include<stddef.h>
#include<search.h>
#include<device_functions.h>
#define MAX_FILE_SIZE 200
#define MAX_HASH_ENTRIES 200
#define M 10

__global__ void getWordCounts(char *fileArray,int *countArray,int *fileSize,char *wordhashtable, int *nextPtr, int *lock){
  unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
  int ind,word_started =0 ,count =0;
  int found;
  int hashvalue;
  char *ptr,*wptr,*temp;
  ptr = &fileArray[i*MAX_FILE_SIZE];
  int  tempi=0,tempi2;

  for(ind =0;ind<fileSize[i];ind++){
    if(ptr[ind]>64&&ptr[ind]<91) ptr[ind]+=32;
    if(ptr[ind]!=' '&&ptr[ind]!='.'&&ptr[ind]!='!')
      if(word_started!=1) {
	word_started = 1;
	hashvalue = ptr[ind];//>64&&ptr[ind]<91) ? ptr[ind]+32:ptr[ind];//temp addition else do only assignemnt
	wptr = &ptr[ind];
      }
      else{//Middle of the word
	hashvalue+= ptr[ind];//>64&&ptr[ind]<91) ? ptr[ind]+32:ptr[ind];
      }
    if(word_started)
      if(ptr[ind]==' '||ptr[ind]=='.'||ptr[ind]=='!'){
        word_started = 0;
	hashvalue = hashvalue % M;// 10 here is hashtable size M

	/*Check Location*/	
	//lock -hashvalue
	while(!atomicCAS(&lock[hashvalue],0,1));
	if(wordhashtable[hashvalue*20]=='\0'){//Not found in Hash
	  temp = &wordhashtable[hashvalue*20];
	  tempi =0;
	  while(&wptr[tempi]!=&ptr[ind])//Entering in hash table
		{temp[tempi]= wptr[tempi];
		tempi++;}
	//unlock -hash value
	  atomicCAS(&lock[hashvalue],1,0);
	  //fn-atomicAdd(&countArray[hashvalue],1);//count

  countArray[hashvalue] = hashvalue; 
	}
	else{//Collision detection
	  tempi =hashvalue;found = -1;
	
	 /*Check word*/
	  while(nextPtr[tempi]!=-1||found==-1){
	    tempi2 = 0;
	    found =1;
	    temp = &wordhashtable[tempi*20];
	    while(&wptr[tempi2]!=&ptr[ind]){
	      if(temp[tempi2]!=wptr[tempi2]) {found =0;break;}
	      tempi2++;
	    }
	    if(temp[tempi2]!='\0') found =0;
	    //unlock - tempi
	    atomicCAS(&lock[tempi],1,0);
	    if(found) break;
	    if(nextPtr[tempi]!=-1){
	       	tempi = nextPtr[tempi];      
		//lock - tempi
		while(!atomicCAS(&lock[tempi],0,1));
	    }
	  }

	  if(found){
	    atomicAdd(&countArray[tempi],1);
		countArray[tempi]=hashvalue;}//DEBUG
	  else{//Collision but record not found
	    tempi2 =0;
	    //lock - M+tempi2
	    while(!atomicCAS(&lock[M+tempi2],0,1));
	    while(wordhashtable[(M+tempi2)*20]!='\0' && tempi2<MAX_HASH_ENTRIES) tempi2++;//10 = M; tempi2 holds location in hast tab;e
	    if(tempi2 < MAX_HASH_ENTRIES){
	    	nextPtr[tempi] = tempi2+M;tempi=0;//tempi holds the location where last hash was found
	        temp = &wordhashtable[(M+tempi2)*20];
		while(&wptr[tempi]!=&ptr[ind]) //Entering in hash table
			{temp[tempi]= wptr[tempi]; 
			tempi++;}
		//unlock - M+tempi2
	        atomicCAS(&lock[M+tempi2],1,0);

  countArray[tempi2+M] = hashvalue; 
		//fn-atomicAdd(&countArray[tempi2+M],1);
	    }//count*/
	    //tryunlock = M+tempi2
	    atomicCAS(&lock[M+tempi2],1,0);
	  }

	}
	//atomicAdd(&countArray[hashvalue],1);
	//atomicExch(&countArray[hashvalue],hashvalue);
	count++;
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
  int *nextPtr;
  int *dnextPtr;
  int *dlock;
  int noOfFiles=0;
  FILE *fp;

  char *temp;int itemp=0;
  filename =(char*) malloc (10*sizeof(char));
  fileArray=(char*) malloc(10*MAX_FILE_SIZE*sizeof(char));
  countArray =(int*) malloc (MAX_HASH_ENTRIES*sizeof(int));//corresponding counts of words
  fileSize =(int*) malloc (10*sizeof(int));
  hashtable=(char*) malloc(20*MAX_HASH_ENTRIES*sizeof(char));
  nextPtr = (int*) malloc (MAX_HASH_ENTRIES*sizeof(int));

  cudaMalloc((void**)&dfileArray,10*MAX_FILE_SIZE*sizeof(char));
  cudaMalloc((void**)&dcountArray,MAX_HASH_ENTRIES*sizeof(int));//corresponding counts of words
  cudaMalloc((void**)&dfileSize,10*sizeof(int));
  cudaMalloc((void**)&dhashtable,20*MAX_HASH_ENTRIES*sizeof(char));//20-max word size 500-max words
  cudaMalloc((void**)&dnextPtr,MAX_HASH_ENTRIES*sizeof(int));//corresponding counts of words
  cudaMalloc((void**)&dlock,MAX_HASH_ENTRIES*sizeof(int));//corresponding counts of words

  cudaMemset(dcountArray,0,MAX_HASH_ENTRIES*sizeof(int));
  cudaMemset(dhashtable,'\0',20*MAX_HASH_ENTRIES*sizeof(char));
  cudaMemset(dnextPtr,-1,MAX_HASH_ENTRIES*sizeof(int));
  cudaMemset(dlock,0,MAX_HASH_ENTRIES*sizeof(int));
  
  while(scanf("%s",filename)!=EOF){
    printf("\nAttempting to open %s",filename);
    fp = fopen(filename,"r");
    if(fp == NULL) {
	        perror("failed to open sample.txt");
        	exit(0) ;//EXIT_FAILURE;
    }
    fread(&fileArray[noOfFiles*200],MAX_FILE_SIZE*sizeof(char),1,fp);
    fileSize[noOfFiles]=ftell(fp);
    fclose(fp);fp = NULL;
    noOfFiles++;
  }

  temp = fileArray;
  while(itemp<noOfFiles){
    printf("%s\n",temp);itemp++;
    temp+=200;
  }
  cudaMemcpy(dfileArray,fileArray,10*MAX_FILE_SIZE*sizeof(char),cudaMemcpyHostToDevice);
  cudaMemcpy(dfileSize,fileSize,10*sizeof(int),cudaMemcpyHostToDevice);
  getWordCounts<<<1,noOfFiles>>>(dfileArray,dcountArray,dfileSize,dhashtable,dnextPtr, dlock);
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
