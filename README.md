# so_proiect

# City Manager - SO Project 2026

A C program that manages city infrastructure reports across districts.
Built for the Operating Systems course, Phase 1.

## How to compile

gcc -Wall -o city_manager city_manager.c city_operations.c secondary_functions.c

## How to run

./city_manager --role manager --user alice --add downtown
./city_manager --role inspector --user bob --list downtown
./city_manager --role manager --user alice --view downtown 1
./city_manager --role manager --user alice --remove_report downtown 1
./city_manager --role manager --user alice --update_threshold downtown 2
./city_manager --role inspector --user bob --filter downtown "severity:>=:2"

## Note on filter

Conditions with operators like >=, <=, >, < must be wrapped in quotes
in the terminal, otherwise the shell interprets them as redirects.

## Project structure

city_manager.c        - main, argument parsing
city_operations.c     - add, list, view, remove_report, update_threshold, filter
secondary_functions.c - helper functions: create_district, write_log, symlinks, permissions