#include "headers/gridnav.h"
#include "headers/Brain.h"
#include "headers/Axon.h"

gridnav::gridnav()
{
	this->SCHEDULED = true;

	this->NOT_SCHEDULED = false;

	
}

gridnav::~gridnav()
{
}
void 
gridnav::init()
{
	for(int i = 0; i < GRID_SIZE; i++)
	{
		for(int j = 0; j < GRID_SIZE; j++)
		{
			scheduled[i][j] = this->NOT_SCHEDULED;
			occupancy[i][j] = EMPTY;


			if(i==0 || (i == (GRID_SIZE - 1)) || (j == 0)|| (j == (GRID_SIZE - 1)))
			{
				occupancy[i][j] = FULL;
				scheduled[i][j] = this->SCHEDULED;
			}

			cost[i][j] = BIG_COST;
		}
	}

	for(int i = 0; i < (GRID_SIZE * GRID_SIZE); i++ )
	{
		open_list[i].x = 0;
		open_list[i].y = 0;
		open_list[i].key = BIG_COST;
		open_list[i].next = i + 1;
	}

	open_list[GRID_SIZE * GRID_SIZE - 1].next = EMPTY;
	free_head = 0;
	open_head = EMPTY;
	
}

int 
gridnav::getNode()
{
	int i;

	if(free_head == EMPTY)
	{
		//xerror("");
	}

	i = free_head;
	free_head = open_list[free_head].next;
	return(i);
}

int 
gridnav::popNode( int *x, int *y)
{
	int i;
	
	if(open_head == EMPTY )
		i = EMPTY;
	else
		{
			i = open_head;
			
			*x = open_list[i].x;
			*y = open_list[i].y;

			open_head = open_list[i].next;
			open_list[i].next = free_head;
			free_head = i;
		}
	
	return(i);
}

void 
gridnav::insertNode(int x, int y, float key )
{
	int i;
	int current;
	int last;

	if(scheduled[x][y] == NOT_SCHEDULED )
	{
		i = getNode();
		open_list[i].x = x;
		open_list[i].y = y;
		open_list[i].key = key;

		
		if(open_head == EMPTY)
		{
			open_list[i].next = EMPTY;
			open_head = i;
		}

		else 
		{
			last = EMPTY;
			current = open_head;
			while((open_list[current].key < key) &&(current != EMPTY))
			{
				last = current;
				current = open_list[current].next;
			}
			if(current == open_head)
			{
				open_head = i;
				open_list[i].next = current;
			}
			else
			{
				open_list[last].next = i;
				open_list[i].next = current;
			}
		}

		scheduled[x][y] = SCHEDULED;
	}
}

int 
gridnav::cellCost(int x, int y)
{
	int low_x, low_y;

	float temp_cost, low_cost = BIG_COST;

	cell_count++;

	if(occupancy[x][y] == FULL)
	{
		cost[x][y] = BIG_COST;
		return 0;
	}

	if( (goal_x == x) && (goal_y == y) )
	{
		cost[x][y] = 0;
		return 0;
	}

	low_x = x;
	low_y = y;
	low_cost = cost[x][y];

	for(int i = (x-1); i <= (x+1); i++)
	{
		for(int j = (y-1); j <=(y+1); j++)
		{
			if( (i==x) || (j == y) )
			temp_cost = cost[i][j] + 1;
			else
			temp_cost = cost[i][j] + SQRT2;
			if(temp_cost < low_cost)
			{
				low_cost = temp_cost;
				low_x = i;
				low_y = j;
			}
		}
	}

	if(cost[x][y] != low_cost)
	{
		cost[x][y] = low_cost;
		return 1;
	}
	else
		return 0;

	
}

void 
gridnav::expand(int x, int y)
{
	int change = 0;
//	float last_cost;

	for(int i = (x - 1); i <= (x +1 ); i++)
	{
		for(int j = (y -1); j <=(y+1); j++)
		{
			if( (i != x) || (j != y))
			{
				change = 0;

				if( (cost[i][j] == BIG_COST) && (occupancy[i][j] != FULL ))
					change = cellCost(i, j);
				if(change)
				{
					insertNode(i, j, cost[i][j]);
				}
			}
		}
	}
}


