
# opts.h

opts.h is a simple and powerful option parser library written in C99, on the top of [optparse](https://github.com/skeeto/optparse) portable and reentrant parser library.

## Features

* gnu-style options (inheriting the feature of the optparse library)
* short and long option keys, and its description can be written in one place
* arguments can be parsed as int, char, string, and bool with user-defined destination variables
* default values and postprocess callbacks (for restoration of default values)
* auto-generation of description message (with invisible and hidden states)
* hierarchical option grouping
* positional arguments with descriptions

## Example

```
int main(int argc, char *argv[])
{
	int help = 0, version = 0;
	char *file1 = NULL, *file2 = NULL, *out = NULL;

	struct opts_params_s const *opts = OPTS_PARAMS(
		.fprintf = fprintf,
		.fp = stderr,
		.header = "This is a header of the help message",
		.table = OPTS_TABLE(
			OPTS_GROUP("Global options",
				/* when variable is passed with OPTS_BOOL macro, the option is interpreted as NO_ARGUMENT one and the variable is set to 1 when specified */
				/* postprocess callbacks are skipped when one of the SKIP_POST tagged options is specified */
				{ 'h', "help", "print help (this) message", OPTS_BOOL(help), OPTS_SKIP_POST },
				{ 'v', "version", "print help (this) message", OPTS_BOOL(version), OPTS_SKIP_POST }
			),
			OPTS_GROUP("I/O options",
				/* positional arguments are expressed with ascending indices from 0 (<32) */
				{ 0, "file1", "first input file name", OPTS_STR(file1) },
				{ 1, "file2", "second input file name", OPTS_STR(file2), OPTS_ADDITIONAL },
				{ 'o', "out", "output file name", OPTS_STR(out) }
			)
		),
		.footer = "This is a footer of the help message"),
	opts_parse(opts, argc, argv);

	if(help != 0) {
		opts_print_desc(opts);
	}

	/* ... */
	return(0);
}
```

## License

MIT
(2016) Hajime Suzuki

