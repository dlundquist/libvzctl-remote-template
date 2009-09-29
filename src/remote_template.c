/*
 *  Copyright (C) 2009 Blue Box Group, LLC, All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <modules.h>
#include <getopt.h> /* in get_opt() and parse_opt() */
#include <stdlib.h> /* exit() */
#include <string.h> /* strlen() strncpy */
#include "remote_template.h"
#include "logger.h" /* logger() */
#include "util.h" /* check_var() */
#include "vzerror.h" /* error codes */
#include "destroy.h" /* del_dir() */
#include "script.h" /* run_script() */

#ifndef lint
static const char rcsid[] = "$Id$";
#endif

/*
 * alloc_data() is a hook to allocated any datastructures required for this module, it's result is
 * passwd to init()
 */
static data_param *alloc_data() {
	data_param *data;

#ifdef DEBUG	
	printf(__FILE__ " alloc_data() called\n");
#endif
	/* Allocate our data_param structure - just has cfg and opt pointers */
	data = (data_param*) calloc(1, sizeof(data_param));
	if (data == NULL) {
		logger(-1, errno, "Error malloc ");
		exit(1);
	};
	/* And our (local) structure to hold our two command line options */
	data->opt = (mod_options*) calloc(1, sizeof(mod_options));
	if (data->opt == NULL) {
		logger(-1, errno, "Error malloc ");
		exit(1);
	}
	return (data);
}

/*
 * init() is a hook that is called when the moduels is loaded it is passed a pointer to it's data struture
 * allocated by alloc_data()
 */
/*
static int init(data_param *data) {
	return (0);
}
*/

/*
 * free_data() releases all internal data structures
 */
static void free_data(data_param *data) {
	mod_options *options;

#ifdef DEBUG	
	printf(__FILE__ " free_data(%p) called\n", data);
#endif
	if (data->opt) {
		options = (mod_options*) data->opt; /* only cast once */

		if (options->template_url)
			free (options->template_url);
		if (options->template_file)
			free (options->template_file);
		free (data->opt);
	}
	if (data->cfg)
		free (data->cfg);
	free (data);
}

/*
 * get_opt should returns additional options we looking on the command line
 * data a pointer to any data we allocated in alloc_data()
 * action is the action name
 */
static struct option *get_opt(data_param *data, const char *action) {
	static struct option create_options[] = {
		{"template_file", required_argument, NULL, PARAM_TEMPLATE_FILE},
		{"template_url", required_argument, NULL, PARAM_TEMPLATE_URL},
		{NULL, 0, NULL, 0},
	};

#ifdef DEBUG	
	printf(__FILE__ " get_opt(%p, %s) called\n", data, action);
#endif

	return (create_options);
}

/*
 * parse_opt is called to parse our get_opts option
 * it is called for each option - opt and some additional ones with the value in rval
 * this function should return one of the ERR_ codes in include/types.h
 */
static int parse_opt(envid_t veid, data_param *data, int opt, const char *rval) {
	size_t len;
	/*
	 * data_params only provides to void pointers so we use a local mod_options
	 * pointer so we don't have to cast on every access
	 */
	mod_options *options = (mod_options*) data->opt;

#ifdef DEBUG	
	printf(__FILE__ " parse_opt(%d, %p, %d, %s) called\n", veid, data, opt, rval);
#endif
	len = strlen(rval);

	switch (opt) {
	case PARAM_TEMPLATE_FILE:
		if (options->template_url)
			return ERR_INVAL_SKIP;

		if (options->template_file)
			return ERR_DUP;

		options->template_file = (char*) malloc(len + 1);

		if (options->template_file == NULL)
			return ERR_NOMEM;

		strncpy(options->template_file, rval, len + 1);
		break;
	case PARAM_TEMPLATE_URL:
		if (options->template_file)
			return ERR_INVAL_SKIP;

		if (options->template_url)
			return ERR_DUP;

		options->template_url = (char*) malloc(len + 1);

		if (options->template_url == NULL)
			return ERR_NOMEM;

		strncpy(options->template_url, rval, len + 1);
		break;
	}
	
	return (0);
}

/* this is the function that gets called to preform the action we defined in actions
 * there are still many questions:
 *    * if we define multiple actions how do we detect which action we are being called on
 *
 * most of this was figured out from lib/create.c vps_create()
 */
