#!/bin/sh

LINT="./cpplint.py --extensions=hpp,h,cpp"
CLANG_FORMAT="clang-format -output-replacements-xml"

lint()
{
  SRC=$1
  FILTER=$2

  # LINT
  echo "  CPPLINT      $SRC"
  $LINT --filter=$FILTER $SRC 2>&1 >/dev/null | egrep -v "Done processing |Total errors" 1>&2
  if [ $? -ne 1 ]; then
    echo "error: cpplint.py found errors in $SRC"
    exit 1
  fi

  # CLANG_FORMAT
  echo "  CLANG_FORMAT $SRC"
  $CLANG_FORMAT $SRC | grep "<replacement " 1>&2
  if [ $? -ne 1 ]; then
    echo "error: $SRC needs to be clang-format'd!" 1>&2
    exit 1
  fi
}


FILTER="-readability/streams,-whitespace/line_length,-build/namespace,-readability/casting,-whitespace/comments"

lint proto3d.hpp "$FILTER,+build/c++11,-build/include"
lint gui_common.h "$FILTER,-build/include_what_you_use,-runtime/int"
lint demos/events_and_shader/events_and_shader.cpp "$FILTER,-legal/copyright,-readability/todo"
