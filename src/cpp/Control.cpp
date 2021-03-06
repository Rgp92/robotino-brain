#include "robotino/headers/Brain.h"
#include "robotino/headers/_OmniDrive.h"
#include "robotino/headers/_Odometry.h"
#include "robotino/headers/_CompactBha.h"
#include "robotino/headers/_LaserRangeFinder.h"
#include "robotino/headers/_DistanceSensors.h"
#include "obstacle/Hinder.h"
#include "geometry/All.h"

#include "kinect/KinectReader.h"

#include <stdlib.h>
#include <iostream>
#include <string>
#include <unistd.h> // Needed by usleep()
#include <math.h>	// Needed by fabs()
#include <vector>
#include <stdexcept>
#include <thread>



/// The maximum height of a coordinate from Kinect that will be considered for fetching
#define CONTROL_FETCH_HEIGHT_LIMIT	0.4

/// The distance between the tip of the gripper and Robotinos center, relative to the floor, when in the calibration position
#define CONTROL_CALIBRATE_ARM_DISPLACEMENT 0.42

/// Microseconds to wait before checking Kinect for a new coordinate
#define CONTROL_KINECT_WAIT	50000
#define BIGCOST 500
#define GRIDSIZE 12
#define SQRT2 1.4142136


/**
 * Class for controlling Brain and providing a simple user interface for user
 * interaction. Also contains some examples demonstrating a bit of the
 * Cababilites of Brain
 */

class Control
{
 public:

		
	/**
	 * Constructs the Control object
	 *
	 * @param	A Brain pointer is required to access Brain and subclasses
	 */
	Control( Brain * pBrain )
	{
		this->pBrain = pBrain;
		
	}

	/**
	 * Provides a simple command interpreter
	 */
	bool prompt()
	{
		std::string
			input = "",
			command = "";

		size_t
			separator = 0;

		sleep( 1 );
		std::cerr << std::endl;

		while ( true )
		{
			input = "";
			std::cerr << "Brain $ ";
			std::getline( std::cin, input );

			separator = input.find_first_of( " " );

			this->_stop = true;
			if ( this->tWorker.joinable() )
				this->tWorker.join();
			this->_stop = false;

			command = input.substr( 0, separator );

			if ( command == "goto" )
			{
				std::cerr << "Going to " << input.substr( ++separator ) << std::endl;
				this->goTo( input.substr( separator ) );
			}
			else if ( command == "stop" )
			{
				std::cerr << "Stopping" << std::endl;
				this->pBrain->drive()->niceStop();
			}
			else if ( command == "go" )
			{
				std::cerr << "Continuing (unless I'm already there :) )" << std::endl;
				this->pBrain->drive()->go();
			}
			else if ( command == "pointat" )
			{
				std::cerr << "Pointing at " << input.substr( ++separator ) << std::endl;
				this->pointAt( input.substr( separator ) );
			}
			else if ( command == "stoppointing" )
			{
				this->pBrain->drive()->stopPointing();
				std::cerr << "Pointing stopped" << std::endl;
			}

			else if ( command == "speed" )
			{
				float s[] = { 0.0, 0.0, 0.0 };

				for ( int i = 0; i < 3; i++ )
				{
					if ( separator == input.npos ) break;
					size_t start = ++separator;
					separator = input.find( ' ', start );
					s[i] = atof( input.substr( start, separator).c_str() );
				}

			std::cerr << "Setting speeds " << s[0] << ", " << s[1] << ", " << s[2] << std::endl;
				this->pBrain->drive()->setVelocity( s[0], s[1], s[2] );
			}

			else if ( command == "resetodometry" )
			{
				this->pBrain->drive()->fullStop();
				usleep( 200000 );
				this->pBrain->odom()->set( 0.0, 0.0, 0.0 );
				this->pBrain->drive()->setDestination( Coordinate( 0.0, 0.0 ) );
				this->pBrain->drive()->stopPointing();
				std::cerr << "Odometry set to 0, 0 ø0" << std::endl;
			}
			else if ( command == "printposition" )
			{
				std::cerr << "Current position: " << this->pBrain->odom()->getPosition() << std::endl;
			}

/*			else if ( command == "inner" )
			{
			//	[coordinate]\n"
			}
			else if ( command == "outer" )
			{
			//	[coordinate]\n"
			}
*/			else if ( command == "relaxarm" )
			{
				std::cerr << "Relaxing arm" << std::endl;
				this->pBrain->cbha()->armRelax();
			}
			else if ( command == "horisontal" )
			{
				std::cerr << "Rotate horisontal" << std::endl;
				this->pBrain->cbha()->rotateHorisontal();
			}
			else if ( command == "vertical" )
			{
				std::cerr << "Rotate vertical" << std::endl;
				this->pBrain->cbha()->rotateVertical();
			}
			else if ( command == "norotate" )
			{
				std::cerr << "Relaxing rotation" << std::endl;
				this->pBrain->cbha()->rotateRelax();
			}
			else if ( command == "grip" )
			{
				std::cerr << "Gripping" << std::endl;
				this->pBrain->cbha()->grip();
			}
			else if ( command == "release" )
			{
				std::cerr << "Releasing" << std::endl;
				this->pBrain->cbha()->release();
			}
			else if ( command == "cbhatest" )
			{
				std::cerr << "cBHA test procedure, please wait..." << std::endl;
				this->cbhaTest();
				std::cerr << "Test completed" << std::endl;
			}

			else if ( command == "calibrate" )
			{
				std::cerr << "Calibrate to Kinect coordinate system" << std::endl;
				this->calibratePositionToKinect();
			}
			else if ( command == "fetch" )
			{
				std::cerr << "Fetching" << std::endl;
				this->tWorker = std::thread( & Control::fetch, this, false );
			}
			else if ( command == "deliver" )
			{
				std::cerr << "Delivering" << std::endl;
				this->tWorker = std::thread( & Control::fetch, this, true );
			}
			else if ( command == "serialfetch" )
			{
				std::cerr << "Looping fetch and deliver" << std::endl;
				this->tWorker = std::thread( & Control::serialfetch, this );
			}
			else if ( command == "mimic" )
			{
				std::cerr << "cBHA mimicking" << std::endl;
				this->tWorker = std::thread( & Control::cbhaMimic, this, true );
			}
			else if ( command == "gotokinect" )
			{
				std::cerr << "Following Kinect position" << std::endl;
				this->tWorker = std::thread( & Control::driveToKinectPos, this );
			}
			else if ( command == "pointatkinect" )
			{
				std::cerr << "Pointing to Kinect position" << std::endl;
				this->tWorker = std::thread( & Control::turnToKinectPos, this );
			}
			else if ( command == "printlaser" )
			{
				if ( this->pBrain->hasLRF() )
				{
					this->pBrain->lrf()->readingsToString();
				}
				else
				{
					std::cerr << "LaserRangeFinder not available" << std::endl;
				}
			}

			else if ( command == "brainstop" )
			{
				this->pBrain->stop();
			}
			else if ( command == "brainstart" )
			{
				this->pBrain->start();
			}

			else if ( command == "nobrain" )
			{
				if ( this->pBrain->isRunning() )
				{
					std::cerr << "Brain loop must be stopped first!" << std::endl;
					continue;
				}
				this->aheadAndBack( 1.5 );
			}

			else if ( command == "help" )
			{
				this->printInstructions();
			}
			else if ( command == "exit" )
			{
				std::cerr << "Ending program" << std::endl;
				break;
			}
			else if ( command == "laserrange")
			{
				if ( this->pBrain->lrf() )
				{
					this->pBrain->lrf()->SetLaserRange();
				}
			}
			else if ( command == "wallfollow" )	// Følg veggen
			{
				
				this->wallfollow();
			}
			else if ( command == "naviger" )	// Navigation to a coordinate whilst avoiding obstacle 
			{
				std::cerr << "\nGoing to " << input.substr( ++separator ) << std::endl;
				this->naviger( input.substr( separator ) );

				//this->naviger( 1.0, 0.0 );
			}
			else if ( command == "printfront" )	// Print ut verdier for minsteverdi for sensor frem og til hoyre
			{
				this->sensorFront();
			}
			else if ( command == "printright" )	// Print ut verdier for minsteverdi for sensor frem og til hoyre
			{
				this->sensorRight();
			}
			else if( command == "printleft" )
			{
				this->sensorLeft();
			}
			else if( command == "printphi" )
			{
				this->printphi();
			}
			else if( command == "hindring" )
			{
				//this->calcObstaclePos();
			}

			else if( command == "obstacle")
			{
				this->obstacleP();
			}
			else if( command == "obstaclel")
			{
				this->calleft();
			}
			else if( command == "obstacler")
			{
				this->callright();
			}
			else if( command== "scan")
			{
				lookFront();
			}
			else if ( command == "printo" )	// Print ut verdier for minsteverdi for sensor frem og til hoyre
			{
				this->obstacleP();
			}

			else
			{
				std::cerr << "I am sorry, I am not familiar with the command \"" << command << "\"."
					<< "\nPlease try again. For help, use the command \"help\"." << std::endl;
			}
		}
		
		return true;
	}


