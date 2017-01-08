#!/bin/sh

LINT="./cpplint.py --filter=+build/c++11,-readability/streams,-whitespace/line_length,-build/namespace,-readability/casting,-whitespace/comments"

$LINT proto3d.hpp
