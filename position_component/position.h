//-------------------------------------------------------------------------------------------------
/**
 * @file position.h
 *
 * Helper lib wrapping le_pos, le_posCtrl and le_avdata, to simplify the 2 following key features:
 *      Get the current location coordinates (longitude, latitude, altitude, accuracies)
 *      Push the current location to AirVantage
 *
 *  NC - March 2018
 */
//-------------------------------------------------------------------------------------------------

#ifndef _POSITION_H_
#define _POSITION_H_

typedef enum
{
	POSITION_LOCATION_NO = 0,
	POSITION_LOCATION_2D = 1,
	POSITION_LOCATION_3D =2
} position_location_type_t;


//Call this function first to initialize positioning
void position_Start();

//Call this function when exiting the app to release positioning lib
void position_Stop();

//Call this function to push the current position to AirVantage.
le_result_t position_PushLocation(le_pos_FixState_t *		fixStatePtr);

//To retrive the position, the return informs the type of positioning being retrieved (2D, 3D or failure)
position_location_type_t position_GetLocation
								(
									double*     			dLatitude,
									double*     			dLongitude,
									int32_t*    			hAccuracy,
									int32_t*    			altitude,
									int32_t*    			vAccuracy,
									le_pos_FixState_t * 	fixStatePtr
								);

//Push the specified 3D position to AirVantage
le_result_t position_Push2DLocation(double dLatitude, double dLongitude, double dRadius);

//Push the specified 2D position to AirVantage
le_result_t position_Push3DLocation(double dLatitude, double dLongitude, double dRadius, double dAltitude, double dvRadius);

#endif //_POSITION_H_