 private:
	Brain *
	/// The pointer to the Brain object
		pBrain;

	bool
	/// A parameter used to stop function loops
		_stop;

	std::thread
	/// A thread for executing looping function without losing control through the promt
		tWorker;

	/**
	 * Prints usage instructions
	 */
	void printInstructions()
	{
		std::cerr
			<< "\nWhat I can do for you:\n"

			<< "\nDriving:\n"
			<< "goto [coordinate]\tI will drive myself to the given coordinate\n"
			<< "stop\tI will come to a halt, no more, no less.\n"
			<< "go\tI will continue, if I was previously stopped\n"
			<< "pointat [coordinate]\tI will turn myself to point at the given coordinate. I will hovever not do this until I am close enough to my destination.\n"
			<< "stoppointing\tI will not point any more\n"
			<< "speed [x< y< omega>>]\tSet Robotino's OmniDrive to the given speeds\n"
			<< "resetodometry\tSets all odometry values to 0. Also resets destination and stops any pointing.\n"

			<< "\nArm:\n"
//			<< "\tinner [coordinate]\n"
//			<< "\touter [coordinate]\n"
			<< "relaxarm\n"
			<< "horisontal\n"
			<< "vertical\n"
			<< "norotate\n"
			<< "grip\n"
			<< "release\n"
			<< "cbhatest\tA cbha test routine\n";

		if ( this->pBrain->kinectIsAvailable() )
		{
			std::cerr
			<< "\nWith kinect:\n"
			<< "calibrate\tWill perform a calibration routine, mapping the Kinect coordinate system to Robotino. The prompt will be disabled during calibration.\n"
			<< "fetch\tI will fetch item from your hand (requires Kinect)\n"
			<< "deliver\tThe same as fetch, only I will deliver any item I am currently holding (requires Kinect)\n"
			<< "mimic\tMy arm will mimic you arm (requires Kinect)\n"
			<< "gotokinect\n"
			<< "pointatkinect\n";
		}
		else
		{
			std::cerr
			<< "\n(Kinect unavailable, commands hidden)\n";
		}

		std::cerr
			<< "Brain controls:\n"
			<< "brainstop\tStops the brain loop\n"
			<< "brainstart\tStarts the brain loop\n"

			<< "Meta functions:\n"
			<< "help\tDisplay this help text\n"
			<< "exit\tExit the program\n"

			<< "\nA new command will cancel any running command\n"
			<< std::endl;	
	}

	/**
	 * The fetch function makes Robotino go to fetch an object from a persons
	 * hand. The persons hand must be tracked by Kinect.
	 * 
	 * The functionality demonstrated here was the initial benchmark Brain was
	 * built to solve.
	 *
	 * @param	deliver	Boolean to make Robotino deliver an object instead of
	 * fetching it.
	 */
	void fetch( bool deliver = false )
	{
		if ( ! this->checkKinect( ( deliver ) ? "Deliver" : "Fetch" ) ) return;

		AngularCoordinate initialPosition = this->pBrain->odom()->getPosition();

		this->pBrain->drive()->setStopWithin( CBHA_ARM_RELAXED_DISTANCE_FROM_CENTER );

		bool stopped = false;
		bool high = false;

		while ( ! this->_stop
				&& ( ( ! deliver && ! this->pBrain->cbha()->isHolding() )
					|| ( deliver && this->pBrain->cbha()->isHolding() ) ) )
		{
			if ( this->pBrain->kinect()->isUpdated() && this->pBrain->kinect()->dataAge() < 200 )
			{
				stopped = false;
				VolumeCoordinate vc = this->pBrain->kinect()->getCoordinate();

				if ( vc.z() < CONTROL_FETCH_HEIGHT_LIMIT ) 
				{
					high = false;
					std::cout << "Pickup at Coordinate: " << (Coordinate) vc << std::endl;
					this->pBrain->drive()->setDestination( vc );
					this->pBrain->drive()->setPointAt( vc );
					this->pBrain->drive()->go();

					// Hold arm up a bit (mostly for show :) )
					this->pBrain->cbha()->setMaxArmSpeed( 0.1 );
					this->pBrain->cbha()->innerToCoordinate( Coordinate( 0.0, 0.3 ) );
					this->pBrain->cbha()->outerToCoordinate( Coordinate( 0.0, 0.2 ) );
				}
				else
				{
					if ( ! high )
					{
						std::cout << "Stopping, above pickup level" << std::endl;
						high = true;
					}
					this->pBrain->cbha()->setMaxArmSpeed( 0.01 );
					this->pBrain->cbha()->armRelax();
					this->pBrain->drive()->niceStop();
					usleep( 50000 );
				}
			}
			else
			{
				if ( this->pBrain->kinect()->dataAge() > 200 )
				{
					if ( ! stopped )
					{
						std::cout << "Stopping, no coordinate" << std::endl;
						stopped = true;
					}

					this->pBrain->drive()->niceStop();
				}
				usleep( CONTROL_KINECT_WAIT );
			}
		}
		
		if ( this->_stop )
		{
			// Recieved stop signal before completing, do cleanup.
			this->pBrain->cbha()->setMaxArmSpeed( 0.01 );
			this->pBrain->cbha()->armRelax();
			this->pBrain->drive()->stopPointing();
			this->pBrain->drive()->niceStop();
			return;
		}

		std::cout << ( ( deliver ) ? "Delivery" : "Pickup" ) << " done! Returning..." << std::endl;

		this->pBrain->cbha()->setMaxArmSpeed( 0.01 );
		this->pBrain->cbha()->armRelax();
		this->pBrain->drive()->setPointAt( Coordinate( 1.0, 0.0 ) );
		this->pBrain->drive()->setStopWithin( 0 );
		this->pBrain->drive()->setDestination( (Coordinate) initialPosition );
		this->pBrain->drive()->go();
	}

	void serialfetch()
	{
		while ( ! this->_stop )
		{
			// Wait til Robotino has stopped (assuming it is currently running fetch/deliver)
			while ( this->pBrain->odom()->currentAbsSpeed() > 0.01 || this->pBrain->odom()->currentAbsOmega() > 0.01 )
				usleep( 100000 );

			this->fetch( this->pBrain->cbha()->isHolding() );
			sleep( 1 ); // Let Robotino get up to speed
		}
	}

	bool checkKinect( std::string requesterName )
	{
		if ( ! this->pBrain->kinectIsAvailable() )
		{
			std::cerr << requesterName  << " requires Kinect!" << std::endl;
			return false;
		}
		return true;
	}

	/**
	 * Sets the destination coordinate of the OmniDrive object, which makes
	 * Robotino drive to this coordinate.
	 *
	 * @param	input	A string parsable to a coordinate
	 */
	bool goTo( std::string input )
	{
		this->pBrain->drive()->stopPointing();
		Coordinate * destination = this->parseCoordinate( input );
		if ( destination == NULL )
		{
			std::cerr << "Unable to parse coordinate, try again" << std::endl;
			return false;
		}

		std::cerr << "Driving to coordinate " << * destination << std::endl;
		this->pBrain->drive()->setDestination( * destination );
		this->pBrain->drive()->go();
		delete destination;
		return true;
	}

	/**
	 * Sets the point at coordinate of the OmniDrive object, which makes
	 * Robotino turn to point at the coordinate.
	 *
	 * @param	input	A string parsable to a coordinate
	 */
	bool pointAt( std::string input )
	{
		Coordinate * target = this->parseCoordinate( input );
		if ( target == NULL )
		{
			std::cerr << "Unable to parse coordinate, try again" << std::endl;
			return false;
		}

		std::cerr << "Pointing to coordinate " << * target << std::endl;
		this->pBrain->drive()->setPointAt( * target );
		delete target;
		return true;
	}

	/**
	 * Helper function to parse a coordinate from a string
	 *
	 * @param	input	A string parsable to a coordinate
	 */
	Coordinate * parseCoordinate( std::string input )
	{
		size_t separator = input.find_first_of( ": " );

		if ( separator == std::string::npos ) return NULL;

		float
			x,
			y;

		try
		{
			x = std::stof( input.substr( 0, separator )  );
			y = std::stof( input.substr( separator ) );
		}
		catch ( const std::invalid_argument & ex )
		{
			std::cerr << "Could not aquire floats from \"" << input << "\"" << std::endl;
			return NULL;
		}

		return new Coordinate( x, y );
	}

