## this software is

Suspend PC Timer for Windows.

command `sustimer.exe 120` to suspend after 120 seconds.

## how to compile

use [gcc](http://gcc.gnu.org/) on command line.

```
mingw32-make
```

or

```
gcc *.c -mwindows -lole32 -lpowrprof
```

## another compile

### Visual Studio

```
nmake
```

or

```
cl *.c /MD /link /ENTRY:__start__ kernel32.lib shell32.lib user32.lib gdi32.lib ole32.lib powrprof.lib
```
