#include <iostream>



/*

Node classen er hvor koordinatene blir lagret, en node for hvert koordinat.

Node classen skal ikke brukes av deg direkte, bare gjennom Obstacleclassen



Obstacle classen er "hovedclassen" som implementerer node classen

Add setter opp en ny Node, setter x og y, og legger den til enden på "rootnode listen" 

(rootnode->next == ny node(->next== neste node) osv.)

Om rootnode ikke er allokert setter den rootnode == newnode.

x og y = koordinat å legge til listen.



Del går bare igjennom lista og sletter den første matchen den finner på x og y som du gir funksjonen

egentlig ubrukelig for deg.



Nearby går igjennom alle de lagrede coordinatene, fra start til slutt.

x og y = punktet du vil sjekke, Range = omkretsen rundt punktet.

Om (node.x  >= x-range OG node.x <=x+range) OG (node.y >= y-range OG node.y <= y+range)

altså innenfor en firkant rundt punktet er  .nearby =1, om det ikke er noen hinder er .nearby=0 

funksjonen returner 1 eller 0.



.IsObstacle er det samme som .Nearby bare at omkretsen er satt til 0 så den sjekker bare ett punkt.



.List Går bare gjennom alle hinder i listen og lister x og y i console.

*/





// Ikke bruk denne alene, Node classen skal bare brukes av/igjennom ObstacleClass

class Node {

 private:

	int X,Y;

	Node* next;



 public:

	Node() {};

	void SetData(float _x,float _y) { X=_x; Y=_y; };

	void SetNext(Node* NextNode) { next = NextNode; };

	int ValueX() { return X; };

	int ValueY() { return Y; };

	Node* Next() { return next; };

};





class ObstacleClass

{

 private:

	Node *RootNode;

 public:

	ObstacleClass () { RootNode = NULL; }

	int Add(float x,float y);

	int Del(int x,int y);

	void List();

	bool IsObstacle(float x,float y);

	int Nearby(float x,float y,int range = 2);



};