	/**
	 * Make Robotino continuosly drive to the coordinate of a hand tracked
	 * by a Kinect.
	 */
	void driveToKinectPos()
	{
		if ( ! this->checkKinect( "DriveToKinectPos" ) ) return;

		while ( ! this->_stop )
		{
			if ( this->pBrain->kinect()->isUpdated() )
			{
				VolumeCoordinate vc = this->pBrain->kinect()->getCoordinate();
				this->pBrain->drive()->setDestination( vc );
				usleep ( CONTROL_KINECT_WAIT );
			}
			else
			{
				usleep( CONTROL_KINECT_WAIT );
			}
		}
	}

	/**
	 * Make Robotino continuosly turn to the coordinate of a hand tracked
	 * by a Kinect.
	 */
	void turnToKinectPos()
	{
		if ( ! this->checkKinect( "TurnToKinectPos" ) ) return;

		while ( ! this->_stop )
		{
			if ( this->pBrain->kinect()->isUpdated() )
			{
				VolumeCoordinate vc = this->pBrain->kinect()->getCoordinate();
				this->pBrain->drive()->setPointAt( vc );
				usleep ( CONTROL_KINECT_WAIT );
			}
			else
			{
				usleep( CONTROL_KINECT_WAIT );
			}
		}
	}

	/**
	 * A test routine to verify that the cBHA is operational
	 */
	void cbhaTest()
	{
		this->pBrain->cbha()->rotateHorisontal();
		sleep( 4 );
		this->pBrain->cbha()->rotateVertical();
		sleep( 4 );
		this->pBrain->cbha()->rotateRelax();
		sleep( 5 );
		this->pBrain->cbha()->innerToCoordinate( Coordinate( 0.0, 1.0 ) );
		this->pBrain->cbha()->outerToCoordinate( Coordinate( 0.0, 1.0 ) );
		sleep( 10 );
		this->pBrain->cbha()->armRelax();
	}
	
	/**
	 * Make Robotinos cBHA mimic the motions of a hand tracket by a Kinect.
	 *
	 * @param	mirror	(default) Robotino will mirror the hans motions. If you
	 * have Robotino facing away from you, set this to false.
	 */
	void cbhaMimic( bool mirror = true )
	{
		if ( ! this->checkKinect( "Mimic" ) ) return;

		VolumeCoordinate
			zero( 1.5, 0, 1.0 ); // Provide a relatively central point if Click-calibration does not work

		unsigned int
			earliestNewCalibration = 0;

		while ( ! this->_stop )
		{
			if ( this->pBrain->kinect()->isUpdated()  )
			{
				if ( this->pBrain->kinect()->clickAge() < 500
						&& this->pBrain->msecsElapsed() > earliestNewCalibration )
				{
					zero = this->pBrain->kinect()->getCoordinate();
					earliestNewCalibration = this->pBrain->msecsElapsed() + 1000;
					continue;
				}

				VolumeCoordinate vc = this->pBrain->kinect()->getCoordinate();

				float x = vc.x() - zero.x();
				float y = vc.y() - zero.y();
				float z = vc.z() - zero.z();
				
				if ( ! mirror ) y *= -1;

				this->pBrain->cbha()->outerToCoordinate( Coordinate( y * 2.0, z * 2.0 ) );
				this->pBrain->cbha()->innerToCoordinate( Coordinate( y * 1.5, z * 1.5 ) );

				if ( x > 0.1 )
					this->pBrain->cbha()->rotateHorisontal();
				else if (x < -0.1 )
					this->pBrain->cbha()->rotateVertical();
				else
					this->pBrain->cbha()->rotateRelax();

				if ( fabs( x ) > 0.2 )
					this->pBrain->cbha()->grip();
				else
					this->pBrain->cbha()->release();
			}
			else
				usleep( CONTROL_KINECT_WAIT );
		}

		this->pBrain->cbha()->rotateRelax();
		this->pBrain->cbha()->armRelax();
		this->pBrain->cbha()->release();
	}


	void calibratePositionToKinect()
	{
		if ( ! this->checkKinect( "CalibratePositionToKinect" ) ) return;

		this->pBrain->drive()->setStopWithin( 0 );

		std::cerr << "Performing odometry calibration, please wait..." << std::endl;

		this->pBrain->cbha()->grip();
		this->pBrain->drive()->niceStop();
		sleep( 4 );

		// Make sure Robotino is not driving
		while ( this->pBrain->odom()->currentAbsSpeed() > 0.01 || this->pBrain->odom()->currentAbsOmega() > 0.01 )
			usleep( 100000 );

		/// @todo Improvement: Use current coordinate system, so this is not lost if calibration is aborted (not currently applicable).
		this->pBrain->odom()->set( 0.0, 0.0, 0.0 );
	
		this->pBrain->cbha()->innerToCoordinate( 0.0, 0.4 );
		this->pBrain->cbha()->outerToCoordinate( 0.0, 0.4 );
		
		std::cerr << "Wait for arm to reach position..." << std::endl;
		while ( this->pBrain->cbha()->armTotalPressureDiff() > 0.1 )
			usleep( 100000 );

		std::cerr
			<< "Stand beside Robotino and make sure Kinect is reading your hand.\n"
			<< "Then, using your wrist, slightly push down on the tip of Robotinos gripper.\n"
			<< "Robotino will, after a slight pause, drive 1 meter forward"
			<< std::endl;

		VolumeCoordinate kinectCoordinate0 = this->pBrain->cbha()->getTouchCoordinate();

		std::cerr << "First coordinate stored : " << kinectCoordinate0 << "\nGet out of my way!" << std::endl;

		this->pBrain->cbha()->innerToCoordinate( 0.0, 0.2 );
		this->pBrain->cbha()->outerToCoordinate( 0.0, 0.2 );
		usleep ( 500000 );

		this->pBrain->drive()->setDestination( Coordinate( 1.0, 0.0 ) );
		this->pBrain->drive()->go();
		this->pBrain->drive()->setPointAt( Coordinate( 1000.0, 0.0 ) );
		sleep ( 2 );

		this->pBrain->cbha()->innerToCoordinate( 0.0, 0.4 );
		this->pBrain->cbha()->outerToCoordinate( 0.0, 0.4 );

		// Wait until Robotino is in position
		while ( this->pBrain->odom()->currentAbsSpeed() > 0.01 || this->pBrain->odom()->currentAbsOmega() > 0.01 )
			usleep( 100000 );

		this->pBrain->drive()->niceStop();

		std::cerr << "Wait for arm to reach position..." << std::endl;
		while ( this->pBrain->cbha()->armTotalPressureDiff() > 0.1 )
			usleep( 100000 );

		std::cerr
			<< "Again, using your wrist tracked by Kinect, slightly push down on the tip of Robotinos gripper."
			<< std::endl;

		VolumeCoordinate kinectCoordinate1 = this->pBrain->cbha()->getTouchCoordinate();
		std::cerr << "Second coordinate stored: " << kinectCoordinate1 << std::endl;

		// Get current odom position
		AngularCoordinate odomPos1 = this->pBrain->odom()->getPosition();

			// Calculate actual heading and position:
		// This is done using the now known travel direction of Robotino, and
		// the approximate distance between the touched arm and Robotinos
		// center.
		Vector odomDeviation = Coordinate( 0.0, 1.0 ).getVector( odomPos1 );
		Angle phi;
		Coordinate kinectCoordinate0Adjusted = kinectCoordinate0;
		float kinectAngleDiff = 99.0;

		while ( true )
		{
			Vector kinectVector = kinectCoordinate0Adjusted.getVector( kinectCoordinate1 );
			kinectAngleDiff -= fabs( kinectVector.phi() );


			kinectAngleDiff = fabs( kinectAngleDiff );

			phi = Angle( kinectVector.phi() + odomPos1.phi() );

			Coordinate convertedOdomDeviation = Vector( odomDeviation.magnitude(), phi.phi() ).cartesian();

			kinectCoordinate0Adjusted = Coordinate(
					kinectCoordinate0.x() + convertedOdomDeviation.x(),
					kinectCoordinate0.y() + convertedOdomDeviation.y() );

			if ( kinectAngleDiff > 0.01 ) break;
			kinectAngleDiff = fabs( kinectVector.phi() );
		}

		// Calculate the arm offset to apply to the second kinect position
		Coordinate armVector = Vector( CONTROL_CALIBRATE_ARM_DISPLACEMENT, phi.phi() ).cartesian();

		// Calculate and apply coordinates and vector
		float x = kinectCoordinate1.x() - armVector.x();
		float y = kinectCoordinate1.y() - armVector.y();
		if ( this->pBrain->odom()->set( x, y, phi.phi() ) )
		{
			usleep( 200000 );	// Give set a moment to take effect
			std::cerr << "Calibration completed, new position set: " << this->pBrain->odom()->getPosition() << std::endl;
		}

		this->pBrain->cbha()->armRelax();
		this->pBrain->cbha()->release();
		this->pBrain->drive()->setDestination( this->pBrain->odom()->getPosition() );
		this->pBrain->drive()->stopPointing();
	}

