# cbc: the cursed bean compiler

NOTE: you can also call it the C bean compilier.

I'm tired of a slow interpreted `beancode`. Time to take it a step further.

## compiling

We use mate.h, which is a C build system written in C.

To make a build script that builds in debug mode:
`cc -DDEBUG -o mate mate.c`

For release mode:
`cc -o mate mate.c`

And to compile:
`./mate`

Which will build an executable.
