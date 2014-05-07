#include <iostream>
#include <math.h>
#include "Axon.h"

#define GRID_SIZE 12 // the grid will be GRID_SIZE^2
/* Programmet bruker bare celle fra 1 til (GRID_SIZE - 2)^2 for å kunne
unngå grense problemer.*/ 


#define BIG_COST 500
#define MAX_VELOCITY 1.0
#define EMPTY -1
#define FULL 1
#define SQRT2 1.4142136

typedef struct 
{
	int x;
	int y;
	float key;
	int next;
}node_t;

		

class gridnav
{ 
 public:

	gridnav();

	~gridnav();
	char occupancy[GRID_SIZE][GRID_SIZE]; // Each cell is empty or full
	char scheduled[GRID_SIZE][GRID_SIZE]; // Each cell is Sche'D or NOT_SC'D
	float cost[GRID_SIZE][GRID_SIZE]; // cost from cell to goal

	float robot_x, robot_y; // where the robot is
	int goal_x, goal_y;	//Where the goal is
	int cell_count; // keeps track of computation

	node_t open_list[GRID_SIZE * GRID_SIZE];
	
	int free_head;
	int open_head;

	void init();
	
	int getNode();

	int popNode(int *x, int *y);

	void insertNode(int x, int y, float key );
	
	int cellCost(int x, int y);

	void expand(int x, int y);

	void readMap();

	int checkSensor();

	void updateMap();

	void replan();

	float checkPlan(int x, int y);

	void moveRobot(float heading);
	
	void printCost();
	
	void xerror(char *msg);

 private:
	bool 

		SCHEDULED,

		NOT_SCHEDULED;


};