	void aheadAndBack( float distance )
	{
		if ( this->pBrain->isRunning() )
		{
			std::cerr << "This function cannot be used when the Brain loop is running" << std::endl;
		}
		AngularCoordinate
			position;

		this->pBrain->odom()->set( position.x(), position.y(), position.phi() );

		while ( position.x() < distance )
		{
			this->pBrain->drive()->setVelocity( 0.4, 0.0, 0.0 );
			usleep( 50000 );
			position = this->pBrain->odom()->getPosition();
		}

		this->pBrain->drive()->setVelocity( 0.0, 0.0, 0.0 );
		sleep( 2 );

		while ( position.x() > 0 )
		{
			this->pBrain->drive()->setVelocity( -0.4, 0.0, 0.0 );
			usleep( 50000 );
			position = this->pBrain->odom()->getPosition();
		}
	}

	void wallfollow()
	{	
		const float *rangev;  // rangevector
		unsigned int rangec;  // rangecount
		unsigned int i	= 0;
		float front	= 0;
		float right	= 0;
		bool ok		= true;
		bool turned	= true;
		
	
		rec::robotino::api2::LaserRangeFinderReadings  r;

		this->pBrain->odom()->set( 0.0, 0.0, 0.0 );
		this->pBrain->drive()->setVelocity( 0.0, 0.0, 0.0 );
		
		rettopp();		// Starter med å rette opp roboten for veggen den er ved.

		do
		{ 	
			
			r = this->pBrain ->lrf()->getReadings();
			r.ranges( &rangev, &rangec );
			//hinder.List();
			i = 0;

			front = avoidFront(rangev);
			right = avoidRight(rangev);

			do
			{
				//std::cerr<<"front: "<<front<<"\tRight: "<<right<<std::endl;		
				//if((i > 0 && i < 84) && (front > 0.6) )
				//if( ( right <= 0.30 && right >= 0.28 ) && ( front > 0.6 ) )
				//{
					if( ( right <= 0.30 && right >= 0.28 ) && ( front > 0.6 ) ) 	// Avstand til vegg er innenfor rekkevidde. Kjør rett frem 	
					{	
						if (turned)rettopp();
						turned = false;
						this->pBrain->drive()->setVelocity( 0.1 , 0.0 , 0.0);
					}
					if( ( right < 0.26 ) && ( front > 0.6 ) )			// Avstand til vegg er større enn 0.3. Kjør til venstre
					{				
						this->pBrain->drive()->setVelocity( 0.0 , 0.1 , 0.0 );
						turned = false;
					}
					if( ( right > 0.32 ) && ( front > 0.6 ) )			// Avstand til vegg er større enn 0.3. Kjør til høyre
					{
						this->pBrain->drive()->setVelocity( 0.0 , -0.1 , 0.0 );
						turned = false;
					}
				//}
				
					if( ( right <= 0.32 && right >= 0.27 ) && ( front < 0.6 ) )	//Avstand til hinder forran er mindre enn 60 cm
					{
						/*turned =*/ turnLeft(); //snu 90grader
						turned = true;
					}
					
				i++;
				usleep(1000);
				}while( i < 1 );	
		
			
		if (i >=1)i=0;
		usleep(10000);

		}while(ok);
	}
		
	Angle printphi()
	{
		//AngularCoordinate position;
		Angle phi = this->pBrain->odom()->getPhi();
		std::cerr << "Phi: " <<phi <<std::endl;	
		/*
		float rotat = 0;
		
		if( phi < 1.57 && rotat < 1.57 )
		{
			this->pBrain->drive()->setVelocity( 0.0 , 0.0 , 0.1 );
		 	rotat +=0.1; 	

		}*/	// Dette funker ikke	

		return phi;
	}
	void turnLeft()
	{
				//float leftTurn	= 1.57079633;
		//float leftTurn	= 1.5708;
		//float newPhi	= leftTurn;

		//this->pBrain->drive()->setVelocity( 0.0 , 0.0 , 0.2);
		//	this->pBrain->odom()->set( position.x(), position.y(), position.phi() );
		//	position = this->pBrain->odom()->getPosition();
		//float position;
		//std::cerr<<"Pos: "<<position.phi()<<std::endl;
		//while(i<y){
			this->pBrain->drive()->setVelocity( 0.1 , 0.0 , 0.7854 );
			//this->pBrain->drive()->setVelocity( 0.15 , 0.0 , 1.57 );
		//i++;
		
		//}
		//if( i >= y)i=0;	
		usleep(2000000);
		//return true;
		rettopp();
	}

	void rettopp()
	{
		const float *rangev;  // rangevector
		unsigned int rangec;  // rangecount
		unsigned int i = 0;
		bool ok = true;
	 	double d0;
		double d1;
		double x;
		float front = 0;
		float right = 0;

		rec::robotino::api2::LaserRangeFinderReadings  r;

		do
		{
			r = this->pBrain ->lrf()->getReadings();
			r.ranges( &rangev, &rangec );
			i = 0;

			front = avoidFront(rangev);
			right = avoidRight(rangev);

			do
			{
				// Sensor verdier hentet fra 
				d0 = floorf(rangev[0]*97)/100;		// Disse to er for vinkelen 
				d1 = floorf(rangev[100]*97)/100;	// roboten retter seg etter.

				// Beregnet hyp avstand
				x  = floorf((d0 / cos(0.52))*97)/100;

				if(/* (d0 > 0.02) &&*/ (d1 > x) )
				{
					//std::cerr<<"d1: "<< d1 <<"\tx: " << x <<"\tfront: "<< front <<std::endl;
					this->pBrain->drive()->setVelocity( 0.0 , 0.0 ,-0.4 );
					ok = true;
				}
				if(/* (d0 > 0.02) &&*/ (d1 < x) )
				{
					//std::cerr<<"d1: "<< d1 <<"\tx: " << x <<"\tfront: "<< front <<std::endl;
					this->pBrain->drive()->setVelocity( 0.0 , 0.0 ,0.4 );
					ok= true;
				}
				
				if(/* (d0 > 0.02) &&*/ (d1 == x) )
				{			
					this->pBrain->drive()->setVelocity( 0.0 , 0.0 ,0.0 );
					//std::cerr<<"d1: "<< d1 <<"\tx: " << x <<"\tfront: "<< front <<std::endl;
					//std::cerr<<"\n\n\n OK! Robot er på linje med veggen. \n\n\n"<<std::endl;
					ok = false;
				}
			
				i++;
				usleep(1000);
			}while(i <= 1 );

			if (i >=1)i=0;
		}while(ok);

	  	std::cerr<<"rettet opp" <<std::endl;
		usleep(10000);
	}

	float avoidFront(const float *rangev) 
	{
		float temp = 0.0;
		float min_front_distance = 5.6;
	
			
		for(unsigned int i = 215; i <=297; i++)
		{
			temp = rangev[i];
			if(temp < min_front_distance) min_front_distance = temp;
		}
		
		usleep(100000);
		return min_front_distance;
	}	
	
	float avoidRight(const float *rangev)
	{
		float temp = 0;
		float min_right_distance = 5.6;
		
		for(unsigned i = 0; i < 84; i++)
		{
			temp = rangev[i];
			if(temp < min_right_distance) min_right_distance = temp;
		}
	
		usleep(1000);
		return min_right_distance;
	}
	
	void frontS( int &minI, int &maxI)
	{
		const float *rangev;  // rangevector
		unsigned int rangec;  // rangecount
		int temparray;
		int maxTemp = 0, minTemp = 297;
		rec::robotino::api2::LaserRangeFinderReadings  r;
	
		r = this->pBrain ->lrf()->getReadings();
		r.ranges( &rangev, &rangec );
	
		
		for(unsigned i = 215; i <= 297; i++)
		{
			if(rangev[i] <=  1.0)
			{
				temparray = i;
				if(temparray < minTemp) 
				{
					minTemp = temparray;
					//minI = &minTemp;
				}
				if(temparray > maxTemp) 
				{
					maxTemp = temparray;
					//maxI = &maxTemp;
				}
			}

		}


		minI = minTemp;
		maxI = maxTemp;
		usleep(10000);	

	}

	float sensorFront()

	{
		const float *rangev;  // rangevector
		unsigned int rangec;  // rangecount
		

		rec::robotino::api2::LaserRangeFinderReadings  r;
	
		r = this->pBrain ->lrf()->getReadings();
		r.ranges( &rangev, &rangec );
		
		float temp = 0;
		float min_front_distance = 5.6;
		
		for(unsigned int i = 215; i <= 297; i++)
		{
			temp = rangev[i] + temp;
			if(rangev[i]< min_front_distance) min_front_distance = rangev[i];
		}
		
		//std::cerr<<"Front: "<<min_front_distance<<std::endl;

		return min_front_distance;

		usleep(100000);
	}

