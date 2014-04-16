#include <iostream>
#include <cmath>

#include "VFH.h"


int modulo( int x, int m)
{
	int r;
	
	if(m < 0) m = -m;
	
 	r = x % m;
	return r < 0 ? r+m: r;
}


int modularDist( int a, int b, int m)
{
	int dist_a, dist_b;

	dist_a = modulo(a-b, m);
	dist_b = modulo(b-a, m);

	return dist_a < dist_b? dist_a : dist_b;
}

grid_t * gridInit(int dime, int reso )
{

	grid_t * grid;
	grid = (grid_t *)malloc(sizeof(grid_t));
	
	if(NULL == grid) return NULL;

	grid->dime = dime % 2 == 0 ? dime + 1 : dime;
	grid->reso = reso;

	grid->cells = (int *)malloc(dime * dime * sizeof(int));
	
	if(NULL == grid->cells)return NULL;

	for(int i = 0; i < dime; ++i)
	{
		for(int j = 0; j < dime; ++j)grid->cells[i * dime + j] = 0;	
		
	}
	
	return grid;
}

int gridUpdate(grid_t * grid, int pos_x, int pos_y, rangeMeasure_t data)
{
	if(grid == NULL) return 0;
	if(grid->cells == NULL) return 0;

	int new_x, new_y;

	new_x = pos_x;
	new_y = pos_y;

	new_x += (int)floor((data.dist / grid->reso)*cos(data.direc * PI/180));
	new_y += (int)floor((data.dist / grid->reso)*sin(data.direc * PI/180));

	if(new_x < grid->dime && new_y < grid->dime) grid->cells[new_x * grid->dime + new_y] += 1;

	return 1;
}

grid_t *getMovingWindow(grid_t *grid, int current_position_x, int current_position_y, int dim )
{
	int grid_i, grid_j;
	grid_t *moving_window;
	
	moving_window = gridInit(dim, grid->reso);

	if(NULL != moving_window)
	{
		for(int i = 0; i < dim; i++)
		{
			for(int j = 0; j < dim; j ++)
			{
				grid_i = i + current_position_x + (dim - 1)/2;
				grid_j = j + current_position_y + (dim - 1)/2;

				if(grid_i < grid->dime && grid_j < grid->dime)
				{
					moving_window->cells[i * dim + j] = grid->cells[grid_i * grid->dime + grid_j]; 
				}
			}
		}
	}

	return moving_window;
}


hist_t * histInit(int alpha, double threshold, double density_a, double density_b)
{
	hist_t * hist;
	hist = (hist_t *)malloc(sizeof(hist_t));
	
	if(NULL == hist)return NULL;
	
	hist->alpha = alpha;
	hist->sectors = 360/alpha;
	hist->threshold = threshold;
	
	hist->densities = (int *)malloc(hist->sectors * sizeof(int));

	if(NULL == hist->densities) return NULL;
	
	for(int i = 0; i < hist->sectors; i++) hist->densities[i] = 0;
	
	return hist; 	
}

void histUpdate(hist_t * hist, grid_t * grid )
{
	int dim;
	double dens_a, dens_b;
	double beta, density;
	
	dim = grid->dime;
	dens_a = hist->density_a;
	dens_b = hist->density_b;

	for(int i = 0; i < dim; ++i)
	{
		for(int j = 0; j < dim; ++j)
		{
			beta = atan2((double)(j-dim/2), (double)(i - dim/2));
			
			density = pow(grid->cells[i * dim + j], 2);
			density *=dens_a - dens_b * sqrt(pow(i - dim/2, 2) + pow(j - dim/2, 2));

			hist->densities[(int)floor(beta / hist->alpha)] += density; 
		}
	}
}


int calculateDirection(hist_t * hist, int objectiveDirection)
{
	int sector, bestDirection = -1;
	int dist_best_and_obj, dist_sector_and_obj;
	
	objectDirection = (int)floor(objectiveDirection/hist->alpha);
	
	
	for(sector = 0; sector < hist->sectors; ++sector)
	{
		if(hist->densities[sector] < hist->threhold)
		{
			dist_best_and_obj = modular(bestDirection, objectiveDirection, hist->sectors);
			dist_sector_and_obj = modular_dist(sector, objectiveDirection, hist->sectors);

			if( bestDirection == -1 || dist_sector_and_obj < dist_best_and_obj) bestDirection = sector; 
		}
	}
	return (int) floor(bestDirection * hist->alpha);	
}
