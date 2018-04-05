//-------------------------------------------------------------------------------------------------
/**
 * @file position.c
 *
 * Helper lib wrapping le_pos, le_posCtrl and le_avdata, to simplify the 2 following key features:
 *      Get the current location coordinates (longitude, latitude, altitude, accuracies)
 *      Push the current location to AirVantage
 *
 *  NC - March 2018
 */
//-------------------------------------------------------------------------------------------------


#include "legato.h"
#include "interfaces.h"

#include "position.h"

//data path for location objects
#define GPS_LAT                             "lwm2m.6.0.0"
#define GPS_LONG                            "lwm2m.6.0.1"
#define GPS_ALTITUDE                        "lwm2m.6.0.2"
#define GPS_RADIUS                          "lwm2m.6.0.3"

//Global variables
static le_posCtrl_ActivationRef_t   		_posCtrlRef = NULL;
static le_avdata_RequestSessionObjRef_t		_requestSessionRef = NULL;

//Callback function to handler AirVantage publishing status
void position_PushRecordCallbackHandler
(
    le_avdata_PushStatus_t status,
    void* contextPtr
)
{
    if (status == LE_AVDATA_PUSH_SUCCESS)
 	{
 		LE_INFO("Push Location OK");
 	}
 	else
 	{
 		LE_INFO("Failed to push Location");
 	}
}

//Helper, request a connection to AirVantage if not done yet
le_avdata_RequestSessionObjRef_t position_CheckConnection()
{
	if (!_requestSessionRef)
	{
		_requestSessionRef = le_avdata_RequestSession();
	}

	return _requestSessionRef;
}

//Helper function to perform a 3D fix and push location to AirVantage
le_result_t position_Push3DLocation(double dLatitude, double dLongitude, double dRadius, double dAltitude, double dvRadius)
{
	if (NULL == position_CheckConnection())
	{
		return LE_FAULT;
	}

	le_avdata_RecordRef_t recordRef = le_avdata_CreateRecord();

	uint64_t        utcMilliSec;
	struct timeval  tv;
	gettimeofday(&tv, NULL);
	utcMilliSec = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;


	le_avdata_RecordFloat(recordRef, GPS_LAT, dLatitude, utcMilliSec);
	le_avdata_RecordFloat(recordRef, GPS_LONG, dLongitude, utcMilliSec);
	le_avdata_RecordFloat(recordRef, GPS_RADIUS, dRadius, utcMilliSec);
	le_avdata_RecordFloat(recordRef, GPS_ALTITUDE, dAltitude, utcMilliSec);

	le_result_t res = le_avdata_PushRecord(recordRef, position_PushRecordCallbackHandler, NULL);

	if (LE_FAULT == res)
	{
		LE_INFO("Failed pushing GNSS");
	}
	else if (LE_OK == res)
	{
		LE_INFO("GNSS location pushed OK");
	}

	le_avdata_DeleteRecord(recordRef);

	return res;
}

//Helper function to perform a 2D fix and push location to AirVantage
le_result_t position_Push2DLocation(double dLatitude, double dLongitude, double dRadius)
{
	if (NULL == position_CheckConnection())
	{
		return LE_FAULT;
	}
	
	le_avdata_RecordRef_t recordRef = le_avdata_CreateRecord();

	uint64_t        utcMilliSec;
	struct timeval  tv;
	gettimeofday(&tv, NULL);
	utcMilliSec = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;


	le_avdata_RecordFloat(recordRef, GPS_LAT, dLatitude, utcMilliSec);
	le_avdata_RecordFloat(recordRef, GPS_LONG, dLongitude, utcMilliSec);
	le_avdata_RecordFloat(recordRef, GPS_RADIUS, dRadius, utcMilliSec);

	le_result_t res = le_avdata_PushRecord(recordRef, position_PushRecordCallbackHandler, NULL);

	if (LE_FAULT == res)
	{
		LE_INFO("Failed pushing GNSS");
	}
	else if (LE_OK == res)
	{
		LE_INFO("GNSS location pushed OK");
	}

	le_avdata_DeleteRecord(recordRef);

	return res;
}

