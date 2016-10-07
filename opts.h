
/**
 * @file opts.h
 *
 * @brief option parser, an wrapper of optarg.h
 */
#ifndef _OPTS_H_INCLUDED
#define _OPTS_H_INCLUDED


#include <ctype.h>
// #include <getopt.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "optparse.h"	/* use optparse (https://github.com/skeeto/optparse) instead of getopt.h */
#include "kvec.h"


/**
 * @type opts_fprintf_t
 */
typedef int (*opts_fprintf_t)(void *fp, char const *fmt, ...);

/**
 * @type opts_store_t
 */
typedef void (*opts_store_t)(void *, char const *);

/**
 * @type opts_print_t
 */
typedef int (*opts_print_t)(void *, opts_fprintf_t, void *);

/**
 * @type opts_post_t
 */
typedef int (*opts_post_t)(void *base, void *ptr, int updated);

/**
 * @struct opts_item_s
 */
struct opts_item_s {
	/* key and description */
	int sopt;			/* index in positional argument */
	char const *lopt;	/* name in positional argument */
	char const *desc;	/* NULL as terminator */

	/* actions */
	void *ptr;			/* destination variable pointer [NULL] */
	char mode;			/* { 'R', 'N', 'O' } ['N'] */
	char type;			/* { 'i', 'c', 's', 'f', 'b' } ['i'] */
	char size;			/* { 1, 2, 4, 8 } [8] */
	opts_store_t store;
	opts_print_t print;
	opts_post_t post;

	/* internal use */
	char hidden;		/* 1: appears in verbose mode, 2: never shown */
	char hide_default;	/* 1: default value will not be printed */
	char additional;	/* 1: non-mandatory positional argument */
	char skip;			/* 1: skip positional argument parsing and postprocess if the option is specified (to implement --help and --version) */
	int item;			/* 0: option, 1: positional argument, 2: nested block */
};
#define OPTS_TABLE(...)			( (struct opts_item_s const []){ __VA_ARGS__, { 0 } } )
#define OPTS_GROUP(_desc, ...)	( (struct opts_item_s const){ .item = 2, .desc = (_desc), .ptr = (void *)OPTS_TABLE(__VA_ARGS__) } )
#define OPTS_TEXT(_text)		( (struct opts_item_s const){ .item = 3, .desc = (_text) } )

/* misc macros */
#define OPTS_VAR_INTL(_mode, _type, _var, _def, _store, _print, _post, ...)	\
	((_var) = (_def), &(_var)), (_mode), (_type), \
	sizeof(_var), (void *)(_store), (void *)(_print), (void *)(_post)
#define OPTS_INT(...)			OPTS_VAR_INTL('R', 'i', __VA_ARGS__, 0, 0, 0, 0)
#define OPTS_CHAR(...)			OPTS_VAR_INTL('R', 'c', __VA_ARGS__, 0, 0, 0, 0)
#define OPTS_FLOAT(...)			OPTS_VAR_INTL('R', 'f', __VA_ARGS__, 0, 0, 0, 0)
#define OPTS_STR(...)			OPTS_VAR_INTL('R', 's', __VA_ARGS__, 0, 0, 0, 0)
#define OPTS_BOOL(...)			OPTS_VAR_INTL('N', 'b', __VA_ARGS__, 0, 0, 0, 0)

#define OPTS_HIDDEN				.hidden = 1
#define OPTS_INVISIBLE			.hidden = 2
#define OPTS_HIDE_DEFAULT		.hide_default = 1
#define OPTS_ADDITIONAL			.additional = 1
#define OPTS_SKIP_POST			.skip = 1


/**
 * @struct opts_params_s
 */
struct opts_params_s {
	/* message printer */
	char verbose;				/* 0: default, 1: verbose */
	opts_fprintf_t fprintf;
	void *fp;
	char const *header;			/* header in help message */
	char const *footer;			/* footer in help message */

	/* object */
	void *base;

	/* configuration */	
	struct opts_item_s const *table;	/* option table */
};
#define OPTS_PARAMS(...)		( &(struct opts_params_s const){ __VA_ARGS__ } )

/**
 * @type opts_option_v
 */
typedef kvec_t(struct optparse_long) opts_option_v;

/**
 * @struct opts_map_s
 */
