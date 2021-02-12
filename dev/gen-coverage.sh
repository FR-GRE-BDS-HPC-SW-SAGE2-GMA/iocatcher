#!/bin/bash
######################################################
#            PROJECT  : ummap-io-v2                  #
#            LICENSE  : Apache 2.0                   #
#            COPYRIGHT: 2020 Bull SAS                #
######################################################

######################################################
# This script provde quick help to run coverage test

######################################################
set -x
set -e

######################################################
lcov -o out.info -c -d .
lcov -o out.info --remove out.info '/usr/*' '*/Test*' '*/googletest/*' '*/googlemock/*'
genhtml -o html out.info
