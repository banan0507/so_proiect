# AI Usage Documentation

## Tool used
Claude (claude.ai)

## What I asked for

I needed two helper functions for the filter command:

1. A function to parse a condition string like "severity:>=:2" into
   three separate parts: field, operator, and value.

2. A function to check if a Report struct satisfies a given condition,
   supporting fields like severity, category, inspector, timestamp
   and operators ==, !=, <, <=, >, >=.

I described my Report struct and asked Claude to generate both functions.

## What was generated

Claude generated parse_condition() using strchr() to find and split
on the ':' separator, and match_condition() using a COMPARE macro
to handle all six comparison operators without repeating code.

## What I changed

- Added explicit null terminators after strncpy calls in parse_condition()
  because I wasn't sure if strncpy always null-terminates (it doesn't
  when the source is longer than n).
- I verified that match_condition() correctly uses atoi() for severity
  and strcmp() for string fields - the AI handled this correctly.

## What I learned

- How strchr() works to split strings by finding a character position
- How C macros can reduce repetitive code (the COMPARE macro)
- The difference between atoi() for integers and strcmp() for strings
- That strncpy() doesn't always null-terminate - you have to do it manually

## What the AI got wrong

The first version of match_condition() didn't handle the case where
the field name is unknown - it just returned 0 silently. I added the
error message so it's easier to debug bad filter conditions.