	float sensorRight()
	{
		const float *rangev;  // rangevector
		unsigned int rangec;  // rangecount
		

		rec::robotino::api2::LaserRangeFinderReadings  r;
	
		r = this->pBrain ->lrf()->getReadings();
		r.ranges( &rangev, &rangec );
		
		float temp = 0;
		float min_right_distance = 5.6;
		
		for(unsigned int i = 0; i <= 214; i++)
		{
			temp = rangev[i] + temp;
			if(rangev[i]< min_right_distance) min_right_distance = rangev[i];
		}
		
		//std::cerr<<"Right: "<<min_right_distance<<std::endl;

		return min_right_distance;

		usleep(100000);
	}
	
	float sensorLeft()
	{
		const float *rangev;  // rangevector
		unsigned int rangec;  // rangecount
		rec::robotino::api2::LaserRangeFinderReadings  r;
	
		r = this->pBrain ->lrf()->getReadings();
		r.ranges( &rangev, &rangec );

		float temp = 0;
		float min_left_distance = 5.6;
		//int point;

		for(unsigned i = 298; i < 513; i++)
		{
			temp = rangev[i] + temp;
			if(rangev[i] < min_left_distance) min_left_distance = rangev[i];
		
		}

		//std::cerr<<"Left: "<<min_left_distance<<std::endl;
		return min_left_distance;

		usleep(100000);
	}

	float sensorRundt()
	{
		const float *rangev;  // rangevector
		unsigned int rangec;  // rangecount
		rec::robotino::api2::LaserRangeFinderReadings  r;
	
		r = this->pBrain ->lrf()->getReadings();
		r.ranges( &rangev, &rangec );

		float temp = 0;
		float min_rundt_distance = 5.6;
		//int point;

		for(unsigned i = 0; i < 513; i++)
		{
			temp = rangev[i] + temp;
			if(rangev[i] < min_rundt_distance) min_rundt_distance = rangev[i];
		
		}

		//std::cerr<<"Left: "<<min_left_distance<<std::endl;
		return min_rundt_distance;

		usleep(100000);
	}

	/*void calcObstaclePos()
	{
		const float *rangev;  // rangevector
		unsigned int rangec;  // rangecount
		
		Coordinate objectPos,robotPos;
		//AngularCoordinate robotPos,robotPos;

		robotPos = this->pBrain->odom()->getPosition();
		rec::robotino::api2::LaserRangeFinderReadings  r;

		float robotX		= robotPos.x(); 
		float robotY		= robotPos.y();
		float laserAngle	= 0;
		float objectLength	= 0;
		float objectX = 0.0, objectY = 0.0, objectHyp = 0.0, objectXmin = 0.0, objectYmin = 0.0, minHyp= 0.0, min_distance = 5.6, temp = 0.0;
		 
		int hindringer		= 0;
		int  y=1, z=0,v=0,h=0;

		int retning = 0;
		
		robotPos	= Coordinate ( robotX , robotY );

		minHyp = sqrt( (0.3*0.3) + (1) );

		for(unsigned int i = 0; i <= 256; i++)
		{
			temp = rangev[i] + temp;

			if(rangev[i]< min_R_distance)
			{
				min_R_distance	= rangev[i];
				min_i		= [i];
			}
		}

		if(min_R_distance < minHyp)
		{
			laserAngle	= ( (180.0/512.0) * min_i );	// Beregner vinkel til objekt

			objectX		= ( (objectLength * 1.0) * cos( laserAngle ) );
			objectY		= ( (objectLength * 1.0) * sin( laserAngle ) );

			objectXmin = sqrt( (objectLength*objectLength) + (objectY*objectY) );
			objectYmin = sqrt( (objectLength*objectLength) - (objectX*objectX) );
			objectHyp = sqrt( (0.3*0.3) + (1*1) );

			objectPos	= Coordinate ( objectXmin , objectYmin );

			if(objectHyp < 1.044) h = rangev[min_i];
				
		}
								
			std::cerr << "object Hx: " << -objectXmin << "\tobject Hy: " << objectY << "\tMin avstand til objekt: " << min_distance << std::endl;

			



		for(unsigned int i = 257; i < 513; i++)
		{
			z = ( i  * 1 - ( 2 * y ) );

			temp = rangev[i] + temp;

			if(rangev[i]< min_R_distance)
			{
				min_R_distance	= rangev[i];
				min_i		= [i];
			}
		}

		if(min_V_distance < minHyp)
		{
			laserAngle	= ( (180.0/512.0) * z );	// Beregner vinkel til objekt

			objectX		= ( (objectLength * 1.0) * cos( laserAngle ) );
			objectY		= ( (objectLength * 1.0) * sin( laserAngle ) );

			objectXmin = sqrt( (objectLength*objectLength) + (objectY*objectY) );
			objectYmin = sqrt( (objectLength*objectLength) - (objectX*objectX) );
			objectHyp = sqrt( (0.3*0.3) + (1*1) );

			objectPos	= Coordinate ( objectXmin , objectYmin );

			if(objectHyp < 1.044) h = rangev[min_i];
				
		}

			if ( i > 256 && i < 513 )	// Venstre 
			{
				// Må gjøre om slik at vi får 0 ved 512 og 256 ved 256
				z = ( i  * 1 - ( 2 * y ) );
				laserAngle	= ( (180.0/512.0) * z );	// Beregner vinkel til objekt

				if(rangev[i] < min_distance)
				{
				min_distance = rangev[i];
				objectX		= ( (objectLength * 1.0) * cos( laserAngle ) );
				objectY		= ( (objectLength * 1.0) * sin( laserAngle ) );

				objectXmin = sqrt( (objectLength*objectLength) + (objectY*objectY) );
				objectYmin = sqrt( (objectLength*objectLength) - (objectX*objectX) );
				objectHyp = sqrt( (0.3*0.3) + (1*1) );

				objectPos	= Coordinate ( -objectXmin , objectYmin );

				if(objectHyp < 1.044) v = rangev[i];
				}

				std::cerr << "object Vx: " << -objectXmin << "\tobject Vy: " << objectY << "\tMin avstand til objekt: " << min_distance << std::endl;
				
				y++;
			}

			if ( objectLength < minHyp )
			{

			objectX		= ( (objectLength * 1.0) * cos(laserAngle * 3.141592 / 180.0) );
			objectY		= ( (objectLength * 1.0) * sin(laserAngle * 3.141592 / 180.0) ); 
			objectPos	= Coordinate ( objectX + robotY , objectY + robotX );
			//objectPos	= objectPos + robotPos; 


				//if( objectX < 0.3 ) std::cerr << "Hindring i veien for X. X-verdi:" << objectX << std::endl;

				//std::cerr << "Robotens pos: " << robotPos  << ". Objektets pos: " << objectPos << std::endl;
				//std::cerr << "Sensor leser av " << objectLength << " cm til objektet\t " << laserAngle * 1 << " grader for Robotino.\n" << std::endl;

				hindringer++;
			}

			usleep(1000);
		

		if ( v < h ) retning = 1;
		if ( h < v ) retning = 2;

		std::cerr << "\nV:" << v << " H:" << h << "\n" << std::endl;

		std::cerr << "\nRobot posisjon:" << robotPos << "\n" << std::endl;

		if ( hindringer == 0 ) std::cerr << "Ingen hindringer innenfor 100 cm avstand." << std::endl;

		if( retning == 2) std::cerr << "VIII MÅÅ GÅÅ TILL HØØYYRREE" << std::endl; callright();
		else if( retning == 1)  std::cerr << "VIII MÅÅ GÅÅ TILL VEENNSSTTRREE" << std::endl; *calleft();
		else std::cerr << "rett frem" << std::endl; // calleft();
		

		//std::cerr << "Sensor leser av " << objectLength * 100 << " cm til objekt ved 0 grader forran robot." << std::endl;
		//std::cerr << "Robotens koordinat: " << robotX << "," << robotY << " " << this->pBrain->odom()->getPhi() << ". Objektets koordinat: " << objectPos << std::endl;
	}
*/