static int setup(vps_handler *h, envid_t veid, data_param *data,
		int vps_state, int skip, vps_param *param) {
	/* h is our container handler - actual handle to interface with kernel stuff */
	/* veid - integer veid */
	/* data - result of alloc_data function in mod_info if defined */
	/* vps_state - STATE_STARTING, _RUNNING, _STOPPED, _STOPPING, _RESTORING, _CHECKPOINTING */
	/* skip - flag set to 0 in lib/create.c */
	/* param - vps parameter */
	int ret = 0;
	char tarball[PATH_LEN];
	char tmp_dir[PATH_LEN];
	char buf[PATH_LEN];
	char *arg[2];
	char *env[4];
	fs_param *fs = &param->res.fs; /* file system info */
	tmpl_param *tmpl = &param->res.tmpl; /* our template into */
	dq_param *dq = &param->res.dq; /* Disk quota parameters */
	mod_options *options = (mod_options*) data->opt; /* only cast once */
	int quota = 0; /* quota flag */

#ifdef DEBUG	
	printf(__FILE__ " setup(%p, %d, %p, %d, %d, %p) called\n", h, veid, data, vps_state, skip, param);
	printf(__FILE__ " os_template:\t%s\n", tmpl->ostmpl);
	printf(__FILE__ " private_dir:\t%s\n", fs->private);
	printf(__FILE__ " root_dir:\t%s\n", fs->root);
	printf(__FILE__ " template_file:\t%s\n", options->template_file);
	printf(__FILE__ " template_url:\t%s\n", options->template_url);
#endif

        if (check_var(tmpl->ostmpl, "OS template is not specified"))
		return VZ_VE_PKGSET_NOTSET;

	/* We have three options for choosing a source for our OS template */
	if(options->template_url) {
		/* TODO test template url:
		   this will require spawning an external script or using libcurl or similar
		*/
	} else if (options->template_file) {
		/* We are using expecting a full file path, so check that first then fall back to ostemplate style paths */
		snprintf(tarball, sizeof(tarball), "%s", options->template_file);
		if (!stat_file(tarball))
			snprintf(tarball, sizeof(tarball), "%s/cache/%s", fs->tmpl, options->template_file);
		if (!stat_file(tarball))
			snprintf(tarball, sizeof(tarball), "%s/cache/%s.tar", fs->tmpl, options->template_file);
		if (!stat_file(tarball))
			snprintf(tarball, sizeof(tarball), "%s/cache/%s.tar.gz", fs->tmpl, options->template_file);
		if (!stat_file(tarball)) {
			logger(-1, 0, "Cached os template %s not found", tarball);
			return VZ_PKGSET_NOT_FOUND;
		}
	} else {
		/* Fall back to using ostemplate as basename */
		/* We do not support using VZOSTEMPLATE to transform the ostemplate name on the fly */
        	snprintf(tarball, sizeof(tarball), "%s/cache/%s.tar", fs->tmpl, tmpl->ostmpl);
        	if (!stat_file(tarball))
                	snprintf(tarball, sizeof(tarball), "%s/cache/%s.tar.gz", fs->tmpl, tmpl->ostmpl);
		if (!stat_file(tarball)) {
			logger(-1, 0, "Cached os template %s not found", tarball);
			return VZ_PKGSET_NOT_FOUND;
	        }
	}

	/* The remainder of this function is copied almost exactly from vzctl create.c fs_create */

        /* Lock CT area */
        if (make_dir(fs->private, 0))
                return VZ_FS_NEW_VE_PRVT;
        snprintf(tmp_dir, sizeof(tmp_dir), "%s.tmp", fs->private);
        if (stat_file(tmp_dir)) {
                logger(-1, 0, "Warning: Temp dir %s already exists, deleting",
                        tmp_dir);
                if (del_dir(tmp_dir)) {
                        ret = VZ_FS_NEW_VE_PRVT;
                        goto err;
                }
        }
        if (make_dir(tmp_dir, 1)) {
                logger(-1, errno, "Unable to create directory %s", tmp_dir);
                ret = VZ_FS_NEW_VE_PRVT;
                goto err;
        }
        if (dq != NULL &&
                dq->enable == YES &&
                dq->diskspace != NULL &&
                dq->diskinodes != NULL)
        {
                if (!quota_ctl(veid, QUOTA_STAT))
                        quota_off(veid, 0);
                quota_ctl(veid, QUOTA_DROP);
                quota_init(veid, tmp_dir, dq);
                quota_on(veid, tmp_dir, dq);
                quota = 1; /* We have enabled quota, so raise flag so we know to clean it up */
        }
	if (options->template_url) {
		arg[0] = VPS_REMOTE_CREATE;
		arg[1] = NULL;
		snprintf(buf, sizeof(buf), "TEMPLATE=%s", options->template_url);
		env[0] = strdup(buf);
		snprintf(buf, sizeof(buf), "VE_PRVT=%s", tmp_dir);
		env[1] = strdup(buf);
		env[2] = strdup(ENV_PATH);
		env[3] = NULL;
	} else {
		arg[0] = VPS_CREATE;
		arg[1] = NULL;
		snprintf(buf, sizeof(buf), "PRIVATE_TEMPLATE=%s", tarball);
		env[0] = strdup(buf);
		snprintf(buf, sizeof(buf), "VE_PRVT=%s", tmp_dir);
		env[1] = strdup(buf);
		env[2] = strdup(ENV_PATH);
		env[3] = NULL;
	}
        ret = run_script(arg[0], arg, env, 0);
        free_arg(env);
        if (ret)
                goto err;
        if (quota) {
                quota_off(veid, 0);
                quota_set(veid, fs->private, dq);
                quota = 0;
        }
        /* Unlock CT area */
        rmdir(fs->private);
        if (rename(tmp_dir, fs->private)) {
                logger(-1, errno, "Can't rename %s to %s", tmp_dir, fs->private);
                ret = VZ_FS_NEW_VE_PRVT;
        }

err:
        if (ret && quota) {
                quota_off(veid, 0);
                quota_ctl(veid, QUOTA_DROP);
        }
        rmdir(fs->private);
        rmdir(tmp_dir);

        return ret;
}

