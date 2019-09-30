#include "curses.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

/**
 * Creator: Felix Reisinger
 * Date: September 2019
 * 
 * Notes: This is a small game in which the protagonist ("Jack") has to jump over
 *        obstacles in order to reach the finish. 
 *        It uses the curses library to draw the screen and threads to handle user input.
 */

// Definition of screen layout.
#define SCREEN_HEIGHT 10     // height of game window
#define SCREEN_WIDTH  50     // width of game window

// Definition of the characters of Jack (the player) and obstacles.
#define Jack          'i'
#define OBSTACLE      '|'
#define ROAD          '_'

// Definition of user input.
#define LEFT         1   // 'a'
#define RIGHT        2   // 'd'
#define UP           3   // 'w' or 'space'
#define QUIT         5   // 'q'

// Variables that hold the state of the game.
unsigned int game_over;
unsigned int game_won;
unsigned int error;
unsigned int quit;

// The position of the elements on the screen is represented by Cartesian coordinates.
typedef struct
{
    int x;
    int y;
} Position;

// The road is represented by a linked list of tiles.
// Every element includes its own type and position and a pointer to the next element of the list.
typedef struct Tile
{
    char type;
    Position position;
    struct Tile* next;
} Tile;

// Represents the road.
Tile* road;

// Holds the player's position.
Position player;

// Global variables.
unsigned int game_speed = 250000;
unsigned int score = 0;
unsigned int score_for_win = 0;

char input;
int air_time;

WINDOW* screen;
int old_cursor = 0;

/**
 * Draws the screen with the current coordinates of tiles and the player.
 */
void drawScreen()
{
    werase(screen);
    box(screen, ' ', '~');

    // Draw Jack (the player).
    move(player.y, player.x);
    addch(Jack);

    // Draw score.
    char score_[10];
    sprintf(score_, "%d", score);
    mvprintw(1, 1, "Meters: ");
    mvprintw(1, 9, score_);

    // Draw the road.
    Tile* active_road = road;
    while(active_road)
    {
        if(active_road->position.x != player.x ||
            (active_road->position.x == player.x && 
            active_road->position.y != player.y))
        {
            move(active_road->position.y, active_road->position.x);
            addch(active_road->type);
        }

        active_road = active_road->next;
    }

    refresh();
}

/**
 * Draws the initial screen with instructions.
 */
void drawInstructionGameScreen()
{
    werase(screen);
    box(screen, 0, 0);
    move(SCREEN_HEIGHT/2-3, 8);
    printw("Jump and Run, Jack!");
    move(SCREEN_HEIGHT/2-1, 8);
    printw("Your Goal: Finish the Track");
    move(SCREEN_HEIGHT/2, 8);
    printw("Controls: 'a', 'd', 'w'/'space'");
    move(SCREEN_HEIGHT/2+3, 8);
    printw("Press E(asy), N(ormal), H(ard) or Q(uit).");
    
    refresh();
    
    char c;
    while((c = getchar()))
    {
        switch (c)
        {
            case 'e':
            case 'E':
                game_speed = 300000;
                score_for_win = 100;
                return;
            case 'n':
            case 'N':
                game_speed = 150000;
                score_for_win = 200;
                return;
            case 'h':
            case 'H':
                game_speed = 75000;
                score_for_win = 400;
                return;
            case 'q':
            case 'Q':
                quit = 1;
                return;
        }
    }
}

/**
 * Draws the end of game screen.
 */
void drawEndOfGameScreen()
{
    werase(screen);
    box(screen, 0, 0);
    move(SCREEN_HEIGHT/2-1, 8);
    if(game_won)
        printw("You won! Congratulations!");
    else if(game_over)
        printw("Game over. You lost!");
    else if(quit == 1)
        printw("You quit.");
    move(SCREEN_HEIGHT/2+1, 8);
    printw("Press any key to exit.");
    
    refresh();
    
    usleep(1000000);
    getchar();
}

/**
 * Initializes the road with road tiles.
 */
