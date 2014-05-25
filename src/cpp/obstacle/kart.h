#ifndef KART_H
#define KART_H

#include <iostream>

#define BIGCOST 500
#define GRIDSIZE 12
#define SQRT2 1.4142136
const bool EMPTY = false;
const bool FULL = true;

extern int occupancy[GRIDSIZE][GRIDSIZE];
extern float cost[GRIDSIZE][GRIDSIZE];

//position

extern float robot_x ; //
extern float robot_y;
const int goal_x = 2;
const int goal_y = 0;
extern int cellCount;

void initialiser();
void findPos(int x, int y);
int sjekkLaser(float *F, float *L, float *R);
void updateKart( float *F, float *L, float *R);
extern int cellCost(int x, int y);
void replan();
void moveRobotino(float heading);
float checkPlan( int x, int y);

#endif 







