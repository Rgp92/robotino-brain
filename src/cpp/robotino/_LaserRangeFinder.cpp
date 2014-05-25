#include "headers/_LaserRangeFinder.h"

#include "headers/Axon.h"
#include "headers/Brain.h"
#include "../geometry/Angle.h"
#include <stdlib.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include <iomanip>


_LaserRangeFinder::_LaserRangeFinder( Brain * pBrain ) :
	Axon::Axon( pBrain ),
	rec::robotino::api2::LaserRangeFinder::LaserRangeFinder()
{
	this->readingsUpdated = false;
	this->updateTime = 0;
}

void
_LaserRangeFinder::analyze()
{
	/// @todo Not yet implemented
	
	this->readingsUpdated = false;
}

void
_LaserRangeFinder::apply()
{}  /// Should be empty as LaserRangeFinder is not an actuator

bool
_LaserRangeFinder::test()
{
	/// @todo As the API does not (yet) have this kind of functionality, this
	/// requires analysing the data sent by the API, and may not be entirely
	/// possible.

	return true;
}

void
_LaserRangeFinder::SetLaserRange()
{
	const float range = 666.0;
	const int rangeSize = 240;

	latestReadings.setRanges(&range,rangeSize); 
}

rec::robotino::api2::LaserRangeFinderReadings
_LaserRangeFinder::setNewAngle()
{
	return this->latestReadings; 
}

void
_LaserRangeFinder::readingsToString()
{
	std::cerr << "Seq = " << latestReadings.seq
		<< "  Stamp = " << latestReadings.stamp
		<< "\nAngles; min = " << latestReadings.angle_min
		<< "  max = " << latestReadings.angle_max
		<< "  increment = " << latestReadings.angle_increment
		<< "\nTime increment = " << latestReadings.time_increment
		<< "  Scan time = " << latestReadings.scan_time
		<< "\nRange; min = " << latestReadings.range_min
		<< "  max = " << latestReadings.range_max
		<< std::cerr;

	const float *rangev; //Holder for rangevector
	unsigned int rangec = 0; // holder for rangecount
	
	this->latestReadings.ranges(&rangev, &rangec);
	std::cerr<<" "<<rangec<<std::endl;	
	for( unsigned int i = 0; i < rangec; i++ )
	{
		if(i % 5 == 0)
		{
			if(i%50 == 0)
			{
				std::cerr<<std::endl;	
			}
			std::cerr << std::setw(5) << std::setprecision(2) << rangev[i] <<" "; 
		}
	}
	std::cerr << std::endl;
	
}

rec::robotino::api2::LaserRangeFinderReadings
_LaserRangeFinder::getReadings()
{
	return this->latestReadings; 
}

 
float 
_LaserRangeFinder::getDistance(Angle angle)
{

	
	float maxAngle = this->latestReadings.angle_max;
	float minAngle = this->latestReadings.angle_min;
	
	float angleDiff = maxAngle - minAngle;  

	return angleDiff;
	//(coordinate target)
	// float dx = (target.x() - this ->_x )
	

}


obstacleAvoidance
_LaserRangeFinder::sensorFront ()
{
	
	obstacleAvoidance Result; 
	const float *rangev;  // rangevector
	unsigned int rangec;  // rangecounty

 	 	
	Result.temp = 0;
	Result.min  = 5.6;
	Result.average = 0;
	//Result.tempI = 0;
	this->latestReadings.ranges(&rangev, &rangec);

		
		for(unsigned int i = 215; i <= 297; i++)
		{
			Result.average = rangev[i] + Result.average;
			Result.temp = rangev[i];
			if( Result.temp < Result.min ) Result.min = Result.temp;
			if(rangev[i] <= 1.0) Result.tempI[i] = i;
		}
		Result.average = Result.average / 83.0;
		std::cerr<<"Front: "<<Result.min<<std::endl;
		return (Result);

	 
	
}
 
obstacleAvoidance
_LaserRangeFinder::sensorRight()
{
	
	obstacleAvoidance Result;
	const float *rangev;  // rangevector
	unsigned int rangec;  // rangecounty
	

 	
	Result.temp = 0;
	Result.min  = 5.6;
	Result.average = 0;
	//Result.tempI = 0;
	this->latestReadings.ranges(&rangev, &rangec);

		
		for(unsigned int i = 0; i <= 214; i++) // ~60 grader
		{
			Result.average = rangev[i] + Result.average;
			Result.temp = rangev[i];
			if( Result.temp < Result.min ) Result.min = Result.temp;
			if( rangev[i] <= 0.60) Result.tempI[i] = i;
		}
		

		Result.average = Result.average / 214.0;
		std::cerr<<"Right: "<<Result.min<<std::endl;
		return Result;


}

obstacleAvoidance
_LaserRangeFinder::sensorLeft( )
{
	
	obstacleAvoidance Result;
	const float *rangev;  // rangevector
	unsigned int rangec;  // rangecounty


 	
	Result.temp = 0;
	Result.min  = 5.6;
	Result.average = 0;
	//Result.temp
	this->latestReadings.ranges(&rangev, &rangec);

		for(unsigned int i = 298; i <= 512; i++) // ~60 grader
		{
			Result.average = rangev[i] + Result.average;
			Result.temp = rangev[i];
			if( Result.temp < Result.min ) Result.min = Result.temp;
			if( rangev[i] <= 0.60 ) Result.tempI[i] = i;
		}
		Result.average = Result.average/ 214.0;
		std::cerr<<"Left: "<<Result.min<<std::endl;
		return Result;


}
// Private functions

void
_LaserRangeFinder::scanEvent(const rec::robotino::api2::LaserRangeFinderReadings & scan )
{
	/// @todo Not yet fully implemented, see header file for intended functions
	this->latestReadings = scan;

	this->readingsUpdated = true;
	this->updateTime = this->brain()->msecsElapsed();
}