	void obstacleP()	// se etter hindringer mindre enn 1m
	{
		const float *rangev;  // rangevector
		unsigned int rangec;  // rangecount

		rec::robotino::api2::LaserRangeFinderReadings  r;

		r = this->pBrain ->lrf()->getReadings();
		r.ranges( &rangev, &rangec );
		//int a, b;

		//float obsarray[514];

		float left		= 0;
		float right		= 0;
		float front 		= 0;
		//int pointMin;


		//do{
			front	= sensorFront();	//les av front sensorSensor leser av 
			right	= sensorRight();	// les av høyre side
			left	= sensorLeft();		// les av venstre side

			//std::cerr << "Er det hindring forran Robotino? "<<min_distance << std::endl;
			//this->pBrain->drive()->setVelocity( 0.1 , 0.0 , 0.0 );

			if( front < 1.0 ) // Les av høyre og venstre side. Den med mest verdi gir robotino beskjed om hvor den skal
			{
				std::cerr << "Noe er i veien forran" << std::endl;

				// Velger å rotere roboten til den siden med hindring som er ikke nærmest
				if( right < left )
				{
					std::cerr << "Go left\n" << std::endl;// roter til venstre

				//	if( (front <= 1.0 && right <=1.0) ) this->pBrain->drive()->setVelocity( 0.1 , 0.1 , 0.0 );
				//	if( (front >= 1.0 && right <= 0.6) ) this->pBrain->drive()->setVelocity( 0.1 ,  0.0, 0.0 );

				}
				else if( right > left )
				{
					std::cerr << "Go right\n" << std::endl;// roter til høyre
					//this->pBrain->drive()->setVelocity( 0.0 , 0.1 , 0.0 );
				}
				else
				{
					std::cerr << "Ingen hindring." <<std::endl;
					//this->pBrain->drive()->setVelocity(0.1, 0.0, 0.0);
				}

				
			}
		
			usleep(1000);
	//	}while(true);
	}

	bool turnPhi( float tX, float tY, float phi )	// setter true / false
	{	//	1.570796327
		bool nord	= false;
		bool sor	= false;
		bool snu	= false;

		if (( phi > 0 && phi < 1.57 ) || ( phi < 0 && phi > -1.57 )) nord = true;
		if (( phi > 1.57 && phi < 3.14 ) || ( phi < -1.57 && phi > -3.14 )) sor = true;

		AngularCoordinate Pos;
		Pos		= this->pBrain->odom()->getPosition();		
		
		float dx	= ( tX - Pos.x() );
		float dy	= ( tY - Pos.y() );
		float dir	= atan2( dy,dx );
		
		if ( nord == true )
		{
		sor = false;
		std::cerr << "Peker nord" << std::endl;
		if (( dir > 1.57 && dir < 3.14 ) || ( dir < -1.57 && dir > -3.14 )) snu = true;
		}

		if ( sor == true )
		{
		nord = false;
		std::cerr << "Peker sør" << std::endl;
		if (( dir < 1.57 && dir > 0 ) || ( dir < 0 && dir > -1.57 )) snu = true;
		}

		std::cerr << "Phi = " << phi /*<< "\nRobot point: " << thisPhi*/ << std::endl;
		
		usleep(10000);

		return snu;
	}

	bool naviger( std::string input )
	{
		this->pBrain->drive()->stopPointing();

		size_t seperator = input.find_first_of( ": ");
		
		if( seperator == std::string::npos ) return NULL;
		
		float goalX;
		float goalY;

		try
		{
			goalX = std::stof( input.substr( 0, seperator ) );
			goalY = std::stof( input.substr( seperator ) );
		}
		catch ( const std::invalid_argument &ex )
		{
			std::cerr << "kunne ikke fra\""<<input<<"\""<<std::endl;
			return NULL;
		}

		std::cerr << "x: " << goalX <<" y: " << goalY << std::endl;

		Coordinate * destination = this->parseCoordinate( input );		// setter destinasjonen
		
		if( destination == NULL )
		{
			std::cerr << "unable to parse coordinate" << std::endl;
			return false;
		}
		else
		{
		
		std::cerr << "Går til koordinat: " << * destination << std::endl;

		bool okx	= false;
		bool oky	= false;
		bool fortsett	= true;
		bool snu	= false;

		int i = 0;

		//const float *rangev;  // rangevector
		//unsigned int rangec;  // rangecounty
		//float sensorN5 = 0;
		//float sensorN4 = 0;
		//int minI, maxI;
		//float angleradian = 0, _x = 0, _y = 0;

		AngularCoordinate odomPos1, odomPos2, Pos;
		//ObstacleClass Hinder;
		//Coordinate * destination;

		//rec::robotino::api2::LaserRangeFinderReadings  r;
		//obstacleAvoidance right, left, front2;
		float front, right, left;
	//	usleep( 200000 );

		//r = this->pBrain ->lrf()->getReadings();
		//r.ranges( &rangev, &rangec );
		//Hinder.List();
		//Pos = this->pBrain->odom()->getPosition();

		snu	= turnPhi(goalX, goalY, Pos.phi());

		if (snu == true) std::cerr << "snur" << std::endl;
		if (snu == false) std::cerr << "ikke snu" << std::endl;

		this->pBrain->drive()->setDestination(* destination);
		this->pBrain->drive()->go();


		//this->pBrain->drive()->setVelocity(0.1, 0.0, 0.0);
		//this->pBrain->drive()->setVelocity( 0.0 , 0.0, 3.14 );	// Hinder for nærme robot forran.

 	do
	{
	//	r = this->pBrain ->lrf()->getReadings();
	//	r.ranges( &rangev, &rangec );

		front = sensorFront();
		right = sensorRight();
		left  = sensorLeft();

		odomPos1 = this->pBrain->odom()->getPosition();
		Pos = this->pBrain->odom()->getPosition();

		//sensorN5 = this->pBrain->dist()->sensorDistance(5);
		//sensorN4 = this->pBrain->dist()->sensorDistance(4);
		//std::cerr<<" "<< sensorN4<<std::endl;
		//Hinder.List();
		
		//snu	= turnPhi(goalX, goalY, Pos.phi());

		if (snu == true) std::cerr << "snur" << std::endl;
		if (snu == false) std::cerr << "ikke snu" << std::endl;
		
	//	if (snu) std::cerr << "kan snu" << std::endl;
	//	if (!snu) std::cerr << "kan ikke snu" << std::endl;



		do
		{
			//calcObstaclePos();


			
			if( front <= 0.7 && snu == true )
			{
				if( front < 0.7 )
				{
					std::cerr << "Snur, men for nær forran" << std::endl;
 					this->pBrain->drive()->setVelocity( -0.1 , 0.0, 0.0 );	// Hinder for nærme robot forran.
				}
				else if( left < 0.3 )
				{
					std::cerr << "Snur, men for nær forran" << std::endl;
					this->pBrain->drive()->setVelocity( 0.0 , -0.1 , 0.0 );	// Hinder for nærme robot venstre side.
				} 
				else if( right < 0.3 )
				{
					std::cerr << "Snur, men for nær forran" << std::endl;
					this->pBrain->drive()->setVelocity( 0.0 , 0.1 , 0.0 );	// Hinder for nærme robot høyre side.i
				}
				else
				{
					std::cerr << snu << std::endl;
					this->pBrain->drive()->setDestination(* destination);
					this->pBrain->drive()->go();

					snu	= turnPhi(goalX, goalY, Pos.phi());
					std::cerr << Pos.phi() << std::endl;
				}
			}
			

			if( front <= 1.0 && snu == false)
			{
			std::cerr << snu << std::endl;
			//std::cerr << turnPhi(goalX, goalY) << std::endl;
			this->pBrain->drive()->setVelocity(0.1, 0.0, 0.0);



				if( left < right )	// Gå til høyre
				{
					std::cerr << "Go right\n" << std::endl;
					//calleft();
					//newAngle = callright();

					// hvis nådd pos orginal dest. 
					
					//this->pBrain->drive()->setDestination(newAngle);
					// if(nådd pos) this->pBrain->drive()->setDestination(* destination);
					//this->pBrain->drive()->go();

					//vi skal til høyre, beregn 
					
					if ( front < 0.7  ) this->pBrain->drive()->setVelocity( 0.0 , -0.1 , 0.0 );	// til høyre 
					else if ( front < 0.9 && left > 0.4 )
					{
						//std::cerr << "Go right\n" << std::endl;			// roter til right
						
						this->pBrain->drive()->setVelocity( 0.1 , -0.1 , 0.0 );
						//odomPos1 = this->pBrain->odom()->getPosition();
					}/*
					else
					{
						this->pBrain->drive()->setDestination(* destination);
						this->pBrain->drive()->go();
					}*/
					
				}
				if( right < left )	// gå til venstre
				{
					std::cerr << "Go left\n" << std::endl;
					//callright();

					//vi skal venstre, beregn høyre og legg til 0.30m
					//newPath = calleft();
 					//this->pBrain->drive()->setDestination(newPath);
					//this->pBrain->drive()->go();

					// calleft();

					
					if ( front < 0.7  ) this->pBrain->drive()->setVelocity( 0.0 , 0.1 , 0.0 );	// Hinder for nærme robot
					else if( front < 0.9 && right > 0.4)
					{
						std::cerr << "Go left\n" << std::endl;			// roter til left
						this->pBrain->drive()->setVelocity( 0.1 , 0.1 , 0.0 );
						//odomPos1 = this->pBrain->odom()->getPosition();
					}/*
					else
					{
						this->pBrain->drive()->setDestination(* destination);
						this->pBrain->drive()->go();
					}*/
				}
				
				//if( ( (front < 0.6) && (left < 0.3) ) || ( (front < 0.6) && (right < 0.3) ) ) 
				usleep(10000);	
			}

			if( front > 1.0 )  
			{
			//this->pBrain->drive()->setDestination(* destination);
			//this->pBrain->drive()->go();

				//std::cerr<<"printfront: "<<front;
				//std::cerr << turnPhi(goalX, goalY, odomPos1.phi()) << std::endl;
				//this->pBrain->drive()->setVelocity(0.1, 0.0, 0.0);

				if( left < 0.3 )
				{
					if ( left < 0.30 ) this->pBrain->drive()->setVelocity( 0.0, -0.1, 0.0 );// hindring er nærmere og ta en kontrollert unnaman
					else
					{
						this->pBrain->drive()->setDestination(* destination);
						this->pBrain->drive()->go();
						this->pBrain->drive()->setVelocity(0.1, -0.1, 0.0);
					}
				}
				if( right < 0.3 )
				{
					if ( right < 0.30 ) this->pBrain->drive()->setVelocity( 0.0, 0.1, 0.0 );
					else
					{
						this->pBrain->drive()->setDestination(* destination);
						this->pBrain->drive()->go();
						this->pBrain->drive()->setVelocity(0.1, 0.1, 0.0 );
					}
				}
				if( left >= 0.3 && right >= 0.3 )
				{
				//	this->pBrain->drive()->setVelocity(0.1, 0.0, 0.0);
					//odomPos1 = tfloat goalX, float goalY his->pBrain->odom()->getPosition();

					//this->pBrain->odom()->set( odomPos1.x(), odomPos1.y(), odomPos1.phi() );
					//this->pBrain->this->pBrain->odom()->getPosition();drive()->stopPointing();
					//std::cerr << "Driving to coordinate " << *destination<< std::endl;
					
					//this->pBrain->drive()->setDestination(Coordinate(goalX, goalY));
					this->pBrain->drive()->setDestination(* destination);
					this->pBrain->drive()->go();
			
					//return true;
						
				}

					//this->pBrain->drive()->setDestination(* destination);
					//this->pBrain->drive()->go();

				usleep(10000);
			}

			//else  this->pBrain->drive()->setVelocity( 0.1 , 0.0 , 0.0 );
		i++;

		usleep(1000);
		}while((i < 1));

		if(i >= 1) i = 0;
		
		usleep(10000);

	float Xverdi	= floor(odomPos1.x()*10+0.2)/10;
	float Yverdi	= floor(odomPos1.y()*10+0.2)/10;
	float MyXgoal	= floor(goalX*10+0.2)/10;
	float MyYgoal	= floor(goalY*10+0.2)/10;

	std::cerr << "X bedt om: " << MyXgoal << " X pos: " << Xverdi << std::endl;
	std::cerr << "Y bedt om: " << MyYgoal << " Y pos: " << Yverdi << std::endl;

	
	if ( Xverdi == MyXgoal ) okx = true;
	if ( Yverdi == MyYgoal ) oky = true;

	if ( okx && oky ) fortsett = false;

	//std::cerr << "X verdi: " << Xverdi << "\tY verdi: " <<Yverdi << std::endl;
	
	}while( fortsett );	// Leser om posisjonen er nådd
	
	delete destination;	// Slett navigasjon 
	
	//Hinder.List();
	this->pBrain->drive()->setVelocity(0.0, 0.0, 0.0);
		
	//this->pBrain->drive()->niceStop();

	}

	std::cerr << "Pos: " << this->pBrain->odom()->getPosition() << std::endl;
	usleep( 20000 );
		
	return true;
	}

