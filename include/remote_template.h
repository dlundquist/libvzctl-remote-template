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
#ifndef _REMOTE_TEMPLATE_H_
#define _REMOTE_TEMPLATE_H_

#define PARAM_TEMPLATE_FILE 16998
#define PARAM_TEMPLATE_URL 16999

#define VPS_CREATE              LIB_SCRIPTS_DIR "vps-create"
#define VPS_REMOTE_CREATE       LIB_SCRIPTS_DIR "vps-remote-create"

typedef struct {
        char *template_file;
        char *template_url;
} mod_options;

int setup(vps_handler *, envid_t, data_param *, int, int, vps_param *);
struct option *get_opt(data_param *, const char *);
int parse_opt(envid_t, data_param *, int, const char *);
data_param *alloc_data(void);
void free_data(data_param *);
const char *get_usage(void);
#endif

