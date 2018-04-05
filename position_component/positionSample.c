//-------------------------------------------------------------------------------------------------
/**
 * @file positionSample.c
 *
 * Sample app using a positioning helper lib to post the current location to AirVantage
 * Only 1 function call is required to achieve this : position_PushLocation
 *   + lib initialization/release: position_Start, position_Stop
 *
 *  NC - March 2018
 */
//-------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"

//Please refer to this include file for more functions exposed by the helper lib
#include "position.h"

//Global var
static le_timer_Ref_t						_positionTimerRef = NULL;  


void OnDemandFix();


//Function to start the timer with a specified delay (seconds)
void SetTimerDelay(uint32_t	delaySecond)
{
	if (!_positionTimerRef)
	{
		_positionTimerRef = le_timer_Create("positionTimer");     //create timer
		le_timer_SetRepeat(_positionTimerRef, 1);                 //set repeat to once

		//set callback function to handle timer expiration event
		le_timer_SetHandler(_positionTimerRef, OnDemandFix);
	}	

	le_clk_Time_t      interval = { delaySecond, 0 };             //set timer delay
	le_timer_SetInterval(_positionTimerRef, interval);

	LE_INFO("Next update in %d seconds...", delaySecond);
	
	//start timer
	le_timer_Start(_positionTimerRef);
}

//function called by the above timer upon expiration
//this function call helper lib's position_PushLocation to push the current location to AirVantage
//the timer delay varies depending on the gnss fix state
void OnDemandFix()
{
	le_pos_FixState_t 	fixState;

	position_PushLocation(&fixState);

	if (LE_POS_STATE_FIX_3D == fixState)
	{
		SetTimerDelay(40);	//make delay longer as we get 3Dfix
	}
	else if (LE_POS_STATE_FIX_2D == fixState)
	{
		SetTimerDelay(20);
	}
	else
	{
		SetTimerDelay(10);
	}
}

//main start
COMPONENT_INIT
{
	le_sig_Block(SIGTERM);
	le_sig_SetEventHandler(SIGTERM, position_Stop); //call position_Stop upon program exit

	//Call position_Start to initialize the positioning lib
    position_Start();

	//start the timer with 10s delay
    SetTimerDelay(10);
}
