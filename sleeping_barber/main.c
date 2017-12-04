#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int NUM_CHAIRS;
int NUM_BARBERS;

enum barber_state { SLEEPING, CUTTING };
int free_chair;

struct customer
{
    pthread_t tid;
    pthread_mutex_t mutex;
    struct customer *self;
    struct customer *next;
};
pthread_mutex_t mutex_customer_list;
struct customer *waiting_customer;
struct customer *last_customer;

struct customer *create_customer(void)
{
    struct customer *temp;
    temp = malloc(sizeof(struct customer));
    assert(temp!=NULL);
    temp->tid = 0;
    pthread_mutex_init(&(temp->mutex), NULL);
    pthread_mutex_lock(&(temp->mutex));
    temp->self = temp;
    temp->next = NULL;
    return temp;
}   

void destory_customer(struct customer *target)
{
    pthread_mutex_destroy(&(target->mutex));
    free(target);
}

int add_customer(struct customer *cust)
{
    pthread_mutex_lock(&mutex_customer_list);
    
    if(free_chair>0)
    {
        if(waiting_customer)
            last_customer->next = cust;
        else
            waiting_customer = cust;
        last_customer = cust;
        free_chair--;
    }
    else
    {
        pthread_mutex_unlock(&mutex_customer_list);
        return 0;   // The barbershop is full.
    }

    pthread_mutex_unlock(&mutex_customer_list);
    return 1;       // The customer can sit on chair.
}


struct customer *get_customer(void)
{
    struct customer *temp = NULL;
    pthread_mutex_lock(&mutex_customer_list);

    if(waiting_customer)
    {
        temp = waiting_customer;
        waiting_customer = waiting_customer->next;
        free_chair++;
    }

    pthread_mutex_unlock(&mutex_customer_list);
    return temp;
}

struct barber
{
    pthread_t tid;
    pthread_mutex_t mutex;
    struct customer *cut_who;
};
struct barber *barbers;

int wakeup_barber(void)
{
    static int i = 0;
    int j = i;
    int get = 0;
    do
    {
        i = (i + 1) % NUM_BARBERS;
        if(barbers[i].cut_who==NULL)
        {
            get = 1;
            break;
        }
    } while(i!=j);
    if(get)
    {
        barbers[i].cut_who = get_customer();
        pthread_mutex_unlock(&(barbers[i].mutex));
        return 1;   // A barber is awakened.
    }
    else
        return 0;   // All barbers are busy.
}

struct barber *init_barbers(int n)
{
    struct barber *temp;
    temp = malloc(n * sizeof(struct barber));
    assert(temp!=NULL);
    for(int i=0;i<n;i++)
    {
        temp->tid = 0;
        pthread_mutex_init(&(temp[i].mutex), NULL);
        pthread_mutex_lock(&(temp[i].mutex));
        temp->cut_who = NULL;
    }
    return temp;
}


void *thread_barber(void *_self)
{
    struct barber *self = _self;

    while(1)
    {
        pthread_mutex_lock(&(self->mutex));
        printf("Barber %p wake up.\n", self);
        while(self->cut_who)
        {
            // cutting hair
            printf("Barber %p cuts customer %p's hair.\n",
                   self, self->cut_who);
            sleep(1);
            pthread_mutex_unlock(&(self->cut_who->mutex));

            self->cut_who = get_customer();
        }
        printf("Barber %p sleep.\n", self);
        self->cut_who = NULL;
    }
}


void *thread_customer(void *_self)
{
    struct customer *self = _self;

    if(add_customer(self))
    {
        wakeup_barber();
        pthread_mutex_lock(&(self->mutex));
        printf("Customer %p have cut hair.\n", self);
    }
    else
        printf("Barbershop is full, customer %p leave.\n", self);

    destory_customer(self);
    return NULL;
}


int main(int argc, char *argv[])
{
    printf("Input how many barbers and how many chair?\n");
    printf("(Default 2 barbers, 10 chairs)\n");
    scanf("%d %d", &NUM_BARBERS, &NUM_CHAIRS);
    if(!NUM_BARBERS) NUM_BARBERS = 2;
    if(!NUM_CHAIRS) NUM_CHAIRS = 10;
    free_chair = NUM_CHAIRS;

    barbers = init_barbers(NUM_BARBERS);
    waiting_customer= NULL;
    last_customer = NULL;
    pthread_mutex_init(&mutex_customer_list, NULL);

    for(int i=0;i<NUM_BARBERS;i++)
    {
        pthread_create(&(barbers[i].tid) , NULL,
                       thread_barber, &(barbers[i]));
        printf("Create barber %p.\n", &(barbers[i]));
    }

    while(1)
    {
        int n_customer = 0;
        struct customer *who_going_barbershop;
        printf("How many customers?\n");
        scanf("%d", &n_customer);
        for(int i=0;i<n_customer;i++)
        {
            who_going_barbershop = create_customer();
            pthread_create(&(who_going_barbershop->tid) , NULL,
                           thread_customer, who_going_barbershop);
        }
        sleep(1);
        intptr_t p = 1;
        while(p)
        {
            p = 0;
            for(int i=0;i<NUM_BARBERS;i++)
                p = (intptr_t)barbers[i].cut_who|p;
            sleep(0);
        }
    }
    return 0;
}

