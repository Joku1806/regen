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

## Installation

To install, clone this repository and run `make lib` in the root of the project. 
It will generate `lib/libregen.so` that you can copy to whereever you need it.

## Usage

To use regen you will need to link against `libregen.so` and also copy `src/matcher.h` to your own project.
After that you can use it like this:

```c
#include <stdlib.h>
#include <stdio.h>
#include "path/to/matcher.h"

int main(int argc, char** argv) {
    char* text = "Having a chat with a cat wearing a hat!";
    char* regex = "(c|h)+at!?";

    size_t matches_count = 0;
    Match* matches = match(text, regex, &matches_count);

    for (size_t idx = 0; idx < matches_count; idx++) {
        Match m = matches[idx];
        printf("Found \"%.*s\"\n", (int)m.length, text + m.offset);
    }

    free(matches);
    return 0;
}
```
When calling `match` you need to pass your text and regex as well as a `size_t*` which will contain the number of matches after the function ends. This way you can to iterate over the returned matches.

The return value of `match` is an array of structs containing offset and length, but no additional information about the text itself.<br>
So don't touch the text until you have done everything you want with the matches!