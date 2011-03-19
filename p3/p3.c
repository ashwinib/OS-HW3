#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include<string.h>
#include "board.h"

#define MAX_THREAD_NUM 200

#ifndef bool
    #define bool int
    #define false ((bool)0)
    #define true  ((bool)1)
#endif

pthread_t th[MAX_THREAD_NUM];
struct th_arg_s th_arg[MAX_THREAD_NUM];
int numOfHacker, numOfSerf;

void* hacker(void *arg);
void* serf(void *arg);

static pthread_mutex_t safe_mutex = PTHREAD_MUTEX_INITIALIZER; //total and safe
static pthread_mutex_t oar = PTHREAD_MUTEX_INITIALIZER;	//to row

static pthread_cond_t cond  = PTHREAD_COND_INITIALIZER;//Waiting to board
static pthread_cond_t condRow  = PTHREAD_COND_INITIALIZER;//Waiting to row
static int safe = 0, total =0 , convert =0;
static int allowableHackers = -1, allowableSerfs=-1; 

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

void broadcast_signal(void)
{
	int i;
	pthread_mutex_lock(&safe_mutex);
	//myprintf("\n Broadcasting to : [%d]",numOfHacker+numOfSerf);
	for ( i=0 ; i<(numOfHacker+numOfSerf) ; i++ ) {
		if ( th_arg[i].state ){			
			//myprintf("\n Broadcasting to : [%08x]",th[i]);
			pthread_kill(th[i], SIGUSR1);
		}
	}
	pthread_mutex_unlock(&safe_mutex);
}

void deplane_signal_handler(int signum){
//write(0, "Signal handler : Attempting to lock\n", 35);
	while(pthread_mutex_trylock(&oar));
	pthread_mutex_unlock(&oar);
//write(0, "Signal handler : Got lock .Unlocking and leaving\n", 33);
	return;
}
void ignore(int signum){
//	write(0, "Signal handler0 : Ignoring\n", 28);
	return;
}


bool isSafeToBoard(void *arg){
	struct th_arg_s *th_arg = (struct th_arg_s *)(arg);
	int isHacker = th_arg->id;

	pthread_mutex_lock(&safe_mutex);


	total++;safe+=isHacker;
	//if(alarmtw == true) 
	//	alarmtw = false;
	if(total<4){//LESS THAN FOUR DONT GET TO GO.

		pthread_mutex_unlock(&safe_mutex);
		return false;
	}
	else {pthread_mutex_unlock(&safe_mutex);return (isStillSafeToBoard(arg));}
	
	
}
/*	
Purpose : When its time to board and it is queried, this function
	decides who will board (based on total and safe) and sets 
	appropriate values(allowableHackers)
Policy :  If enough homogeneuos people then allow them.
	Else 2/3S2H or 
*/
int isStillSafeToBoard(void *arg){
struct th_arg_s *th_arg = (struct th_arg_s *)(arg);
	int isHacker = th_arg->id;

	//if(isHacker) pintf("\nHacker asks..");
	//else pintf("\nSerf asks..");
	pthread_mutex_lock(&safe_mutex);
		//myprintf("\nTotal = %d",total);myprintf("\tsafe = %d\n",safe);
		//myprintf("AllwdHack = %d",allowableHackers);myprintf("\tAllwdSerfs = %d\n",allowableSerfs);

	if(allowableHackers == -1&& allowableSerfs ==-1 ){//not scheduled so schedule;
		//pintf("\n------------------------\n");
		if(safe >= 4){	
			//pintf("\nSetting hackers to4 \n");
			allowableHackers = 4;
			allowableSerfs = 0;
		}else if(total - safe >=4){
			allowableHackers = 0;
			allowableSerfs = 4;
		}else if(safe==3 && total-safe ==1){
			allowableHackers =-1;
			allowableSerfs = -1;
			pthread_mutex_unlock(&safe_mutex);
			return 0;
		}else if(total<4 ){
			allowableHackers = -1;
			allowableSerfs = -1;	
			pthread_mutex_unlock(&safe_mutex);
			return 0;	
		}else {	
			//pintf("\n.Simple schedule .\n");
			allowableSerfs=total-safe;
			allowableHackers= 4 - allowableSerfs;
			if(allowableHackers ==1)
				convert=1;
		}
	}
	//pintf("\n.Askin am i Safe ? .\n");
	if(isHacker)
		if(allowableHackers > 0){
			allowableHackers--;
			pthread_mutex_unlock(&safe_mutex);
			return 1;
		}
		else {	
			pthread_mutex_unlock(&safe_mutex);
			return 0;}
	else
		if(allowableSerfs > 0){
			allowableSerfs--;
			pthread_mutex_unlock(&safe_mutex);
			return 1;
		}

		else {	pthread_mutex_unlock(&safe_mutex);
			return 0;}
}

