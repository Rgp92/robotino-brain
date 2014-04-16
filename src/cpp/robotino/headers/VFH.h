// Vector field histogram 



#ifndef VFH_H
#define VFH_H


#define PI 3.14159265335
#define DIM 11
#define CERTAINTY_GRID_RESOLUTION 0.1
#define WINDOW_DIM 5
#define ALPHA 5
#define DENSITY_A 10
#define DENSITY_B 5
#define OBSTACLE_DENSITY_THREHOLD 10
#define OBJECTIVE_DIRECTION 90





// class defination



class grid
{
	public: 
		int dim; // dimensjon
		int reso; // oppløsning 
		int *cells; // celle	
};

class rangeMeasure{
	public: 
		int direc; /*[grader] */
		unsigned long dist; //distansen [cm]
};

class hist 
{
	public: 
		int alpha;
		int sectoris;
		double threshold;
		double dampingConstant;
		double densityA;
		double densityB;
		int *densities; 
		
};

class controlSignal
{
	public:
		int direction; // [degrees] 
};

grid grid_t;
rangeMeasure rangeMeasure_t;
hist hist_t;
controlSignal controlSignal_t;



int modulo(int x, int m);

int modularDist(int a, int b, int m);

grid_t * gridInit(int dime, int reso);

int gridUpdate(grid_t * grid, int pos_x, int pos_y, rangeMeasure_t data);

grid_t * getMovingWindow( grid_t * grid, int pos_x, int pos_y, int dim);

hist_t * histInit(int alpha, double threshold, double density_a, double density_b);

void histUpdate(hist_t * hist, grid_t * grid ); 


int calculateDirection(hist_t * hist, int objectDirection); 

#endif 
