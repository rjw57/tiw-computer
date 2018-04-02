# Tiw Computer

## Running emulator

```console
$ make emulator
$ ./emulator/mame64 -debug -window tiw -rs232a terminal
```

## Updating MAME

Warm up object store

```console
$ git fetch https://github.com/mamedev/mame master
```

Do pull

```console
$ git subtree pull -P emulator --squash https://github.com/mamedev/mame master
```