struct opts_map_s {
	int sopt;					/* -1 as terminator */
	int updated;				/* number of option specified */
	int additional;				/* 1: non-mandatory positional argument */
	int skip;
	char const *lopt;
	void *ptr;					/* pointer to variable */
	opts_store_t store;
	opts_post_t post;
};

/**
 * @type opts_map_v
 */
typedef kvec_t(struct opts_map_s) opts_map_v;

/**
 * @fn opts_get_argtype
 */
static inline
int opts_get_argtype(
	char mode)
{
	mode = toupper(mode);
	if(mode == 'R') {
		return(OPTPARSE_REQUIRED);
	} else if(mode == 'O') {
		return(OPTPARSE_OPTIONAL);
	}
	return(OPTPARSE_NONE);
}

/**
 * @fn opts_build_long_option_rec
 */
static inline
void opts_build_long_option_rec(
	struct opts_item_s const *p,
	opts_option_v *v)
{
	if(p == NULL) {
		return;
	}

	for(struct opts_item_s const *o = p; o->desc != NULL; o++) {
		if(o->item != 0) {
			opts_build_long_option_rec((struct opts_item_s const *)o->ptr, v);
		} else {
			kv_push(*v, ((struct optparse_long){
				.longname = o->lopt,
				.shortname = o->sopt,
				.argtype = opts_get_argtype(o->mode)
			}));
		}
	}
	return;
}

/**
 * @fn opts_build_long_option
 */
static inline
struct optparse_long *opts_build_long_option(
	struct opts_item_s const *params)
{
	opts_option_v v;
	kv_init(v);

	opts_build_long_option_rec(params, &v);
	kv_push(v, ((struct optparse_long){ 0 }));
	return(kv_ptr(v));
}

/**
 * @fn opts_atoi
 */
static inline
int64_t opts_atoi_prefix(
	char const *val)
{
	if(*val == 'x') {
		return(strtoll(++val, NULL, 16));
	} else if(*val == 'd') {
		return(strtoll(++val, NULL, 10));
	} else if(*val == 'b') {
		return(strtoll(++val, NULL, 2));
	}
	return(strtoll(val, NULL, 8));
}
static inline
int64_t opts_atoi_dec(
	char const *val)
{
	int64_t len = strlen(val);
	int64_t mul = 1, base = 1000;
	if(isdigit(val[len - 1])) {
		return(strtoll(val, NULL, 10));
	}
	do {
		switch(val[len - 1]) {
			case 'i': base = 1024; len--; continue;
			case 'T': mul *= base;
			case 'G': mul *= base;
			case 'M': mul *= base;
			case 'K':
			case 'k': mul *= base;
			default:;
				char tmp[len];
				memcpy(tmp, val, len - 1);
				tmp[len - 1] = '\0';
				return(strtoll(tmp, NULL, 10));
		}
	} while(0);
	return(0);
}
static inline
int64_t opts_atoi(
	char const *val)
{
	if(*val == '0') {
		return(opts_atoi_prefix(val + 1));
	} else {
		return(opts_atoi_dec(val));
	}
}

/**
 * @fn opts_store_i*
 */
static
void opts_store_i8(
	void *ptr,
	char const *arg)
{
	if(ptr == NULL) { return; }
	int8_t *p = (int8_t *)ptr;
	*p = (arg == NULL) ? 1 : opts_atoi(arg);
	return;
}
static
void opts_store_i16(
	void *ptr,
	char const *arg)
{
	if(ptr == NULL) { return; }
	int16_t *p = (int16_t *)ptr;
	*p = (arg == NULL) ? 1 : opts_atoi(arg);
	return;
}
static
void opts_store_i32(
	void *ptr,
	char const *arg)
{
	if(ptr == NULL) { return; }
	int32_t *p = (int32_t *)ptr;
	*p = (arg == NULL) ? 1 : opts_atoi(arg);
	return;
}
static
void opts_store_i64(
	void *ptr,
	char const *arg)
{
	if(ptr == NULL) { return; }
	int64_t *p = (int64_t *)ptr;
	*p = (arg == NULL) ? 1 : opts_atoi(arg);
	return;
}

/**
 * @fn opts_store_f32
 */
static
void opts_store_f32(
	void *ptr,
	char const *arg)
{
	if(ptr == NULL || arg == NULL) { return; }
	float *p = (float *)ptr;
	*p = atof(arg);
	return;
}
static
void opts_store_f64(
	void *ptr,
	char const *arg)
{
	if(ptr == NULL || arg == NULL) { return; }
	double *p = (double *)ptr;
	*p = atof(arg);
	return;
}

