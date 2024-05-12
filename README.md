
# gera-cJSON

Bindings for [cJSON](https://github.com/DaveGamble/cJSON), an "ultralightweight JSON parser in ANSI C".
This package is only available for the `c` target, but gives great speed in return.
For other targets consider using [gera-json](https://github.com/typesafeschwalbe/gera-json), which is written in Gera instead (and therefore supports all targets).

Null characters are sadly not supported in any way, since cJSON does not support them either.