	void wallfollow(float X, float Y)
	{	
		const float *rangev;  // rangevector
		unsigned int rangec;  // rangecount

		unsigned int i	= 0;
		float front	= 0;
		float right	= 0;
		bool ok		= true;
		//Angula
		this->pBrain->odom()->getPosition();	
	
		rec::robotino::api2::LaserRangeFinderReadings  r;

		this->pBrain->odom()->set( 0.0, 0.0, 0.0 );
		this->pBrain->drive()->setVelocity( 0.0, 0.0, 0.0 );
		
		rettopp();		// Starter med å rette opp roboten for veggen den er ved.

		do
		{ 	
			
			r = this->pBrain ->lrf()->getReadings();
			r.ranges( &rangev, &rangec );
			//hinder.List();
			i = 0;

			front = avoidFront(rangev);
			right = avoidRight(rangev);

			do
			{
				//std::cerr<<"front: "<<front<<"\tRight: "<<right<<std::endl;		
				//if((i > 0 && i < 84) && (front > 0.6) )
				//if( ( right <= 0.30 && right >= 0.28 ) && ( front > 0.6 ) )
				//{
					if( ( right <= 0.30 && right >= 0.28 ) && ( front > 0.6 ) ) 	// Avstand til vegg er innenfor rekkevidde. Kjør rett frem 	
					{	
						this->pBrain->drive()->setVelocity( 0.1 , 0.0 , 0.0);
					}
					if( ( right < 0.26 ) && ( front > 0.6 ) )			// Avstand til vegg er større enn 0.3. Kjør til venstre
					{				
						this->pBrain->drive()->setVelocity( 0.0 , 0.1 , 0.0 );
					}
					if( ( right > 0.32 ) && ( front > 0.6 ) )			// Avstand til vegg er større enn 0.3. Kjør til høyre
					{
						this->pBrain->drive()->setVelocity( 0.0 , -0.1 , 0.0 );
					}
				//}
				
									
				i++;
				usleep(1000);
				}while( i < 1 );	
		
			
		if (i >=1)i=0;
		usleep(10000);

		}while(ok);

		//return Coordinate(Y,X);
	}

	void callright()
	{
		const float *rangev;  // rangevector
		unsigned int rangec;  // rangecount
		int temp = 0;
		AngularCoordinate odomPos1;	
		float newAngle, X, Y, hyp = 0;
		rec::robotino::api2::LaserRangeFinderReadings  r;
	
		r = this->pBrain ->lrf()->getReadings();
		r.ranges( &rangev, &rangec );
		for(int i = 256; i < 386; i++)
		{
			if(rangev[i] < 1.41 )
			{	
				temp = i;
				hyp = rangev[i];
			}
		}
		//newPhi = printphi();
		odomPos1 = this->pBrain->odom()->getPosition();
 		newAngle = (512-temp) * (180.0/512.0);
		
		X = hyp*cos((180.0 - 90.0 - newAngle) * 3.1415/180.0);
		Y = hyp*sin((180.0 - 90.0 - newAngle) * 3.1415/180.0);
		Y = Y + 0.40;
		
		std::cerr << "Left - temp: " << temp << " angle: " << newAngle << " hyp: " << hyp << "\t(" << -X << ", " << Y << ")" << std::endl;
		//this->pBrain->drive()->setDestination(Coordinate(X,Y));
		//this->pBrain->drive()->go();

		bool okx = false, oky = false, fortsett = true;

		do
		{
		odomPos1 = this->pBrain->odom()->getPosition();
		std::cerr << "\n" << std::endl;

		float Xverdi	= floor(odomPos1.x()*10+0.5)/10;
		std::cerr << "X-verdi: " << Xverdi << std::endl;

		float MyXgoal	= floor(X*10+0.5)/10;
		std::cerr << "MyXgoal: " << MyXgoal << std::endl;

		float Yverdi	= floor(odomPos1.y()*10+0.5)/10;
		std::cerr << "Y-verdi: " << Yverdi << std::endl;

		float MyYgoal	= floor(Y*10+0.5)/10;
		std::cerr << "MyYgoal: " << MyYgoal << std::endl;

		if ( Xverdi == MyXgoal /* ( Xverdi || Xverdi + 0.1 || Xverdi - 0.1 ) */) okx = true;
		if ( Yverdi == MyYgoal /*( Yverdi || Yverdi + 0.1 || Yverdi - 0.1 )*/ ) oky = true;
		if ( okx && oky ) fortsett = false;

		usleep(10000);

		}while( fortsett );	// Leser om posisjonen er nådd
		//} while ( ((floor(X*1)/10) != (floor(odomPos1.x()*1)/10))  && ( (floor(Y*1)/10) != (floor(odomPos1.y()*1)/10 )) );
		std::cerr << this->pBrain->odom()->getPosition() << std::endl;	

		//return;


		return;
	}

