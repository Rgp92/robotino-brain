
/**
 * @file 	hinder.h
 * @brief	header file for Obstacle Class
 */

#ifndef _HINDER_H
#define _HINDER_H


#include <iostream>

/*
The NodeClass is where the Coordinate are stored, one node par coordinate.
The Nodeclass should not be used directly, only through the ObstacleClass 

Obstacleclass is the main Class, and through it the Nodeclass is implemented
Add function sets up new node, sets x and y and puts it on the end of the "RootNode list"
(RootNode->next == ny Node(->next == next Node) etc. )
if RootNode is not allocated, then set it to RootNode == newNode

Del function goes through the list and delete the first match it finds of x and y that you give function. 

Nearby goes through the saved coordinates, from start to end. 
x and y = the point you want to check, Range = perimeter around the points 
Whether(Node.x >= x -range and Node.x <= x + range) AND (Node.y >= y-range AND Node.y <= y +range)
that is the rectangle around the point is .nearby = 1, and if no obstacle is found .nearby = 0
the function returns 1 or 0

.IsObstacle is the same has .Nearby only that the perimeter is set to 0 it only check one point
.List only goes through all the obstacle in the list og write the x and y list in console 

*/


// Dont use alone, Node class must be used by/through ObstacleClass
class Node
{	
	public: 
		Node(){};
		void SetData(float _x, float _y) 
		{
			X = _x;
			Y = _y;
		};

		void SetNext(Node* NextNode)
		{
			next = NextNode;
		};

		float ValueX()
		{
			return X; 
		};

		float ValueY()
		{
			return Y;
		};

		Node* Next()
		{
			return next;
		};

	private:
		float X, Y;
		Node* next;

};

class ObstacleClass
{
	public: 
		ObstacleClass()
		{
			RootNode = NULL;
		}

		float Add(float x, float y);
		
		float Del(float x, float y);
		
		float List();

		bool IsObstacle(float x, float y);

		float Nearby(float x, float y );

		float Nearby(float x, float y, float range );

	private:
		Node *RootNode;
};

#endif 
