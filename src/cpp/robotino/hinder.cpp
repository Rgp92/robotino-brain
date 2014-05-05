#include "headers/hinder.h"

float ObstacleClass::Add(float x, float y)
{
	Node* NewNode = new Node(); // create a new Node
	
	NewNode->SetData( x , y); // Set x and y in the new Node 

	NewNode->SetNext( NULL ); // Next = 0 = No next Node 

	Node *TempNode = RootNode; // Sets TempNode to point to RootNode 

	if(TempNode != NULL ) // if TempNode != NULL go through the list and find the last Node
	{
		while( TempNode->Next() != NULL )
		{
			TempNode = TempNode->Next();
		}

		TempNode->SetNext( NewNode );
	}
	else 
	{ // if TempNode == NULL set RootNode == the new Node..
		RootNode = NewNode;
	}
	
	return 1;	
}

float ObstacleClass::Del(float x, float y)
{
	Node *TempNode = RootNode; // Sets TempNode to point to RootNode
	
	if( TempNode== NULL ) 	   // if TempNode == NULL the list is empty and nothing to delete, so return -1.
		return -1;
	
	if( TempNode->Next() == NULL) 
	{ 			   // if TempNode.Next == NULL is the only Node in the list, delete it
		delete TempNode;
		TempNode == NULL;
	}
	else
	{			   // if TempNode.Next != NULL, then there are many nodes in the list
		Node *prev;
		do
		{		   // So goes through it
			if( TempNode->ValueX() == x && TempNode->ValueY() == y ) break; // if one node have the same x and y value that you want to delete, break out of t											   he while loop
			prev = TempNode; // set *prev == TempNode(the node we just past through)
			TempNode = TempNode->Next(); // Set TempNode to next Node
		}while( TempNode != NULL ); // loop so long TempNode != NULL
	

	prev->SetNext( TempNode->Next() ); //Set Next to the previous node to point to this node .Next
					   //We can then jump over this(TempNode) node i in the list 

	delete TempNode; // and can fri memories 

	}

	return 1;
}

float ObstacleClass::Nearby( float x, float y, float range )
{
	//std::cerr<<"Searching for Obstacle near: " <<x << ","<<y<<"range: "<< range << std::endl;
	//std::cerr<<"==========================================="std::endl;
	bool SomethingFound = false; // Flag to check if obstacle is found or not
	Node *TempNode = RootNode;

	if( TempNode == NULL )	    // if TempNode == NULL then the list is empty and nothing can be deleted, return -1 
		return -1;

	if( TempNode->Next() == NULL ) //if TempNode.Next == NULL is the only Node in the list
	{ // Check if Node X and Y is inside the perimeter(range) to point ( X and Y )
	  // and set SomethingFound to true if x and y is within
		if( (TempNode->ValueX()>= x - range && TempNode->ValueX() <= x + range) && (TempNode->ValueY()>= y -range && TempNode->ValueY() <= y+range))
		{
			std::cerr <<"X: "<<TempNode->ValueX()<<" Y: " << TempNode->ValueY() << std::endl;
			SomethingFound = true;
		}
	}
	else
	{    // if TempNode->Next() != NULL, then there are many nodes in the list 
		do
		{	// go through all nodes and check if something is inside. same as before		
			if( (TempNode->ValueX()>= x - range && TempNode->ValueX() <= x + range) && (TempNode->ValueY()>= y -range && TempNode->ValueY() <= y+range))
			{
				std::cerr <<"X: "<<TempNode->ValueX()<<" Y: " << TempNode->ValueY() << std::endl;
				SomethingFound = true;
			}
			TempNode = TempNode->Next();

		}while( TempNode != NULL ); // loop until tempnode == null== last node in the list
	}
	
	if(SomethingFound)
		return 1;
	else
		return 0;

} 

bool ObstacleClass::IsObstacle(float x, float y)
{	//check if one exact coordinate is an obstacle. This only check one koordinate a time and not the one around. 
	return Nearby( x, y, 0 );
}

void ObstacleClass::List()
{	// loops through the node and print the x and y.
	Node *TempNode = RootNode;
	
	//No nodes 
	if( TempNode == NULL)
	{
		std::cerr<<"EMPTY"<<std::endl;
		return;
	}

	//One node in the list
	if( TempNode->Next() == NULL )
	{
		std::cerr"X: "<<TempNode->ValueX()<<" Y: "<<TempNode->ValueY()<<std::endl;
	}
	else
	{
	// Parse and print the list 
		do
		{
			std::cerr<< "X: "<<TempNode->ValueX()<< " Y: "<< TempNode->ValueY()<<std::endl;
			TempNode = TempNode->Next();
		}while( TempNode != NULL );
	}
}