	void calleft()
	{
		const float *rangev;  // rangevector
		unsigned int rangec;  // rangecount

		int temp = 255;

		AngularCoordinate odomPos1;
	
		float newAngle, X, Y, hyp = 0;
		//float tempX, tempY;

		rec::robotino::api2::LaserRangeFinderReadings  r;
	
		r = this->pBrain ->lrf()->getReadings();
		r.ranges( &rangev, &rangec );

		for(int i = 129; i < 256; i++)
		{
			if(rangev[i] < 1.41 )
			{	
				if(i < temp)
				   temp = i;
				
				hyp = rangev[i];
			}

		usleep(100);
		}

		//newPhi = printphi();
		odomPos1 = this->pBrain->odom()->getPosition();
 		newAngle = (temp) * (180.0/512.0);
		
		X = hyp*cos((180.0 - 90.0 - newAngle) * 3.1415/180.0);
		Y = hyp*sin((180.0 - 90.0 - newAngle) * 3.1415/180.0);
		Y = Y-0.40;
		
		std::cerr << "temp: " << temp << " angle: " << newAngle << " hyp: " << hyp << "\t(" << X << ", " << Y << ")" << std::endl;

		std::cerr << Coordinate(X,Y) << std::endl;
		std::cerr << "\n" << std::endl;

		//this->pBrain->drive()->setDestination(Coordinate(X,Y));
		//this->pBrain->drive()->go();

		bool okx = false, oky = false, fortsett = true;

		do
		{
		odomPos1 = this->pBrain->odom()->getPosition();
		std::cerr << "\n" << std::endl;


		float Xverdi	= floor(odomPos1.x()*10+0.5)/10;
		std::cerr << "X-verdi: " << Xverdi << std::endl;

		float MyXgoal	= floor(X*10+0.5)/10;
		std::cerr << "MyXgoal: " << MyXgoal << std::endl;

		float Yverdi	= floor(odomPos1.y()*10+0.5)/10;
		std::cerr << "Y-verdi: " << Yverdi << std::endl;

		float MyYgoal	= floor(Y*10+0.5)/10;
		std::cerr << "MyYgoal: " << MyYgoal << std::endl;

		if ( Xverdi == MyXgoal /* ( Xverdi || Xverdi + 0.1 || Xverdi - 0.1 ) */) okx = true;
		if ( Yverdi == MyYgoal /*( Yverdi || Yverdi + 0.1 || Yverdi - 0.1 )*/ ) oky = true;
		if ( okx && oky ) fortsett = false;

		usleep(10000);

		}while( fortsett );	// Leser om posisjonen er nådd
		//} while ( ((floor(X*1)/10) != (floor(odomPos1.x()*1)/10))  && ( (floor(Y*1)/10) != (floor(odomPos1.y()*1)/10 )) );
		std::cerr << this->pBrain->odom()->getPosition() << std::endl;	

		//return;
	}

	float leftObstacle()
	{
		const float *rangev;  // rangevector
		unsigned int rangec;  // rangecount
		rec::robotino::api2::LaserRangeFinderReadings  r;
	
		r = this->pBrain ->lrf()->getReadings();
		r.ranges( &rangev, &rangec );

		float minLeftDistance = 5.6;
		//int point;

		for(unsigned i = 320; i < 416; i++)
		{	
			if(rangev[i] < minLeftDistance) minLeftDistance = rangev[i];
		
		}

		//std::cerr<<"Left: "<<min_left_distance<<std::endl;
		return minLeftDistance;

		usleep(10000);

	}

	float rightObstacle()
	{
		const float *rangev;  // rangevector
		unsigned int rangec;  // rangecount
		rec::robotino::api2::LaserRangeFinderReadings  r;
	
		r = this->pBrain ->lrf()->getReadings();
		r.ranges( &rangev, &rangec );

		float minRightDistance = 5.6;
		//int point;

		for(unsigned i = 97; i < 193; i++)
		{	
			if(rangev[i] < minRightDistance) minRightDistance = rangev[i];
		
		}

		//std::cerr<<"Left: "<<min_left_distance<<std::endl;
		return minRightDistance;

		usleep(10000);
	}




/*	void wallfollow()
	{	
		const float *rangev;  // rangevector
		unsigned int rangec;  // rangecount
		unsigned int i	= 0;
		float front	= 0;
		float right	= 0;
		bool ok		= true;
		bool turned	= true;
		
	
		rec::robotino::api2::LaserRangeFinderReadings  r;

		this->pBrain->odom()->set( 0.0, 0.0, 0.0 );
		this->pBrain->drive()->setVelocity( 0.0, 0.0, 0.0 );
		
		rettopp();		// Starter med å rette opp roboten for veggen den er ved.

		do
		{ 	
			
			r = this->pBrain ->lrf()->getReadings();
			r.ranges( &rangev, &rangec );
			//hinder.List();
			i = 0;

			front = avoidFront(rangev);
			right = avoidRight(rangev);

			do
			{
				//std::cerr<<"front: "<<front<<"\tRight: "<<right<<std::endl;		
				//if((i > 0 && i < 84) && (front > 0.6) )
				//if( ( right <= 0.30 && right >= 0.28 ) && ( front > 0.6 ) )
				//{
					if( ( right <= 0.30 && right >= 0.28 ) && ( front > 0.6 ) ) 	// Avstand til vegg er innenfor rekkevidde. Kjør rett frem 	
					{	
						if (turned)rettopp();
						turned = false;
						this->pBrain->drive()->setVelocity( 0.1 , 0.0 , 0.0);
					}
					if( ( right < 0.26 ) && ( front > 0.6 ) )			// Avstand til vegg er større enn 0.3. Kjør til venstre
					{				
						this->pBrain->drive()->setVelocity( 0.0 , 0.1 , 0.0 );
						turned = false;
					}
					if( ( right > 0.32 ) && ( front > 0.6 ) )			// Avstand til vegg er større enn 0.3. Kjør til høyre
					{
						this->pBrain->drive()->setVelocity( 0.0 , -0.1 , 0.0 );
						turned = false;
					}
				//}
				
					if( ( right <= 0.32 && right >= 0.27 ) && ( front < 0.6 ) )	//Avstand til hinder forran er mindre enn 60 cm
					{
						turned = turnLeft(); //snu 90grader
						turned = true;
					}
					
				i++;
				usleep(1000);
				}while( i < 1 );	
		
			
		if (i >=1)i=0;
		usleep(10000);

		}while(ok);
	}
	*/

	void lookFront()
	{
		const float *rangev;  // rangevector
		unsigned int rangec;  // rangecount
		
		Coordinate objectPos,robotPos;
		//AngularCoordinate robotPos,robotPos;

		robotPos = this->pBrain->odom()->getPosition();
		rec::robotino::api2::LaserRangeFinderReadings  r;

		float robotX		= robotPos.x(); 
		float robotY		= robotPos.y();
		float laserAngle	= 0;
		float objectLength	= 0;
		float objectX, objectY;
		int hindringer		= 0;
		int i=0, y=1, z=0;
		
		robotPos	= Coordinate ( robotX , robotY );

		for(i = 0; i < 513; i++)
		{
			r = this->pBrain ->lrf()->getReadings();
			r.ranges( &rangev, &rangec );

			objectLength	= rangev[ i ];			// Leser avstand til objekt i cm
			//laserAngle	= ( (180.0/512.0) * i );	// Beregner vinkel til objekt

			if ( i > 0 && i <= 256 ) laserAngle	= ( (180.0/512.0) * i );	// Beregner vinkel til objekt

			if ( i > 256 && i < 513 )
			{
				// Må gjøre om slik at vi får 0 ved 512 og 256 ved 256
				z = ( i  * 1 - ( 2 * y ) );
				laserAngle	= ( (180.0/512.0) * z );	// Beregner vinkel til objekt
				
				y++;
				//std::cerr << i << " - " << z <<" -  "<<laserAngle<< std::endl;
			}

			objectX		= ( (objectLength * 1.0) * cos(laserAngle * 3.141592 / 180.0) );
			objectY		= ( (objectLength * 1.0) * sin(laserAngle * 3.141592 / 180.0) ); 
			objectPos	= Coordinate ( objectX + robotY , objectY + robotX );
			//objectPos	= objectPos + robotPos; 

			if ( objectLength <= 0.30 )
			{

			std::cerr << "Robotens pos: " << robotPos  << ". Objektets pos: " <<" i: "<<i << objectPos << std::endl;
			std::cerr << "Sensor leser av " << objectLength << " cm til objektet\t " << laserAngle * 1 << " grader for Robotino.\n" << std::endl;

			hindringer++;
			}

			usleep(1000);
		}

		if ( hindringer == 0 ) std::cerr << "Ingen hindringer innenfor 100 cm avstand." << std::endl;
	}

};


