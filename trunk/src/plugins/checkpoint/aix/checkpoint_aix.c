/*****************************************************************************\
 *  checkpoint_aix.c - AIX slurm checkpoint plugin.
 *****************************************************************************
 *  Copyright (C) 2004 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Morris Jette <jette1@llnl.gov>
 *  UCRL-CODE-2002-040.
 *  
 *  This file is part of SLURM, a resource management program.
 *  For details, see <http://www.llnl.gov/linux/slurm/>.
 *  
 *  SLURM is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *  
 *  SLURM is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with SLURM; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#if HAVE_STDINT_H
#  include <stdint.h>
#endif
#if HAVE_INTTYPES_H
#  include <inttypes.h>
#endif

#include <stdio.h>
#include <slurm/slurm.h>
#include <slurm/slurm_errno.h>

#include "src/common/log.h"
#include "src/common/pack.h"
#include "src/common/xmalloc.h"
#include "src/slurmctld/slurmctld.h"

struct check_job_info {
	uint16_t ckpt_errno;
	uint16_t disabled;
};

/*
 * These variables are required by the generic plugin interface.  If they
 * are not found in the plugin, the plugin loader will ignore it.
 *
 * plugin_name - a string giving a human-readable description of the
 * plugin.  There is no maximum length, but the symbol must refer to
 * a valid string.
 *
 * plugin_type - a string suggesting the type of the plugin or its
 * applicability to a particular form of data or method of data handling.
 * If the low-level plugin API is used, the contents of this string are
 * unimportant and may be anything.  SLURM uses the higher-level plugin
 * interface which requires this string to be of the form
 *
 *	<application>/<method>
 *
 * where <application> is a description of the intended application of
 * the plugin (e.g., "checkpoint" for SLURM checkpoint) and <method>
 * is a description of how this plugin satisfies that application.  SLURM will
 * only load checkpoint plugins if the plugin_type string has a 
 * prefix of "checkpoint/".
 *
 * plugin_version - an unsigned 32-bit integer giving the version number
 * of the plugin.  If major and minor revisions are desired, the major
 * version number may be multiplied by a suitable magnitude constant such
 * as 100 or 1000.  Various SLURM versions will likely require a certain
 * minimum versions for their plugins as the checkpoint API matures.
 */
const char plugin_name[]       	= "Checkpoint AIX plugin";
const char plugin_type[]       	= "checkpoint/aix";
const uint32_t plugin_version	= 90;

/*
 * init() is called when the plugin is loaded, before any other functions
 * are called.  Put global initialization here.
 */
int init ( void )
{
	return SLURM_SUCCESS;
}


int fini ( void )
{
	return SLURM_SUCCESS;
}

/*
 * The remainder of this file implements the standard SLURM checkpoint API.
 */

extern int slurm_ckpt_op ( uint16_t op, uint16_t data,
		struct step_record * step_ptr )
{
	int rc = SLURM_SUCCESS;

	switch (op) {
		case CHECK_COMPLETE:
			step_ptr->check_job->ckpt_errno = 0;
			if (step_ptr->check_job->vacate)
			break;
		case CHECK_DISABLE:
			step_ptr->check_job->disabled = 1;
			break;
		case CHECK_ENABLE:
			step_ptr->check_job->disabled = 0;
			break;
		case CHECK_FAILED:
			step_ptr->check_job->ckpt_errno = data;
			break;
		case CHECK_CREATE:
		case CHECK_VACATE:
		case CHECK_RESUME:
			rc = ESLURM_NOT_SUPPORTED;
			break;
		default:
			error("Invalid checkpoint operation: %d", op);
			rc = EINVAL;
	}

	return rc;
}

extern int slurm_ckpt_error ( struct step_record * step_ptr, 
		uint32_t *ckpt_errno, char **ckpt_strerror)
{
	if (ckpt_errno)
		*ckpt_errno = step_ptr->check_job->ckpt_errno;
	else
		return EINVAL;

	if (ckpt_strerror)
		*ckpt_strerror = "TBD";
	else
		return EINVAL;

	return SLURM_SUCCESS;
}

extern int slurm_ckpt_alloc_job(check_jobinfo_t *jobinfo)
{
	*jobinfo = (check_jobinfo_t) xmalloc(sizeof(check_jobinfo_t));
	return SLURM_SUCCESS;
}

extern int slurm_ckpt_free_job(check_jobinfo_t jobinfo)
{
	xfree(jobinfo);
	return SLURM_SUCCESS;
}

extern int slurm_ckpt_pack_job(check_jobinfo_t jobinfo, Buf buffer)
{
	pack16(job_info->ckpt_errno, buffer);
	pack16(job_info->disabled, buffer);

	return SLURM_SUCCESS;
}

extern int slurm_ckpt_unpack_job(check_jobinfo_t jobinfo, Buf buffer)
{
	safe_unpack16(&jobinfo->ckpt_errno, buffer);
	safe_unpack16(&jobinfo->disabled, buffer);

	return SLURM_SUCCESS; 

    unpack_error:
	return SLURM_ERROR;
}

