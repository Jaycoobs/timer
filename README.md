# Timer

Tiny project for timing speedruns.

## Features

- [x] ~1000 SLOC
- [x] Runs in ANSI terminals
- [x] Tracks PB
- [x] Tracks gold splits
- [x] Tracks attempts
- [ ] Global Hotkeys (probly never)

## Keybinds

These are not global.

|  Key   | Action                 |
|--------|------------------------|
| space  | split                  |
| escape | pause/reset            |
| j      | skip                   |
| k      | undo                   |
| w      | save run (when paused) |
| q      | quit                   |

## Data Format

### Category Directory

Each category gets a directory. Here's an example:

```
$ tree mario16/

mario16/
├── attempts
├── golds
├── pb
└── splits
```

### Splits File

The `splits` file is a list of segments, each on a new line...

```
bob
wf
ccm
dw
ssl
lll
hmc
mips
ddd
fs
bljs
bits
```

### Run Files

The `pb` file looks like this...

```
bob                        1:41.626       1:41.626
wf                         3:15.727       4:57.353
ccm                        1:24.941       6:22.295
dw                         2:19.524       8:41.819
ssl                        1:26.993      10:08.813
lll                          35.454      10:44.268
hmc                        1:23.246      12:07.514
mips                         55.745      13:03.259
ddd                          51.175      13:54.434
fs                         1:40.466      15:34.900
bljs                         58.901      16:33.802
bits                       2:48.225      19:22.028
```

Other runs that you save will also appear like this.

### Golds File

The `golds` file looks like this...

```
bob                        1:41.626
wf                         3:15.727
ccm                        1:24.941
dw                         2:02.396
ssl                        1:26.993
lll                          35.454
hmc                        1:21.756
mips                         55.745
ddd                          47.868
fs                         1:40.466
bljs                         58.901
bits                       2:48.225
```

### Attempts File

The `attempts` file looks like this...

```
bob                      1     7
wf                       1     6
ccm                      0     5
dw                       3     5
ssl                      0     2
lll                      0     2
hmc                      0     2
mips                     0     2
ddd                      0     2
fs                       0     2
bljs                     1     2
bits                     0     1
completed                0     1
```

The first column is the split name, the second is the number of resets
on that split, and the third is the number of runs which have made it to
that split before being reset.

## Running

To start running, all you need to do is make a category directory, place
a splits file in it with a list of segments and run...

```
$ timer <category dir>
```

## Compiling

```
make timer
```