void initRoad()
{
    road = (Tile*)malloc(sizeof(Tile));

    if(!road)
    {
        error = 1;
        return;
    }

    Tile*road_tmp = road;

    road_tmp->type = ROAD;
    road_tmp->position.x = 0;
    road_tmp->position.y = SCREEN_HEIGHT - 3;
    road_tmp->next = 0;

    int i;
    for(i = 0; i < SCREEN_WIDTH; i++)
    {
        Tile* new_tile = (Tile*)malloc(sizeof(Tile));
        if (!new_tile)
        {
            error = 1;
            return;
        }

        new_tile->type = ROAD;
        new_tile->position.x = i;
        new_tile->position.y = SCREEN_HEIGHT - 3;
        new_tile->next = 0;

        road_tmp->next = new_tile;
        road_tmp = road_tmp->next;
    }

}

/**
 * Shifts the tiles to the left; one coordinate per turn.
 */
void moveTiles()
{
    Tile* active_tile = road;
    Tile* previous_tile = road;
    Tile* following_tile = 0;

    while(active_tile)
    {
        following_tile = active_tile->next;
        
        if(following_tile)
        {
            active_tile->type = following_tile->type;
        }
        // Create right-most tile, which can be an obstacle or a road.
        else
        {
            if(previous_tile->type != OBSTACLE)
            {
                int random_number = rand() % 10; 

                if(random_number < 2)
                {
                    active_tile->type = OBSTACLE;

                    move(active_tile->position.y, active_tile->position.x);
                    addch(OBSTACLE);
                }
                else
                {
                    active_tile->type = ROAD;

                    move(active_tile->position.y, active_tile->position.x);
                    addch(ROAD);
                }
                
            }
            else
            {
                active_tile->type = ROAD;

                move(active_tile->position.y, active_tile->position.x);
                addch(ROAD);
            }

            score++;

            if(score == score_for_win)
            {
                game_won = 1;
                return;
            }

            return;
        }

        if(active_tile->type == OBSTACLE && 
            active_tile->position.x == player.x &&
            active_tile->position.y == player.y)
        {
            game_over = 1;
            return;
        }

        active_tile = following_tile;
        previous_tile = active_tile;
        
        refresh(); 
    }
}

/**
 * Moves the player (Jack).
 */
void movePlayer()
{
    if(input == 'a' && (player.x > 0))
    { 
        player.x--;
    }
    
    if(input == 'd' && (player.x < (SCREEN_WIDTH)))
    {
        player.x++;
    }

    if ((input == 'w' || input == ' '))
    {
        player.y--;
        air_time = 3;
        
        for(air_time = 3; air_time > 0; air_time--)
        {
            usleep(game_speed);
        }
        
        player.y++;
        air_time = 0;
    }

    if(input == 'q')
    {
        quit = 1;
    }
}
 
/**
 * Reads the user input.
 */
void *readInput()
{
    while((input = getchar()) && !(quit || error || game_over))
    {
        movePlayer();
        refresh();

        if (quit || error || game_over || game_won)
        {
            break;
        }

        usleep(10000);
    }

    return 0;
}

/**
 * Frees the allocated memory for road.
 */
void freeRoad()
{
    Tile* current_tile = road;
    Tile* next_tile = 0;
    
    while(current_tile)
    {
        next_tile = current_tile->next;
        free(current_tile);

        current_tile = next_tile;
    }
}

/**
 * Main function.
 */
int main(int argc, char** argv)
{
    screen = initscr();
    
    if(screen == NULL)
        exit(-1);
    wresize(screen, SCREEN_HEIGHT, SCREEN_WIDTH);
    old_cursor = curs_set(0);

    player.x = SCREEN_WIDTH * 0.3;
    player.y = SCREEN_HEIGHT - 3;

    // Set up a thread for the user input.
    pthread_t tid;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_create(&tid, &attr, readInput, NULL);

    // Start the game.
    drawInstructionGameScreen();

    initRoad();

    while(!quit && !error && !game_over && !game_won)
    {
        moveTiles();
        drawScreen();
        usleep(game_speed);
    }

    drawEndOfGameScreen();

    // Clean up.
    delwin(screen);
    curs_set(old_cursor);
    endwin();
    refresh();

    freeRoad();

    pthread_cancel(tid);
    pthread_exit(0);

    return 0;
}