/**
 * @fn opts_store_str
 */
static
void opts_store_str(
	void *ptr,
	char const *arg)
{
	if(ptr == NULL || arg == NULL) { return; }
	char **p = (char **)ptr;
	if(*p != NULL) {
		free(*p);
	}
	*p = strdup(arg);
	return;
}

/**
 * @fn opts_store_char
 */
static
void opts_store_char(
	void *ptr,
	char const *arg)
{
	if(ptr == NULL || arg == NULL) { return; }
	char *p = (char *)ptr;
	*p = arg[0];
	return;
}

/**
 * @fn opts_store_b*
 */
static
void opts_store_b8(
	void *ptr,
	char const *arg)
{
	if(ptr == NULL) { return; }
	(*((int8_t *)ptr))++;
	return;
}
static
void opts_store_b16(
	void *ptr,
	char const *arg)
{
	if(ptr == NULL) { return; }
	(*((int16_t *)ptr))++;
	return;
}
static
void opts_store_b32(
	void *ptr,
	char const *arg)
{
	if(ptr == NULL) { return; }
	(*((int32_t *)ptr))++;
	return;
}
static
void opts_store_b64(
	void *ptr,
	char const *arg)
{
	if(ptr == NULL) { return; }
	(*((int64_t *)ptr))++;
	return;
}

/**
 * @fn opts_get_store
 */
static inline
opts_store_t opts_get_store(
	char type,
	char size)
{
	type = tolower(type);
	if(type == 'i') {
		if(size == 1) {
			return(opts_store_i8);
		} else if(size == 2) {
			return(opts_store_i16);
		} else if(size == 4) {
			return(opts_store_i32);
		} else if(size == 8) {
			return(opts_store_i64);
		}
	} else if(type == 'b') {
		if(size == 1) {
			return(opts_store_b8);
		} else if(size == 2) {
			return(opts_store_b16);
		} else if(size == 4) {
			return(opts_store_b32);
		} else if(size == 8) {
			return(opts_store_b64);
		}
	} else if(type == 's') {
		return(opts_store_str);
	} else if(type == 'c') {
		return(opts_store_char);
	} else if(type == 'f') {
		if(size == 4) {
			return(opts_store_f32);
		} else if(size == 8) {
			return(opts_store_f64);
		}
	}
	return(NULL);
}

/**
 * @fn opts_build_map_rec
 */
static inline
int opts_build_map_rec(
	struct opts_item_s const *p,
	opts_map_v *v,
	int max)
{
	if(p == NULL) {
		return(max);
	}

	for(struct opts_item_s const *o = p; o->desc != NULL; o++) {
		if(o->item != 0) {
			max = opts_build_map_rec((struct opts_item_s const *)o->ptr, v, max);
		} else {
			if(o->sopt > max) {
				max = o->sopt;
			}
			kv_push(*v, ((struct opts_map_s){
				.sopt = o->sopt,
				.lopt = o->lopt,
				.additional = o->additional,
				.skip = o->skip,
				.ptr = o->ptr,
				.store = (o->store != NULL)
					? o->store
					: opts_get_store(o->type, o->size),
				.post = o->post
			}));
		}
	}
	return(max);
}

/**
 * @fn opts_build_map
 */
static inline
struct opts_map_s *opts_build_map(
	struct opts_item_s const *params)
{
	opts_map_v v;
	kv_init(v);

	/* append all the entries to vector */
	int max_idx = opts_build_map_rec(params, &v, 127);

	/* build mapping table */
	struct opts_map_s *p = malloc(sizeof(struct opts_map_s) * (max_idx + 2));
	for(int i = 0; i < max_idx + 1; i++) {
		p[i] = (struct opts_map_s){ .sopt = -1 };
	}
	for(int i = 0; i < kv_size(v); i++) {
		p[kv_at(v, i).sopt] = kv_at(v, i);
	}
	p[max_idx + 1] = (struct opts_map_s){ .sopt = -2 };
	kv_destroy(v);
	return(p);
}

/**
 * @fn opts_parse
 */
