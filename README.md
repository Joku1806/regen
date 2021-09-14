# regen

regen (*reg*ex *en*gine) is a library for matching text with regular expressions written in C.<br>
This is a one-man hobby project, there is no testing and bugs are regularly checked into main.<br>
You probably shouldn’t use this in your own projects.

## Syntax

regen supports the following operators and building blocks:

Symbol | Name | Example
-------|------|--------
`\` | Escape | `\\|` matches \| literally.
`\|` | Alternator | `a \| b \| c` matches either a, b, or c.
`(…)` | Group | `(a \| b)(c \| d)` matches either a or b followed by either c or d.
`[_, _]` | Character Range | `[a, z]` matches any character from a to z. (Parser only)
`{_, _}` | Repetition Range | `a{3, 5}` matches sequences of 3-5 a’s. (Parser only)
`?` | Optional | `a?` matches a or nothing.
`+` | Multiple | `a+` matches sequences of at least one a.
`*` | Any | `a*` matches any sequence of a’s.

`abc*` does not match repetitions of abc, but ab followed by any number of c’s. To match the former, use `(abc)*` instead.

Any whitespace in the regex is ignored.<br>
To match whitespace, either escape it or use reserved keywords such as \n or \t.

## Usage

Right now, to use regen in your own code, you would have to include Parser, Generator and Matcher seperately and compile your regex in multiple stages. For a reference, see main.c

## Installation

To install, just clone this repository and run `make lib` in the root of the repository. To produce a standalone binary, there is also `make debug` and `make release`.