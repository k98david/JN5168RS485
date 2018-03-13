/*****************************************************************************/
/*!
 *\MODULE              Config (Application Layer)
 *
 *\COMPONENT           $HeadURL: https://www.collabnet.nxp.com/svn/lprf_apps/Application_Notes/JN-AN-1069-IEEE-802.15.4-Serial-Cable-Replacement/Tags/Release_3v0-Public/Common/Source/config.h $
 *
 *\VERSION			   $Revision: 16555 $
 *
 *\REVISION            $Id: config.h 16555 2015-11-20 12:14:37Z nxa04494 $
 *
 *\DATED               $Date: 2015-11-20 12:14:37 +0000 (Fri, 20 Nov 2015) $
 *
 *\AUTHOR              $Author: nxa04494 $
 *
 *\DESCRIPTION         Config (Definitions)
 */
/****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on each
 * copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd 2009. All rights reserved
 *
 ****************************************************************************/

#ifndef  CONFIG_H_INCLUDED
#define  CONFIG_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <jendefs.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define WATCHDOG_ENABLED 1
#define Z200	1
#define Z200485 1
#define Z200Old485 0
#define Z200USB	2
#define hardware Z200USB
//#define hardware Z200485
//#define SendTestUse
#define CTSpin	12  //from the view point of CP2104
#define RTSpin	13  //form the view point of CP2104
#if  hardware == Z200485
	#define LED1pin	4  //upper green light
	#define LED2pin 5
	#define KEY0pin	11
	#define KEY1pin	1
#endif
#if hardware == Z200Old485
	#define LED1pin	16  //upper green light
	#define LED2pin 17
	#define KEY0pin	4
	#define KEy1pin	5
#endif
#if hardware == Z200USB
	#define KEY0pin	4
	#define LED1pin	16  //upper green light
   // #define LED1pin	11  //gatway test
	#define LED2pin 17
	//#define CTSRTS	1
#else
	#undef CTSRTS
#endif

#define LED3pin	11
#if (hardware == Z200485) | (hardware == Z200Old485)
	#define REpin	12
	#define DEpin 	13
#endif


/* Network parameters */
#define Default_Channel				25
#define DefaultApplicationResendTimes			1
//#define PAN_ID                      0x1234
#define PAN_ID                      0xBEEF
#define MAX_CHILDREN             	5

/* Defines the channels to scan. Each bit represents one channel. All channels
   in the channels (11-26) in the 2.4GHz band are scanned. */
#define SCAN_CHANNELS 		        0x07FFF800UL
//#define SCAN_CHANNELS 		        0x00000800UL
//#define CHANNEL_MIN                 26
//#else
//#define SCAN_CHANNELS 		        0x04000000UL
#define CHANNEL_MIN                 11
//#endif

/* Sleep period between scans of the full channel mask for end devices */
#define SCAN_SLEEP					32000

/* Ticks per ms on tick timer */
#define TICK_TIMER_MS  				16000

/* Duration (ms) = 15.36ms x (2^ACTIVE_SCAN_DURATION + 1) */
#define ACTIVE_SCAN_DURATION        3
/* Duration (ms) = 15.36ms x (2^ENERGY_SCAN_DURATION + 1) */
#define ENERGY_SCAN_DURATION        3

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* CONFIG_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
