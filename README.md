# Timer

Tiny project for timing speedruns.

## Features

- [x] ~1000 SLOC
- [x] Runs in ANSI terminals
- [x] Tracks PB
- [ ] Tracks best splits (planned)
- [ ] Tracks attempt count (Soon)
- [ ] Global Hotkeys (not for me)

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

Each category gets a directory. Here's an example:

```
~/Projects/timer $ tree mario16

mario16/
├── pb
└── splits
```

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

The `pb` file looks like this...

```
bob                   1:40.723       1:40.723
wf                    3:27.458       5:08.181
ccm                   1:32.480       6:40.661
dw                    2:43.376       9:24.038
ssl                   1:31.997      10:56.035
lll                     37.531      11:33.566
hmc                   1:27.313      13:00.880
mips                  1:07.988      14:08.869
ddd                     44.665      14:53.534
fs                    1:45.703      16:39.238
bljs                    55.864      17:35.102
bits                  2:14.520      19:49.623
```

Other runs that you save will also appear like this.

To start running, all you need to do is make a category directory, place
a splits file in it with a list of segments and run...

```
$ timer <category dir>
```

## Compiling

```
make timer
```
