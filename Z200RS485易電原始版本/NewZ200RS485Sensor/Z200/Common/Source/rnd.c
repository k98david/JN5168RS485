/*****************************************************************************/
/*!
 *\MODULE              Random (Application Layer)
 *
 *\COMPONENT           $HeadURL: https://www.collabnet.nxp.com/svn/lprf_apps/Application_Notes/JN-AN-1069-IEEE-802.15.4-Serial-Cable-Replacement/Tags/Release_3v0-Public/Common/Source/rnd.c $
 *
 *\VERSION			   $Revision: 16555 $
 *
 *\REVISION            $Id: rnd.c 16555 2015-11-20 12:14:37Z nxa04494 $
 *
 *\DATED               $Date: 2015-11-20 12:14:37 +0000 (Fri, 20 Nov 2015) $
 *
 *\AUTHOR              $Author: nxa04494 $
 *
 *\DESCRIPTION         Random (Application Layer) - implementation.
 */
/*****************************************************************************
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

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
/* Standard includes */
#include <stdlib.h>
/* Jennic includes */
#include <jendefs.h>
#include "AppHardwareApi.h"
#include <AppHardwareApi_JN516x.h>
/* Application includes */
#include "rnd.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: RND_vInit
 *
 * DESCRIPTION:
 * Initialises random number generator.
 *
 ****************************************************************************/
PUBLIC void		RND_vInit(void)
{
	/* Start RND generator in continuous mode and without interrupts */
	vAHI_StartRandomNumberGenerator(E_AHI_RND_CONTINUOUS, E_AHI_INTS_DISABLED);
}

/****************************************************************************
 *
 * NAME: RND_u32GetRand
 *
 * DESCRIPTION:
 * Calculate random number.
 *
 ****************************************************************************/
PUBLIC uint32 RND_u32GetRand(uint32 u32Min, uint32 u32Max)
{
	if (u32Min == u32Max)
	{
		/* Return only possible result */
		return u32Min;
	}
	else
	{
		uint32 u32Range;
		uint32 u32Random = 0;

		/* Calculate range */
		u32Range = u32Max - u32Min;

		/* Get a random value and scale it to the range specified */
		/* Range+1 because otherwise the upper limit is never reached */
		u32Random = u32Min + (u16AHI_ReadRandomNumber() % (u32Range+1));

		/* Return result */
		return u32Random;
	}
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
