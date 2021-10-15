/*
 * catsem.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use SEMAPHORES to solve the cat syncronization problem in 
 * this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>


/*
 * 
 * Constants
 *
 */

/*
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2

static struct semaphore *mutex;
static struct semaphore *cats_queue;
static struct semaphore *mice_queue;
static struct semaphore *dish_mutex;
static struct semaphore *cat_done;
static struct semaphore *mouse_done;

int dish1_busy = 0;
int dish2_busy = 0;
int no_cat_eat = 1;
int all_dishes_avail = 1;
int first_cat_eat = 0;
int another_cat_eat = 0;
int first_mouse_eat = 0;
int another_mouse_eat = 0;
int no_mouse_eat = 1;


int cat_wait_count = 0;
int mice_wait_count = 0;


/*
 * 
 * Function Definitions
 * 
 */


static void FirstCatNoMouse(int catNum){
        P(mutex);
        kprintf("\nCat %d is hungry", catNum);
        if(all_dishes_avail){
                all_dishes_avail = 0;
                V(cats_queue);
        }
        cat_wait_count++;
        V(mutex);
        P(cats_queue);
        if(no_cat_eat){
                no_cat_eat = 0;
                first_cat_eat = 1;
        } else {
                first_cat_eat = 0;
        }
}

static void FirstCat(int catNum){

        if(first_cat_eat){
                P(mutex);
                if(cat_wait_count>1){
                        another_cat_eat = 1;
                        V(cats_queue);
                }
                V(mutex);
        }
        kprintf("\nCat %d is in the kitchen", catNum);

}

static void CatDishes(int *myDish, int catNum){
        P(dish_mutex);
        if(!dish1_busy){
                dish1_busy = 1;
                myDish = 1;
        } else {
                assert(!dish2_busy);
                dish2_busy = 1;
                myDish = 2;
        }
        V(dish_mutex)
        kprintf("\nCat %d is eating from dish %d", catNum, myDish);
        clocksleep(1);
        kprintf("\nCat %d is finished eating from dish %d", catNum, myDish);
}

static void releaseDishCat(int *myDish){
        P(mutex);
        P(dish_mutex);
        if(myDish==1){
                dish1_busy = 0;
        } else { 
                assert(myDish==2);
                dish2_busy = 0;
        }
        V(dish_mutex);
        cat_wait_count--;
        V(mutex);
}

static void catsLeaving(int catNum){
        if(first_cat_eat){
                if(another_cat_eat){
                        P(cat_done);
                }
                //print first cat leaving;
                no_cat_eat = 1;
                switchTurnsCat();
        } else {
                V(cat_done);
        }
        kprintf("\nCat %d is leaving the kitchen", catNum);
}

static void switchTurnsCat(){
        P(mutex);
        if(mice_wait_count>0){
                V(mice_queue);
        } else {
                if(cat_wait_count>0){
                        V(cats_queue);
                } else {
                        all_dishes_avail = 1;
                }
        }
        V(mutex);
}




static void FirstMouseNoCat(int mouseNum){
        P(mutex);
        kprintf("\nMouse %d is hungry", mouseNum);
        if(all_dishes_avail){
                all_dishes_avail = 0;
                V(mice_queue);
        }
        mice_wait_count++;
        V(mutex);
        P(mice_queue);
        if(no_mouse_eat){
                no_mouse_eat = 0;
                first_mouse_eat = 1;
        } else {
                first_mouse_eat = 0;
        }
}

static void FirstMouse(int mouseNum){

        if(first_mouse_eat){
                P(mutex);
                if(mice_wait_count>1){
                        another_mouse_eat = 1;
                        V(mice_queue);
                }
                V(mutex);
        }
        kprintf("\nMouse %d is in the kitchen", mouseNum);

}

static void MouseDishes(int *myDish, int mouseNum){
        P(dish_mutex);
        if(!dish1_busy){
                dish1_busy = 1;
                myDish = 1;
        } else {
                assert(!dish2_busy);
                dish2_busy = 1;
                myDish = 2;
        }
        V(dish_mutex)
        kprintf("\nMouse %d is eating from dish %d", mouseNum, myDish);
        clocksleep(1);
        kprintf("\nMouse %d is finished eating from dish %d", mouseNum, myDish);
}

static void releaseDishMouse(int *myDish){
        P(mutex);
        P(dish_mutex);
        if(myDish==1){
                dish1_busy = 0;
        } else { 
                assert(myDish==2);
                dish2_busy = 0;
        }
        V(dish_mutex);
        mice_wait_count--;
        V(mutex);
}

static void mouseLeaving(int mouseNum){
        if(first_mouse_eat){
                if(another_mouse_eat){
                        P(mouse_done);
                }
                no_mouse_eat = 1;
                switchTurnsMouse();
        } else {
                V(mouse_done);
        }
        kprintf("\nMouse %d is leaving the kitchen", mouseNum);
}

static void switchTurnsMouse(){
        P(mutex);
        if(cats_wait_count>0){
                V(cats_queue);
        } else {
                if(mice_wait_count>0){
                        V(mice_queue);
                } else {
                        all_dishes_avail = 1;
                }
        }
        V(mutex);
}
/*
 * catsem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
catsem(void * unusedpointer, 
       unsigned long catnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) unusedpointer;
        (void) catnumber;
        int myDish = 0;

        FirstCatNoMouse(catnumber);
        FirstCat(catnumber);
        CatDishes(&myDish, catnumber);
        releaseDishCat(&myDish);
        catsLeaving(catnumber);
        switchTurnsCat();


}
        

/*
 * mousesem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
mousesem(void * unusedpointer, 
         unsigned long mousenumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) unusedpointer;
        (void) mousenumber;

        int myDish = 0;

        FirstMouseNoCat(mousenumber);
        FirstMouse(mousenumber);
        MouseDishes(&myDish, mousenumber);
        releaseDishMouse(&myDish);
        MouseLeaving(mousenumber);
        switchTurnsMouse();



}


/*
 * catmousesem()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catsem() and mousesem() threads.  Change this 
 *      code as necessary for your solution.
 */

int
catmousesem(int nargs,
            char ** args)
{
        int index, error;
   
        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;

        bowls = sem_create("bowls", 2);
   
        /*
         * Start NCATS catsem() threads.
         */

        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catsem Thread", 
                                    NULL, 
                                    index, 
                                    catsem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catsem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }
        
        /*
         * Start NMICE mousesem() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mousesem Thread", 
                                    NULL, 
                                    index, 
                                    mousesem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mousesem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        return 0;
}


/*
 * End of catsem.c
 */
