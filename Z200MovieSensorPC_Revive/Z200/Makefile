#########################################################################
#
# MODULE:   JN-AN-1046-IEEE802.15.4-App-Template
#
# DESCRIPTION: MakeFile
#
############################################################################
# 
# This software is owned by Jennic and/or its supplier and is protected
# under applicable copyright laws. All rights are reserved. We grant You,
# and any third parties, a license to use this software solely and
# exclusively on Jennic products. You, and any third parties must reproduce
# the copyright and warranty notice and any other legend of ownership on each
# copy or partial copy of the software.
# 
# THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
# EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
# ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
# BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
# INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
# 
# Copyright Jennic Ltd 2009. All rights reserved
#
#############################################################################
# Subversion variables
# $HeadURL: https://www.collabnet.nxp.com/svn/lprf_apps/Application_Notes/JN-AN-1069-IEEE-802.15.4-Serial-Cable-Replacement/Tags/Release_3v0-Public/Makefile $
# $Revision: 16560 $
# $LastChangedBy: nxa04494 $
# $LastChangedDate: 2015-11-23 09:28:11 +0000 (Mon, 23 Nov 2015) $
# $Id: Makefile 16560 2015-11-23 09:28:11Z nxa04494 $
#
#############################################################################

DIRS	?= Coordinator/Build EndDevice/Build
CHIPS	?= JN5168 JN5169

all: 
	$(foreach i, $(DIRS), $(foreach j, $(CHIPS), $(MAKE) -C $(i) JENNIC_CHIP=$(j) $(MFLAGS) clean all;))

clean: 
	$(foreach i, $(DIRS), $(foreach j, $(CHIPS), $(MAKE) -C $(i) JENNIC_CHIP=$(j) $(MFLAGS) clean;))