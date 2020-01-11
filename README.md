## Freeware

Suspend PC Timer for Windows.

command `sustimer.exe 120` to suspend after 120 seconds.

* `sustimer 1:00:00` to 1 hours.
* `sustimer 3:33` to 3 minutes 33 seconds.

### command options

* `/a` to keep awaking PC while counting down.
* `/debug` to **no** suspend PC if timeout.

## how to compile

operating in command line.

### Visual Studio

```
nmake
```

or

```
cl *.c /MD /link /ENTRY:__start__ kernel32.lib shell32.lib user32.lib gdi32.lib ole32.lib powrprof.lib
```

### [gcc](http://gcc.gnu.org/)

no longer tried, in 2020 year.

```
mingw32-make
```

or

```
gcc *.c -mwindows -lole32 -lpowrprof
```
