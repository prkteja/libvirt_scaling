#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#define PORT 5000
#define num_vms 2

// const int num_vms = 2;
int num_active = 1;
const char* ips[] = {"192.168.122.3", "192.168.122.4", "192.168.122.5"};
int sockets[num_vms];
int queue[num_vms][100];
int num_requests[num_vms];
int threadids[num_vms];
pthread_t threads[num_vms];
pthread_mutex_t locks[num_vms];
pthread_cond_t cvs[num_vms];
pthread_t update_active;
pthread_mutex_t threadlock;

void *run(void* data){
    int id = *(int *)data;
    while(1){
        printf("thread %d\n", id);
        pthread_mutex_lock(locks+id);
        while(num_requests[id]==0){
            pthread_cond_wait(cvs+id, locks+id);
        }
        char str[1024] = {0};
        sprintf(str, "%d", queue[id][num_requests[id]-1]);
        send(sockets[id], str, strlen(str), 0);
        char buffer[1024] = {0};
        read(sockets[id], buffer, 1024);
        printf("%s : %s\n", str, buffer);
        num_requests[id]--;
        pthread_mutex_unlock(locks+id);
    }
}

void *checkactive(void *data){
    while(1){
        sleep(1);
        FILE* file = fopen("num_active", "r");
        if(!file) {
            printf("could not open file\n");
            continue;
        }
        int i = 0;
        fscanf(file, "%d", &i);
        printf("num active = %d\n", i);
        fclose(file);
        if(i > num_active){
            pthread_mutex_lock(&threadlock);
            i = num_active;
            struct sockaddr_in serv_addr;
            if ((sockets[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                printf("\n Socket creation error \n");
                continue;
            }
        
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT);
            
            if(inet_pton(AF_INET, ips[i], &serv_addr.sin_addr)<=0) 
            {
                printf("\nInvalid address/ Address not supported \n");
                continue;
            }

            if (connect(sockets[i], (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            {
                printf("\nConnection Failed \n");
                continue;
            }
            pthread_create(threads+num_active, NULL, run, (void *)(threadids+num_active));
            num_active++;
            pthread_mutex_unlock(&threadlock);
        }
    }
}

int main(int argc, char const *argv[])
{
    for(int i=0; i<num_active; i++){
        struct sockaddr_in serv_addr;

        if ((sockets[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("\n Socket creation error \n");
            return -1;
        }
    
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);
        
        if(inet_pton(AF_INET, ips[i], &serv_addr.sin_addr)<=0) 
        {
            printf("\nInvalid address/ Address not supported \n");
            return -1;
        }

        if (connect(sockets[i], (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("\nConnection Failed \n");
            return -1;
        }
    }

    for(int i=0; i<num_vms; i++){
        num_requests[i] = 0;
        threadids[i] = i;
    }
   
    for(int i=0; i<num_active; i++){
        int id = i;
        pthread_create(threads+i, NULL, run, (void *)(threadids+i));
    }

    pthread_create(&update_active, NULL, checkactive, NULL);

    long long int num = 10000;
    while(1){
        if(num<11000) usleep(10000);
        else usleep(100);
        num++;
        pthread_mutex_lock(&threadlock);
        int threadno = num%num_active;
        pthread_mutex_unlock(&threadlock);
        pthread_mutex_lock(locks+threadno);
        if(num_requests[threadno]>=100) {
            pthread_mutex_unlock(locks+threadno);
            continue;
        }else{
            queue[threadno][num_requests[threadno]] = num;
            num_requests[threadno]++;
            pthread_cond_broadcast(cvs+threadno);
            pthread_mutex_unlock(locks+threadno);
        }
    }

    return 0;
}
