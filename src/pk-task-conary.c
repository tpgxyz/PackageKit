/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2007 Ken VanDine <ken@vandine.org>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <glib/gi18n.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "pk-debug.h"
#include "pk-task.h"
#include "pk-task-common.h"
#include "pk-spawn.h"
#include "pk-network.h"

static void     pk_task_class_init	(PkTaskClass *klass);
static void     pk_task_init		(PkTask      *task);
static void     pk_task_finalize	(GObject     *object);

#define PK_TASK_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PK_TYPE_TASK, PkTaskPrivate))

struct PkTaskPrivate
{
	gboolean		 whatever_you_want;
	guint			 progress_percentage;
	PkNetwork		*network;
};

static guint signals [PK_TASK_LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (PkTask, pk_task, G_TYPE_OBJECT)

/**
 * pk_task_get_actions:
 **/
gchar *
pk_task_get_actions (void)
{
	gchar *actions;
	actions = pk_task_action_build (PK_TASK_ACTION_INSTALL,
				        PK_TASK_ACTION_REMOVE,
/*				        PK_TASK_ACTION_UPDATE,*/
				        PK_TASK_ACTION_GET_UPDATES,
				        PK_TASK_ACTION_REFRESH_CACHE,
/*				        PK_TASK_ACTION_UPDATE_SYSTEM,*/
				        PK_TASK_ACTION_SEARCH_NAME,
/*				        PK_TASK_ACTION_SEARCH_DETAILS,*/
/*				        PK_TASK_ACTION_SEARCH_GROUP,*/
/*				        PK_TASK_ACTION_SEARCH_FILE,*/
/*				        PK_TASK_ACTION_GET_DEPENDS,*/
/*				        PK_TASK_ACTION_GET_REQUIRES,*/
				        PK_TASK_ACTION_GET_DESCRIPTION,
				        0);
	return actions;
}

/**
 * pk_task_get_updates:
 **/
gboolean
pk_task_get_updates (PkTask *task)
{
	g_return_val_if_fail (task != NULL, FALSE);
	g_return_val_if_fail (PK_IS_TASK (task), FALSE);

	if (pk_task_assign (task) == FALSE) {
		return FALSE;
	}

	pk_task_set_job_role (task, PK_TASK_ROLE_QUERY, NULL);
	pk_task_spawn_helper (task, "get-updates.py", NULL);
	return TRUE;
}

/**
 * pk_task_refresh_cache:
 **/
gboolean
pk_task_refresh_cache (PkTask *task, gboolean force)
{
	g_return_val_if_fail (task != NULL, FALSE);
	g_return_val_if_fail (PK_IS_TASK (task), FALSE);

	if (pk_task_assign (task) == FALSE) {
		return FALSE;
	}

	/* check network state */
	if (pk_network_is_online (task->priv->network) == FALSE) {
		pk_task_error_code (task, PK_TASK_ERROR_CODE_NO_NETWORK, "Cannot refresh cache whilst offline");
		pk_task_finished (task, PK_TASK_EXIT_FAILED);
		return TRUE;
	}

	/* easy as that */
	pk_task_set_job_role (task, PK_TASK_ROLE_REFRESH_CACHE, NULL);
	pk_task_spawn_helper (task, "refresh-cache.py", NULL);

	return TRUE;
}

/**
 * pk_task_update_system:
 **/
gboolean
pk_task_update_system (PkTask *task)
{
	g_return_val_if_fail (task != NULL, FALSE);
	g_return_val_if_fail (PK_IS_TASK (task), FALSE);

	if (pk_task_assign (task) == FALSE) {
		return FALSE;
	}

	pk_task_set_job_role (task, PK_TASK_ROLE_SYSTEM_UPDATE, NULL);
	pk_task_not_implemented_yet (task, "UpdateSystem");
	return TRUE;
}

/**
 * pk_task_search_name:
 **/
gboolean
pk_task_search_name (PkTask *task, const gchar *filter, const gchar *search)
{
	g_return_val_if_fail (task != NULL, FALSE);
	g_return_val_if_fail (PK_IS_TASK (task), FALSE);

	if (pk_task_assign (task) == FALSE) {
		return FALSE;
	}

	if (pk_task_filter_check (filter) == FALSE) {
		pk_task_error_code (task, PK_TASK_ERROR_CODE_FILTER_INVALID, "filter '%s' not valid", filter);
		pk_task_finished (task, PK_TASK_EXIT_FAILED);
		return TRUE;
	}

	pk_task_no_percentage_updates (task);
	pk_task_change_job_status (task, PK_TASK_STATUS_QUERY);
	pk_task_spawn_helper (task, "search-name.py", filter, search);
	return TRUE;
}

/**
 * pk_task_search_details:
 **/
gboolean
pk_task_search_details (PkTask *task, const gchar *filter, const gchar *search)
{
	g_return_val_if_fail (task != NULL, FALSE);
	g_return_val_if_fail (PK_IS_TASK (task), FALSE);

	if (pk_task_assign (task) == FALSE) {
		return FALSE;
	}

	if (pk_task_filter_check (filter) == FALSE) {
		pk_task_error_code (task, PK_TASK_ERROR_CODE_FILTER_INVALID, "filter '%s' not valid", filter);
		pk_task_finished (task, PK_TASK_EXIT_FAILED);
		return TRUE;
	}

	pk_task_set_job_role (task, PK_TASK_ROLE_QUERY, search);
	pk_task_spawn_helper (task, "search-details.py", filter, search, NULL);
	return TRUE;
}

/**
 * pk_task_search_group:
 **/
gboolean
pk_task_search_group (PkTask *task, const gchar *filter, const gchar *search)
{
	pk_task_set_job_role (task, PK_TASK_ROLE_QUERY, search);
	pk_task_not_implemented_yet (task, "SearchGroup");
	return TRUE;
}

/**
 * pk_task_search_file:
 **/
gboolean
pk_task_search_file (PkTask *task, const gchar *filter, const gchar *search)
{
	pk_task_set_job_role (task, PK_TASK_ROLE_QUERY, search);
	pk_task_not_implemented_yet (task, "SearchFile");
	return TRUE;
}

/**
 * pk_task_get_depends:
 **/
gboolean
pk_task_get_depends (PkTask *task, const gchar *package_id)
{
	g_return_val_if_fail (task != NULL, FALSE);
	g_return_val_if_fail (PK_IS_TASK (task), FALSE);

	if (pk_task_assign (task) == FALSE) {
		return FALSE;
	}

	pk_task_set_job_role (task, PK_TASK_ROLE_QUERY, package_id);
	pk_task_not_implemented_yet (task, "GetDepends");
	return TRUE;
}

/**
 * pk_task_get_requires:
 **/
gboolean
pk_task_get_requires (PkTask *task, const gchar *package_id)
{
	g_return_val_if_fail (task != NULL, FALSE);
	g_return_val_if_fail (PK_IS_TASK (task), FALSE);

	if (pk_task_assign (task) == FALSE) {
		return FALSE;
	}

	pk_task_set_job_role (task, PK_TASK_ROLE_QUERY, package_id);
	pk_task_not_implemented_yet (task, "GetRequires");
	return TRUE;
}

/**
 * pk_task_get_description:
 **/
gboolean
pk_task_get_description (PkTask *task, const gchar *package_id)
{
	g_return_val_if_fail (task != NULL, FALSE);
	g_return_val_if_fail (PK_IS_TASK (task), FALSE);

	if (pk_task_assign (task) == FALSE) {
		return FALSE;
	}

	/* only copy this code if you can kill the process with no ill effect */
	pk_task_allow_interrupt (task, TRUE);

	pk_task_set_job_role (task, PK_TASK_ROLE_QUERY, package_id);
	pk_task_spawn_helper (task, "get-description.py", package_id, NULL);
	return TRUE;
}

/**
 * pk_task_remove_package:
 **/
gboolean
pk_task_remove_package (PkTask *task, const gchar *package_id, gboolean allow_deps)
{
	const gchar *deps;
	g_return_val_if_fail (task != NULL, FALSE);
	g_return_val_if_fail (PK_IS_TASK (task), FALSE);

	if (pk_task_assign (task) == FALSE) {
		return FALSE;
	}

	if (allow_deps == TRUE) {
		deps = "yes";
	} else {
		deps = "no";
	}

	pk_task_set_job_role (task, PK_TASK_ROLE_PACKAGE_REMOVE, package_id);
	pk_task_spawn_helper (task, "remove.py", deps, package_id, NULL);
	return TRUE;
}
/**
 * pk_task_install_package:
 **/
gboolean
pk_task_install_package (PkTask *task, const gchar *package_id)
{
	g_return_val_if_fail (task != NULL, FALSE);
	g_return_val_if_fail (PK_IS_TASK (task), FALSE);

	if (pk_task_assign (task) == FALSE) {
		return FALSE;
	}

	/* check network state */
	if (pk_network_is_online (task->priv->network) == FALSE) {
		pk_task_error_code (task, PK_TASK_ERROR_CODE_NO_NETWORK, "Cannot install when offline");
		pk_task_finished (task, PK_TASK_EXIT_FAILED);
		return TRUE;
	}

	pk_task_set_job_role (task, PK_TASK_ROLE_PACKAGE_INSTALL, package_id);
	pk_task_spawn_helper (task, "install.py", package_id, NULL);
	return TRUE;
}

/**
 * pk_task_update_package:
 **/
gboolean
pk_task_update_package (PkTask *task, const gchar *package_id)
{
	g_return_val_if_fail (task != NULL, FALSE);
	g_return_val_if_fail (PK_IS_TASK (task), FALSE);

	if (pk_task_assign (task) == FALSE) {
		return FALSE;
	}

	/* check network state */
	if (pk_network_is_online (task->priv->network) == FALSE) {
		pk_task_error_code (task, PK_TASK_ERROR_CODE_NO_NETWORK, "Cannot update when offline");
		pk_task_finished (task, PK_TASK_EXIT_FAILED);
		return TRUE;
	}

	pk_task_set_job_role (task, PK_TASK_ROLE_PACKAGE_UPDATE, package_id);
	pk_task_not_implemented_yet (task, "UpdatePackage");
	return TRUE;
}

/**
 * pk_task_cancel_job_try:
 **/
gboolean
pk_task_cancel_job_try (PkTask *task)
{
	g_return_val_if_fail (task != NULL, FALSE);
	g_return_val_if_fail (PK_IS_TASK (task), FALSE);

	/* check to see if we have an action */
	if (task->assigned == FALSE) {
		pk_warning ("Not assigned");
		return FALSE;
	}

	pk_task_not_implemented_yet (task, "CancelJobTry");
	return TRUE;
}

/**
 * pk_task_class_init:
 **/
static void
pk_task_class_init (PkTaskClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = pk_task_finalize;
	pk_task_setup_signals (object_class, signals);
	g_type_class_add_private (klass, sizeof (PkTaskPrivate));
}

/**
 * pk_task_init:
 **/
static void
pk_task_init (PkTask *task)
{
	task->priv = PK_TASK_GET_PRIVATE (task);
	task->signals = signals;
	task->priv->network = pk_network_new ();
}

/**
 * pk_task_finalize:
 **/
static void
pk_task_finalize (GObject *object)
{
	PkTask *task;
	g_return_if_fail (object != NULL);
	g_return_if_fail (PK_IS_TASK (object));
	task = PK_TASK (object);
	g_return_if_fail (task->priv != NULL);
	g_object_unref (task->priv->network);
	G_OBJECT_CLASS (pk_task_parent_class)->finalize (object);
}

/**
 * pk_task_new:
 **/
PkTask *
pk_task_new (void)
{
	PkTask *task;
	task = g_object_new (PK_TYPE_TASK, NULL);
	return PK_TASK (task);
}

