/*
 * TODO
 * Copyright (C) 2018  zerons
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
/*
 * find appropriate module and give comment
 */

#include "si_core.h"

static char *show_cmdname = "show";
static void show_usage(void)
{
	fprintf(stdout, "\t(type) [path]\n"
			"\ttype: (SRC/BIN)_(KERN/USER)_(LINUX/...)_(...) "
			"check si_core.h for more\n"
			"\tPick appropriate module and show comment\n");
}

static char *target_object = NULL;
static char *expand_comment(struct si_module *p)
{
	/* we search for ${ in the string */
	char *ret, *tmp;
	size_t len = 0;
	char *pos_s = p->comment, *pos_e;
	while (*pos_s) {
		pos_e = strstr(pos_s, "${");
		if (!pos_e) {
			len += strlen(pos_s) + 1;
			break;
		}

		len += pos_e - pos_s;

		if (!strncasecmp(pos_e+2, "SOPATH", 6)) {
			len += strlen(p->path);
		} else if (!strncasecmp(pos_e+2, "RESFILE", 7)) {
			len += strlen(DEF_TMPDIR) + strlen(si->src_id) + 2 + 7;
		} else if (!strncasecmp(pos_e+2, "TPROJECT", 8)) {
			if (target_object)
				len += strlen(target_object);
		} else {
			err_dbg(0, "unknown expand string");
			return NULL;
		}

		pos_e = strchr(pos_e, '}');
		if (!pos_e) {
			err_dbg(0, "expand string format err");
			return NULL;
		}
		pos_s = pos_e + 1;
	}

	ret = malloc(len);
	if (!ret) {
		err_dbg(0, "malloc err");
		return NULL;
	}
	tmp = ret;
	pos_s = p->comment;
	while (*pos_s) {
		pos_e = strstr(pos_s, "${");
		if (!pos_e) {
			memcpy(tmp, pos_s, strlen(pos_s)+1);
			break;
		}

		memcpy(tmp, pos_s, pos_e - pos_s);
		tmp += pos_e - pos_s;

		if (!strncasecmp(pos_e+2, "SOPATH", 6)) {
			memcpy(tmp, p->path, strlen(p->path));
			tmp += strlen(p->path);
		} else if (!strncasecmp(pos_e+2, "RESFILE", 7)) {
			sprintf(tmp, "%s/%s/resfile", DEF_TMPDIR, si->src_id);
			tmp += strlen(DEF_TMPDIR) + strlen(si->src_id) + 2 + 7;
		} else if (!strncasecmp(pos_e+2, "TPROJECT", 8)) {
			if (target_object) {
				sprintf(tmp, "%s", target_object);
				tmp += strlen(target_object);
			}
		}

		pos_e = strchr(pos_e, '}');
		pos_s = pos_e + 1;
	}

	return ret;
}

C_SYM int si_module_str_to_type(struct si_type *type, char *string);
static long show_cb(int argc, char *argv[])
{
	if (unlikely(!si)) {
		err_dbg(0, "si not set yetn");
		return -1;
	}

	if (unlikely((argc != 2) && (argc != 3))) {
		err_dbg(0, "argc invalid");
		show_usage();
		return -1;
	}

	int err;
	char *given_type = argv[1];
	char *given_path = argv[2];
	struct si_type type;
	err = si_module_str_to_type(&type, given_type);
	if (err) {
		err_dbg(0, "si_module_str_to_type err");
		return -1;
	}

	struct si_module **mods;
	struct list_head *head;
	head = si_module_get_head(SI_PLUGIN_CATEGORY_COLLECT);
	if (!head) {
		err_dbg(0, "si_module_get_head err");
		return -1;
	}

	mods = si_module_find_by_type(&type, head);
	if (!mods) {
		err_dbg(0, "si_module_find_by_type err");
		return -1;
	}

	int i = 0;
	fprintf(stdout, "several modules found:\n");
	if (given_path)
		target_object = given_path;
	while (mods[i]) {
		char *real_comment = expand_comment(mods[i]);
		if (!real_comment) {
			err_dbg(0, "expand_comment err");
			return -1;
		}

		fprintf(stdout, "%s:\t%s\n", mods[i]->name, real_comment);
		free(real_comment);
		i++;
	}
	target_object = NULL;

	free(mods);
	return 0;
}

SI_MOD_SUBENV_INIT()
{
	int err = clib_cmd_ac_add(show_cmdname, show_cb, show_usage);
	if (err) {
		err_dbg(0, "clib_cmd_ac_add err");
		return -1;
	}
	return 0;
}

SI_MOD_SUBENV_DEINIT()
{
	return;
}

SI_MOD_SUBENV_SETUP(collect);