static inline
int opts_parse(
	struct opts_params_s const *params,
	int argc,
	char *argv[])
{
	/* transparent on invalid params and argv */
	if(params == NULL || argc < 2) {
		return(0);
	}

	int ret = 0;
	opts_fprintf_t _fprintf = (params->fprintf == NULL)
		? (opts_fprintf_t)fprintf
		: params->fprintf;
	void *fp = (_fprintf == (opts_fprintf_t)fprintf && params->fp == NULL)
		? stderr
		: params->fp;

	/* parse */
	struct optparse options;
    optparse_init(&options, argv);
    struct optparse_long *opts_long = opts_build_long_option(params->table);
	struct opts_map_s *map = opts_build_map(params->table);

	int c, skip = 0;
	while((c = optparse_long(&options, opts_long, NULL)) != -1) {
		if(c == '?') {
			_fprintf(fp, "[warning] %s\n", options.errmsg);
			continue;
		}

		if(map[c].sopt < 0 || map[c].store == NULL) { continue; }
		map[c].store(map[c].ptr, options.optarg);
		map[c].updated++;
		skip += map[c].skip;
	}

	if(skip > 0) {
		return(ret);
	}

	/* positional arguments */
	for(int i = 0; map[i].sopt > -1; i++) {
		char *arg = optparse_arg(&options);
		if(arg == NULL) { break; }

		if(map[i].store != NULL) {
			map[i].store(map[i].ptr, arg);
			map[i].updated++;
		}
	}

	if(optparse_arg(&options) != NULL) {
		_fprintf(fp, "[ERROR] Too many positional arguments.\n");
		ret = 1;
		goto _opts_parse_error_handler;
	}

	/* postprocess */
	for(int i = 0; map[i].sopt > -2; i++) {
		if(map[i].post == NULL) { continue; }
		map[i].post(params->base, map[i].ptr, map[i].updated);
		map[i].updated++;
	}

	/* check positional arguments */
	for(int i = 0; map[i].sopt > -1; i++) {
		if(map[i].additional == 0 && map[i].updated == 0) {
			_fprintf(fp, "[ERROR] %d%s positional argument %s%s%s is required.\n",
				i + 1,
				(char const *[]){ "st", "nd", "rd", "th" }[i > 3 ? 3 : i],
				map[i].lopt == NULL ? "" : "<",
				map[i].lopt == NULL ? "" : map[i].lopt,
				map[i].lopt == NULL ? "" : ">");
			ret = 1;
		}
	}

_opts_parse_error_handler:;
	free(opts_long);
	free(map);
	return(ret);
}


/**
 * @fn opts_clean_rec
 */
static inline
void opts_clean_rec(
	struct opts_item_s const *p)
{
	if(p == NULL) {
		return;
	}

	for(struct opts_item_s const *o = p; o->desc != NULL; o++) {
		if(o->item != 0) {
			opts_clean_rec((struct opts_item_s const *)o->ptr);
		} else {
			if(tolower(o->type) == 's' && o->size == sizeof(char *)) {
				free(*((char **)o->ptr));
			}
		}
	}
	return;
}

/**
 * @fn opts_clean
 */
static inline
void opts_clean(
	struct opts_params_s const *p)
{
	opts_clean_rec(p->table);
	return;
}


/**
 * @fn opts_print_i*
 */
static
int opts_print_i8(
	void *ptr,
	opts_fprintf_t _fprintf,
	void *fp)
{
	if(ptr == NULL) { return(0); }
	int8_t *p = (int8_t *)ptr;
	return(_fprintf(fp, "[%d]", *p));
}
static
int opts_print_i16(
	void *ptr,
	opts_fprintf_t _fprintf,
	void *fp)
{
	if(ptr == NULL) { return(0); }
	int16_t *p = (int16_t *)ptr;
	return(_fprintf(fp, "[%d]", *p));
}
static
int opts_print_i32(
	void *ptr,
	opts_fprintf_t _fprintf,
	void *fp)
{
	if(ptr == NULL) { return(0); }
	int32_t *p = (int32_t *)ptr;
	return(_fprintf(fp, "[%d]", *p));
}
static
int opts_print_i64(
	void *ptr,
	opts_fprintf_t _fprintf,
	void *fp)
{
	if(ptr == NULL) { return(0); }
	int64_t *p = (int64_t *)ptr;
	return(_fprintf(fp, "[%" PRId64 "]", *p));
}

/**
 * @fn opts_print_f32
 */
