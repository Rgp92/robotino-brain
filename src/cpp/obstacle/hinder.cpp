#include  "Hinder.h"



using namespace std;



int ObstacleClass::Add(float x,float y)

{

	Node* NewNode = new Node();	//lag en ny node



	NewNode->SetData(x,y);		//sett x og y i den nye noden

	NewNode->SetNext(NULL);		//next = 0 = ingen neste node..



	Node *TempNode = RootNode;	//sette tempnode til å peke til rootnode



	if ( TempNode != NULL ) {	// om tempmode != NULL gå igjennom hele lista og finn den siste noden

		while ( TempNode->Next() != NULL ) {

			TempNode = TempNode->Next();

		}

		TempNode->SetNext(NewNode);

	}

	else {						//om tempnode == null sett rootnode == den nye noden..

		RootNode = NewNode;

	}

	return 1;

}



int ObstacleClass::Del(int x,int y)
{

	Node *TempNode = RootNode;	//sette tempnode til å peke til rootnode
	//Node *prev;


	if ( TempNode == NULL )		//Om tempnode == NULL er lista tom og ingenting kan slettes, så return -1.

		return -1;



	if ( TempNode->Next() == NULL ) { //om tempnode.next == Null er det den eneste noden i lista, så slett den.

		delete TempNode;

		TempNode = NULL;

	} else  {					//om tempnode.next  ikke er NULL er det flere noder(koordinater) i lista

		Node *prev = RootNode ;

		do {					//så gå igjennom dem

			if ( TempNode->ValueX() == x  && TempNode->ValueY() == y) break; //om en node har samme x og y verdi som du vil slette, break ut av while loopen

			prev = TempNode;	//set *prev == tempnode (noden vi akuratt gikk gjennom)

			TempNode = TempNode->Next();  // sett tempnode til neste node

		} while ( TempNode != NULL ); // loop så lenge tempnode != null



		//de to linjene under burde ha litt bedre error checking



		prev->SetNext(TempNode->Next()); //sett next i den forrige noden til å peke til denne noden's .next

										 //da hopper vi over denne(tempnode) noden i lista 



		delete TempNode;				//og kan frigjøre minnet. 

	}



	return 1;

}



int ObstacleClass::Nearby(float x,float y,int range)

{

	bool SomethingFound =false; //bare et flag som sier om det er funnet et hinder eller ikke

	Node *TempNode = RootNode;	//sette tempnode til å peke til rootnode



	if ( TempNode == NULL )		//Om tempnode == NULL er lista tom og ingenting kan slettes, så return -1.

		return -1;



	if ( TempNode->Next() == NULL ) {	 //om tempnode.next == Null er det den eneste noden i lista

										 //sjekk node X og Y er innenfor omkretsen (range) til punktet (X og Y)

										 //og sett SomethingFound til true om x og y er innenfor

		if((TempNode->ValueX()>= x-range && TempNode->ValueX() <= x+range) &&

			(TempNode->ValueY()>= y-range && TempNode->ValueY() <= y+range)) {

				/*cout << "X: " << TempNode->ValueX() << " Y: " << TempNode->ValueY() << "\n";*/

				SomethingFound = true;

		}

	}

	else {		// om tempnode->next() !=NULL er det fler nodes i lista

		do {	// Gå igjennom alle nodes og sjekk om de er innenfor. samme som over

			if((TempNode->ValueX()>= x-range && TempNode->ValueX() <= x+range) &&

				(TempNode->ValueY()>= y-range && TempNode->ValueY() <= y+range)) {

					/*cout << "X: " << TempNode->ValueX() << " Y: " << TempNode->ValueY() << "\n";*/

					SomethingFound = true;

			}

			TempNode = TempNode->Next();

		}

		while ( TempNode != NULL ); //loop til tempnode == null == siste node i lista

	}

	

	 	if(SomethingFound)

	 		return 1;

	 	else

	 		return 0;

}



bool ObstacleClass::IsObstacle(float x,float y)

{

	//sjekk om ett eksakt koordinat er et hinder, og ikke sjekk koordinatene rundt.

	return Nearby(x,y,0); 

}



void  ObstacleClass::List()

{

	// går bare gjennom alle nodes og printer x og y til stdout..

	Node *TempNode = RootNode;



	if ( TempNode == NULL ) {

		cout << "EMPTY" << endl;

		return;

	}



	if ( TempNode->Next() == NULL ) {

		cout << "X: " << TempNode->ValueX() << " Y: " << TempNode->ValueY() << "\n";

	}

	else {

		do {

			cout << "X: " << TempNode->ValueX() << " Y: " << TempNode->ValueY() << "\n";

			TempNode = TempNode->Next();

		}

		while ( TempNode != NULL );

	}

}
