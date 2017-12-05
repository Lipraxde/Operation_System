#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int NUM_CHAIRS;
int NUM_BARBERS;

int free_chair;

struct customer
{
    // sem_t sem_customer;
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
    // sem_init(&, 0, 0);
    assert(pthread_mutex_init(&(temp->mutex), NULL)==0);
    assert(pthread_mutex_lock(&(temp->mutex))==0);
    temp->self = temp;
    temp->next = NULL;
    return temp;
}   

void destory_customer(struct customer *target)
{
    assert(pthread_mutex_destroy(&(target->mutex))==0);
    free(target);
}

int add_customer(struct customer *cust)
{
    assert(pthread_mutex_lock(&mutex_customer_list)==0);
    
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
        assert(pthread_mutex_unlock(&mutex_customer_list)==0);
        return 0;   // The barbershop is full.
    }

    assert(pthread_mutex_unlock(&mutex_customer_list)==0);
    return 1;       // The customer can sit on chair.
}


struct customer *get_customer(void)
{
    struct customer *temp = NULL;
    assert(pthread_mutex_lock(&mutex_customer_list)==0);

    if(waiting_customer)
    {
        temp = waiting_customer;
        waiting_customer = waiting_customer->next;
        free_chair++;
    }

    assert(pthread_mutex_unlock(&mutex_customer_list)==0);
    return temp;
}

struct barber
{
    pthread_t tid;
    struct customer *cut_who;
};
pthread_mutex_t mutex_barber_count;
int barber_count;
sem_t sem_barber;
struct barber *barbers;


int wakeup_barber(void)
{ 
    assert(pthread_mutex_lock(&mutex_barber_count)==0);
    if(barber_count>0)
    {
        sem_post(&sem_barber);
        barber_count--;
        assert(pthread_mutex_unlock(&mutex_barber_count)==0);
        return 1;   // A barber is awakened.
    }
    else
        assert(pthread_mutex_unlock(&mutex_barber_count)==0);
    return 0;   // All barbers are busy.
}

void sleep_barber(struct barber *target)
{
    assert(pthread_mutex_lock(&mutex_barber_count)==0);
    barber_count++;
    assert(pthread_mutex_unlock(&mutex_barber_count)==0);
    sem_wait(&sem_barber);
}

struct barber *init_barbers(int n)
{
    struct barber *temp;
    temp = malloc(n * sizeof(struct barber));
    assert(temp!=NULL);
    for(int i=0;i<n;i++)
    {
        temp->tid = 0;
        temp->cut_who = NULL;
    }
    return temp;
}


void *thread_barber(void *_self)
{
    struct barber *self = _self;

    while(1)
    {
        sleep_barber(self);
        printf("Barber %p wake up.\n", self);
        self->cut_who = get_customer();
        while(self->cut_who)
        {
            // cutting hair
            printf("Barber %p cuts customer %p's hair.\n",
                   self, self->cut_who);
            sleep(1);

            assert(pthread_mutex_unlock(&(self->cut_who->mutex))==0);
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
        assert(pthread_mutex_lock(&(self->mutex))==0);
        printf("Customer %p have cut hair.\n", self);
    }
    else
        printf("Barbershop is full, customer %p leave.\n", self);

    assert(pthread_mutex_unlock(&(self->mutex))==0);
    destory_customer(self);
    pthread_exit(NULL);
}

void *test(void *p)
{
    assert(pthread_detach(pthread_self())==0);
    free(p);
    pthread_exit(NULL);
}

pthread_mutex_t mutex_mem;
void *me_malloc(size_t size)
{
    void *temp = 0;
    pthread_mutex_lock(&mutex_mem);
    temp = malloc(size);
    pthread_mutex_unlock(&mutex_mem);
    return temp;
}

void me_free(void *target)
{
    pthread_mutex_lock(&mutex_mem);
    me_free(target);
    pthread_mutex_unlock(&mutex_mem);
}


int main(int argc, char *argv[])
{
    pthread_t tid;
    void *p;
    // void *ret_p;
    pthread_mutex_init(&mutex_mem, NULL);
    // while(1)
    for(int i=0;i<10;i++)
    {
        p = me_malloc(8*1024*1024);
        assert(p!=NULL);
        // printf("Pointer p = %p\n", p);
        assert(pthread_create(&tid, NULL, test, p)==0);
        // assert(pthread_join(tid, &ret_p)==0);
        sleep(0);
    }
    return 0;
    while(1) sleep(0);



    printf("Input how many barbers and how many chair?\n");
    printf("(Default 2 barbers, 10 chairs)\n");
    scanf("%d %d", &NUM_BARBERS, &NUM_CHAIRS);
    if(!NUM_BARBERS) NUM_BARBERS = 2;
    if(!NUM_CHAIRS) NUM_CHAIRS = 10;
    free_chair = NUM_CHAIRS;

    barber_count = 0;   // Will add at barber go sleeping.
    barbers = init_barbers(NUM_BARBERS);
    pthread_mutex_init(&mutex_barber_count, NULL);
    sem_init(&sem_barber, 0, 0);

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
        pthread_t tid_customer;
        struct customer *new_customer;

        printf("How many customers?\n");
        scanf("%d", &n_customer);
        for(int i=0;i<n_customer;i++)
        {
            new_customer = create_customer();
            pthread_create(&tid_customer, NULL,
                           thread_customer, new_customer);
            pthread_detach(tid_customer);
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