static
int opts_print_f32(
	void *ptr,
	opts_fprintf_t _fprintf,
	void *fp)
{
	if(ptr == NULL) { return(0); }
	float *p = (float *)ptr;
	return(_fprintf(fp, "[%f]", *p));
}
static
int opts_print_f64(
	void *ptr,
	opts_fprintf_t _fprintf,
	void *fp)
{
	if(ptr == NULL) { return(0); }
	double *p = (double *)ptr;
	return(_fprintf(fp, "[%f]", *p));
}

/**
 * @fn opts_print_str
 */
static
int opts_print_str(
	void *ptr,
	opts_fprintf_t _fprintf,
	void *fp)
{
	if(ptr == NULL) { return(0); }
	char **p = (char **)ptr;
	return(_fprintf(fp, "[%s]", *p));
}

/**
 * @fn opts_get_print
 */
static inline
opts_print_t opts_get_print(
	char type,
	char size)
{
	type = tolower(type);
	if(type == 'i') {
		if(size == 1) {
			return(opts_print_i8);
		} else if(size == 2) {
			return(opts_print_i16);
		} else if(size == 4) {
			return(opts_print_i32);
		} else if(size == 8) {
			return(opts_print_i64);
		}
	} else if(type == 's') {
		return(opts_print_str);
	} else if(type == 'f') {
		if(size == 4) {
			return(opts_print_f32);
		} else if(size == 8) {
			return(opts_print_f64);
		}
	}
	return(NULL);
}

/**
 * @fn opts_print_desc_rec
 */
static inline
void opts_print_desc_rec(
	struct opts_item_s const *p,
	opts_fprintf_t _fprintf,
	void *fp,
	char verbose,
	int depth,
	int tab_len)
{
	/* 40 spaces */
	static char const *pad = "                                        ";

	if(p == NULL || depth > strlen(pad) / 2) {
		return;
	}

	for(struct opts_item_s const *o = p; o->desc != NULL; o++) {
		if(o->hidden > verbose) { continue; }

		if(o->item == 0) {
			/* skip positional arguments */
			if(o->sopt < '0') { continue; }

			/* print option */
			char sopt[] = { '-', o->sopt, '\0' };
			int print_sopt = isprint(o->sopt);
			int print_lopt = (o->lopt != NULL) && (strlen(o->lopt) != 0);

			int len = _fprintf(fp, "%s%s%s%s%s",
				&pad[strlen(pad) - 2 * depth],
				print_sopt ? sopt : "",
				(print_sopt && print_lopt) ? ", " : "",
				print_lopt ? "--" : "",
				print_lopt ? o->lopt : "");

			/* print tab */
			_fprintf(fp, "%s", &pad[strlen(pad) - (tab_len - len)]);

			/* print default value */
			opts_print_t print = (o->print != NULL)
				? o->print
				: opts_get_print(o->type, o->size);
			if(print != NULL && o->hide_default == 0) {
				print(o->ptr, _fprintf, fp);
				_fprintf(fp, " ");
			}

			/* print description */
			_fprintf(fp, "%s\n", o->desc);

		} else if(o->item == 2) {
			/* group */
			_fprintf(fp, "\n%s%s\n", &pad[strlen(pad) - 2 * depth], o->desc);
			opts_print_desc_rec((struct opts_item_s const *)o->ptr,
				_fprintf, fp,
				verbose, depth + 1, tab_len);

		} else if(o->item == 3) {
			/* text */
			_fprintf(fp, "\n%s%s\n", &pad[strlen(pad) - 2 * depth], o->desc);
		}
	}
	return;
}

/**
 * @fn opts_print_desc
 */
static inline
void opts_print_desc(
	struct opts_params_s const *params)
{
	if(params == NULL) {
		return;
	}

	opts_fprintf_t _fprintf = (params->fprintf == NULL)
		? (opts_fprintf_t)fprintf
		: params->fprintf;
	void *fp = (_fprintf == (opts_fprintf_t)fprintf && params->fp == NULL)
		? stderr
		: params->fp;

	if(params->header != NULL) {
		_fprintf(fp, "%s\n", params->header);
	}

	int tab_len = 25;
	opts_print_desc_rec(params->table, _fprintf, fp, params->verbose, 1, tab_len);

	if(params->footer != NULL) {
		_fprintf(fp, "%s\n", params->footer);
	}
	return;
}

#endif
/**
 * end of opts.h
 */