/*
 * get_usage should return a char pointer to be appended to the usage output
 */
static const char *get_usage() {
	static const char *usage = "This version of vzctl has the Blue Box Group remote template module loaded\n"
	"that supports the following expanded create syntax:\n"
	"vzctl create <veid> --ostemplate <name>] [--config <name>]\n"
	"   [--private <path>] [--root <path>] [--ipadd <addr>] [--hostname <name>]\n"
	"   [--template_url <url_path>] | [--template_file <file_path>]\n";

#ifdef DEBUG	
	printf(__FILE__ " get_usage() called\n");
#endif
	return (usage);
}

/* vz_get_mod_info returns some structure to vzctl that describes this module
 * using this as a hook to examine our environment */
struct mod_info * vz_get_mod_info() {
	/* An array of vzctl actions that we are loaded for */
	static char *actions[] = { "create", NULL };
	static struct mod_info this_mod = {
		NULL, /* void *handle; this is populated by vzctl module.c add_module() */
		actions, /* char **actions; */
		8786, /* int id;  */
		"Remote Template", /* char *desc; */
		&alloc_data, /* data_param *(*alloc_data)(void); */
		NULL, /* int (*init)(data_param *data); */
		NULL, /* int (*parse_cfg)(envid_t veid, data_param *data, const char *name, 
			      const char *rval); */
		&parse_opt, /* int (*parse_opt)(envid_t veid, data_param *data, int opt,
				const char *rval); */
		NULL, /* int (*store)(data_param *data, list_head_t *conf_head); */
		&setup, /* int (*setup)(vps_handler *h, envid_t veid, data_param *data,
				int vps_state, int skip, vps_param *param); */
		NULL, /* int (*cleanup)(vps_handler *h, envid_t veid, data_param *data,
				vps_param *param); */
		&free_data, /* void (*free_data)(data_param *data); */
		&get_opt, /* struct option *(*get_opt)(data_param *data, const char *action); */
		&get_usage, /* const char *(*get_usage)(); */
	};

#ifdef DEBUG	
	printf(__FILE__ " mod_info() called\n");
#endif
	return (&this_mod);
}