void* bl_here(void *arg,int i){
struct th_arg_s *th_arg = (struct th_arg_s *)(arg);
	sigset_t sigusr_set;
	int isHacker = th_arg->id;
	sigemptyset(&sigusr_set);
	sigaddset(&sigusr_set,SIGUSR1);

	pthread_mutex_lock(&safe_mutex);
		//myprintf("\nTotal = %d",total);myprintf("\tsafe = %d\n",safe);
		//myprintf("AllwdHack = %d",allowableHackers);myprintf("\tAllwdSerfs = %d\n",allowableSerfs);
	if(i!=3){/*Leaves after deplaning and not because of waiting.*/
	if(isHacker)
		safe--;
	total--;
	}
	if(isHacker  && convert ==1 && i!=0){	
		int *try ; try = &th_arg->id;
		*try = 0; //Hacker is recruited
		convert = 0;		
	}
	pthread_mutex_unlock(&safe_mutex);
	if(i==0)
	board(arg);
	else
	leave(arg);
}

void* hacker(void *arg)
{
struct th_arg_s *th_arg = (struct th_arg_s *)(arg);
	int rc,rct,set;
	bool iWasRower = false;
	int isHacker = th_arg->id;
	struct timeval tv;
    struct timespec ts;
	th_arg->state = 1;

	struct sigaction sa;
	/*Define Signal Handler*/
	sigset_t sigusr_set;
	/*Define set for use ahead*/
	sigemptyset(&sigusr_set);
	sigaddset(&sigusr_set,SIGUSR1);

	sa.sa_handler = ignore;
	sigemptyset( &sa.sa_mask);
	sigaddset( &sa.sa_mask ,SIGUSR1);
  	sigaction (SIGUSR1, &sa, NULL);
	pthread_sigmask(SIG_UNBLOCK, &sigusr_set , NULL); //Get ready to be signalled by captain
	
	rc =  gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec + 3;
        ts.tv_nsec = 0;
	//pintf("\nInside hacker");

	/*BOARDING LOGIC STARTS HERE*/
	rc = isSafeToBoard(arg);
	if( rc == 0){//NOT SAFE
waittag:	/*DEBUG*///pintf("Going in Wait");
		/*pthread_mutex_lock(&safe_mutex);
		pthread_cond_wait(&cond,&safe_mutex);		
		pthread_mutex_unlock(&safe_mutex);*/
		//pintf("\nAwakeNow");
		//if(alarmtw == true){//
			//if(isTimeToWait(arg)){//Not all do timed wait.
		pthread_mutex_lock(&safe_mutex);
		rct = pthread_cond_timedwait(&cond,&safe_mutex,&ts);
		pthread_mutex_unlock(&safe_mutex);
	    if (rct == ETIMEDOUT) {
			//pintf("\nWait timed out!\n");
			bl_here(arg,1);
			pthread_exit(NULL);
		}	


tryboarding:	
		if((rc = isStillSafeToBoard(arg))==1)
			bl_here(arg,0);
		else if(rc == 0) goto waittag;
	}
	else if(rc == 1){
		bl_here(arg,0);
		//pintf("\nBroadcasting for others to board\n");
		pthread_cond_broadcast(&cond);
	}

	/*ROWING LOGIC STARTS HERE*/
	
	if(peopleOnBoard() != 4 ){
		//pintf("Non Rower: Waiting to deplane\n");
		sa.sa_handler = deplane_signal_handler;
	  	sigaction (SIGUSR1, &sa, NULL);
		sigwait(&sigusr_set,&set);
		//pintf("Non Rower: Back from sigwait\n");
		deplane(arg);
		//pintf("Non Rower: Restting sighandler to ignore\n");
		sa.sa_handler = ignore;
	  	sigaction (SIGUSR1, &sa, NULL);
		//pintf("Non Rower: Leaving Oar\n");
	}
	else{
		//pintf("Rower: Picking oar\n");
		pthread_mutex_lock(&oar);
		rowBoat(arg);	
		iWasRower = true;
		pthread_mutex_unlock(&oar);
		//pintf("Rower: Leaving oar , blocking and broadcasting\n");
		//wake people to deplane
		pthread_sigmask(SIG_BLOCK, &sigusr_set , NULL);//block else ill ignore
		broadcast_signal();
	}	
	

	/*DEPLANING*/
	if(iWasRower){
		while(peopleOnBoard()!=1){//Do nothing as you have already broadcasted and done  your deed.:)
		}
		//pintf("Rower: Resetting sghandler adn unblocking\n");

		sa.sa_handler = deplane_signal_handler;
	  	sigaction (SIGUSR1, &sa, NULL);
		//pintf("Rower: Unblocking to deplane\n");
		pthread_sigmask(SIG_UNBLOCK, &sigusr_set , NULL);//Read the last signal broadcasted.
		//pintf("Rower: Waiting to deplane\n"); Do not insert Sigwait here...it causes problems.
		//pintf("Rower: Back from sigwait\n");

		deplane(arg);
		sa.sa_handler = ignore;
	  	sigaction (SIGUSR1, &sa, NULL);
		//pintf("Rower: Leaving oar after deplane and resetting handler to ignore\n");
	}
	
	
	if(iWasRower){
		iWasRower=false;
		pthread_mutex_lock(&safe_mutex);//Explicitly scheduling
		allowableHackers=-1;
		allowableSerfs=-1;
		total++;
		if(isHacker) safe++;
		pthread_mutex_unlock(&safe_mutex);
		//pintf("\nBroadcasting to board\n");
		pthread_cond_broadcast(&cond);//wake people to start boarding again
		goto tryboarding;}

	bl_here(arg,3);
	th_arg->state = 0;
	pthread_exit(NULL);
}

