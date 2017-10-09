# sfserver
## Simple File Server

A local file server written in C for learning Inter Process Communications.

## Coding Style Suggestions

http://doc.cat-v.org/bell_labs/pikestyle

## Usage

```sh
sfs [-D directory_name] [-s]
    D: specify directory to serve (default is current directory).
    s: run in silent mode.
```

### Compiling and running

```sh
# to compile
$ make
# to run after compiling
$ bin/sfs
```