void gridnav::xerror(char *msg)
{
	std::cerr<<" "<<msg<<std::endl;
	exit(1);
}

void gridnav::readMap()
{
	FILE *file;

	char c;
	int k;	

	file = std::fopen("map.txt", "r");
	if(file == NULL)  std::cerr<<" cant open file";//xerror("readmap: can't open map.txt");

	for(int j = (GRID_SIZE)-2; j>0; j--)
	{
		for( int i = 1; i <= (GRID_SIZE-2); i++)
		{
			//Get a character
			k = std::fscanf(file, "%c", &c);
			
			//if an obstacle
			if(c == 'O')
			{
				occupancy[i][j] = FULL;
			}

			if( c == 'R' )
			{
				robot_x = (float) i;
				robot_y = (float) j;
			}

			if( c == 'G' )
			{
				goal_x = i;	
				goal_y = j;
				cost[i][j] = 0;
			}

			if(c == '\n' )
				break;
		}
	while( c != '\n')
	k = std::fscanf(file, "%c", &c);
	}

 std::fclose(file);
}

int gridnav::checkSensor()
{
return 0;	
}

void gridnav::updateMap()
{

}

void gridnav::replan()
{
	int x, y, result = EMPTY;

	insertNode(goal_x, goal_y, 0.0);
	
	while(open_head != EMPTY)
	{
		result = popNode(&x, &y);
		if(result != EMPTY)
			expand(x,y);
		else
		{
			break;
		}
	}
}

float gridnav::checkPlan(int x, int y)
{

	
	float 
		x_part = 0,
		y_part = 0, 
		low_x  = 0, 
		low_y  = 1, 
		low_cost, 
		heading;
	bool obstacle_warning = false; 

	for( int i = (x -1); i <= (x+1); i++)
	{
		for(int j = (y-1); j <=(y+1); j++)
		{
			if(i < x) x_part += cost[i][j];
			if(i > x) x_part -= cost[i][j];
			if(i < y) y_part += cost[i][j];
			if(i > y) y_part -= cost[i][j];

			if(cost[i][j] == BIG_COST)
			{
				obstacle_warning = true;
				break;
			}

		}
		if(obstacle_warning )break;
	}

	
	if(!obstacle_warning)
	{
		if( (y_part == 0.0) && (cost[x][y] > cost[x][y-1]))
		{
			y_part = 2*(cost[x][y-1] - cost[x][y]);
		}

		if((x_part == 0.0) &&(cost[x][y]>cost[x-1][y]))
		{
			x_part = 2*(cost[x-1][y] - cost[x][y]);
		}
	}
	else
	{
		low_cost = cost[x][y];
		x_part = 0; y_part = 1;

		for(int i = (x-1); i <=(x+1); i++)
		{
			for(int j = (y - 1); j <= (y + 1); j ++)
			{
				if(cost[i][j] < low_cost)
				{
					low_cost = cost[i][j];
					low_x = i;
					low_y = j;
				}
			}
		}
	
		
		
	x_part = low_x - x;
	y_part = low_y - y;
	}

	heading = atan2(y_part, x_part);

	return heading;


	
}

void gridnav::moveRobot(float heading)
{
	float x_part = 0, y_part = 0;

	x_part = MAX_VELOCITY * cos(heading);
	y_part = MAX_VELOCITY * sin(heading);


	robot_x +=x_part;
	robot_y +=y_part;

	

}

void gridnav::printCost()
{
	float highestCost = 0;

	for(int j = GRID_SIZE - 2; j > 0; j--)
	{
		for(int i = 1; i < GRID_SIZE - 1; i++)
		{
			if( (cost[i][j] != BIG_COST) && (cost[i][j] > highestCost))
				highestCost = cost[i][j];
		}
	}



	std::cerr<<" "<<GRID_SIZE - 2<<" "<< GRID_SIZE - 2 <<std::endl;

	for(int j = GRID_SIZE - 2; j > 0; j--)
	{
		for(int i = GRID_SIZE - 2; i > 0; i--)
		{
			if(cost[i][j] > highestCost )
			std::cerr<<" "<<highestCost/2<<std::endl;
			else
			std::cerr<<" "<<cost[i][j]/3<<std::endl;
	
		}
	}

}