void* serf(void *arg)
{
	//pintf("\nInside serf");
	hacker(arg);
	//pthread_exit(NULL);
}

void initialize(void)
{
	allowableHackers = -1; allowableSerfs=-1; 
	safe = 0; total =0;
}

int main(int argc, char** argv)
{

	int i;
	sigset_t sigusr_set;
	sigemptyset(&sigusr_set);
	sigaddset(&sigusr_set,SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &sigusr_set,NULL);

	while(1) {
		scanf("%d%d", &numOfHacker, &numOfSerf);

		if ( numOfHacker == 0 && numOfSerf == 0 )
			break;

		for ( i=0 ; i<(numOfHacker+numOfSerf) ; i++ ){
			scanf("%d", &(th_arg[i].id));
			th_arg[i].state = 0;
		}

		initialize();

		for ( i=0 ; i<(numOfHacker+numOfSerf) ; i++ ) {
			if ( th_arg[i].id == 1 )
				pthread_create(&(th[i]), NULL, hacker, (void*)(&th_arg[i]));
			else
				pthread_create(&(th[i]), NULL, serf, (void*)(&th_arg[i]));
		}

		for ( i=0 ; i<(numOfHacker+numOfSerf) ; i++ )
			pthread_join(th[i], NULL);
		
		pintf("\n------------------------\n");
	}

	return 0;
}