//retrieve the current location, automatically get 2D or 3D location depending on the gnss fix state
position_location_type_t position_GetLocation
(
	double*     			dLatitude,
	double*     			dLongitude,
	int32_t*    			hAccuracy,
	int32_t*    			altitude,
	int32_t*    			vAccuracy,
	le_pos_FixState_t * 	fixStatePtr
)
{
	position_location_type_t	ret = POSITION_LOCATION_NO;    //0=KO, 1=2D, 2=3D
	le_pos_FixState_t			fixState = LE_POS_STATE_UNKNOWN;
	le_result_t					res;
	int32_t						latitude;
	int32_t						longitude;

	if (LE_OK == le_pos_GetFixState(&fixState))
	{
		LE_INFO("position fix state %d", fixState);

		if (LE_POS_STATE_FIX_3D == fixState)
		{
			res = le_pos_Get3DLocation(&latitude, &longitude, hAccuracy, altitude, vAccuracy);

			LE_INFO("le_pos_Get3DLocation %s",
					(LE_OK == res) ? "OK" : (LE_OUT_OF_RANGE == res) ? "parameter(s) out of range":"ERROR");
			LE_INFO("Get3DLocation latitude.%d, longitude.%d, hAccuracy.%d, altitude.%d, vAccuracy.%d", latitude, longitude, *hAccuracy, *altitude, *vAccuracy);

			if (LE_OK == res)
			{
				*dLatitude = (double)latitude/1000000.0;
				*dLongitude = (double)longitude/1000000.0;
				ret = POSITION_LOCATION_3D;
			}
		}
		else
		{
			res = le_pos_Get2DLocation(&latitude, &longitude, hAccuracy);

			LE_INFO("le_pos_Get2DLocation %s",
					(LE_OK == res) ? "OK" : (LE_OUT_OF_RANGE == res) ? "parameter(s) out of range":"ERROR");
			LE_INFO("Get2DLocation latitude.%d, longitude.%d, hAccuracy.%d",
					latitude, longitude, *hAccuracy);

			if (LE_OK == res)
			{
				*dLatitude = (double)latitude/1000000.0;
				*dLongitude = (double)longitude/1000000.0;
				*altitude = 0;
				*vAccuracy = 0;
				ret = POSITION_LOCATION_2D;
			}
		}            
	}
	else
	{
		LE_INFO("Failed to GetFixState");
	}

	if (fixStatePtr)
	{
		*fixStatePtr = fixState;
	}

	return ret;
}

//a simple helper function to perform on-demand fix and post locatin to AirVantage
le_result_t position_PushLocation(le_pos_FixState_t * 	fixStatePtr)
{
	le_result_t res = LE_FAULT;

	double	latitude;
	double	longitude;
	int32_t	hAccuracy;
	int32_t	altitude;
	int32_t	vAccuracy;

	position_location_type_t ret = position_GetLocation(&latitude, &longitude, &hAccuracy, &altitude, &vAccuracy, fixStatePtr);

	if (POSITION_LOCATION_3D == ret)
	{
		res = position_Push3DLocation(latitude, longitude, hAccuracy, altitude, vAccuracy);
	}
	else if (POSITION_LOCATION_2D == ret)
	{
		res = position_Push2DLocation(latitude, longitude, hAccuracy);
	}

	return res;
}

//release positioning service and release session with AirVantage
void position_Stop()
{
	if (NULL != _posCtrlRef)
	{
		LE_INFO("Releasing positioning service");
		le_posCtrl_Release(_posCtrlRef);
	}

	if (_requestSessionRef)
	{
		LE_INFO("Releasing AVC Session");
		le_avdata_ReleaseSession(_requestSessionRef);
	}
}

//Initialize the positioning service
void position_Start()
{
	_posCtrlRef = le_posCtrl_Request();
	if (NULL == _posCtrlRef)
	{
		LE_INFO("Cannot activate le_pos !");
	}
}

