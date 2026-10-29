#!/bin/sh
# prepro-split.sh C2M -- c2m -E -fdump-defines reads back as its input did.
#
# The output ends with the #pragma pack state and the macros still defined,
# so a translation unit split in two compiles the same from
# [head's -E output][rest] as from [head][rest], with head preprocessed once:
#
#   1. prepro-split/{head,rest}.c, split that way: the same MIR, and it runs
#      (pack stack, #/##/variadic macros, -D and predefined macros undefined
#      or redefined, tokens that must not glue, an identifier starting u8);
#   2. every c-tests/new program that compiles: its -E -fdump-defines output
#      compiles to the same MIR as the program.
c2m=$1
dir=`dirname $0`
tmp=__prepro-split
fail=0

$c2m -w -DCMD=1 -E -fdump-defines $dir/prepro-split/head.c > $tmp-head.i || fail=1
cat $dir/prepro-split/head.c $dir/prepro-split/rest.c > $tmp-whole.c
cat $tmp-head.i $dir/prepro-split/rest.c > $tmp-split.c
$c2m -w -DCMD=1 -S $tmp-whole.c -o $tmp-whole.mir || fail=1
$c2m -w -DCMD=1 -S $tmp-split.c -o $tmp-split.mir || fail=1
if cmp -s $tmp-whole.mir $tmp-split.mir; then echo "prepro-split: same MIR"
else echo "prepro-split: MIR differs -- FAIL"; fail=1; fi
if $c2m -w -DCMD=1 $tmp-split.c -ei; then echo "prepro-split: runs"
else echo "prepro-split: run -- FAIL"; fail=1; fi

same=0
for f in $dir/new/*.c; do
  $c2m -w -I$dir/new -S $f -o $tmp-a.mir > /dev/null 2>&1 || continue
  $c2m -w -I$dir/new -E -fdump-defines $f > $tmp-e.c 2> /dev/null
  $c2m -w -I$dir/new -S $tmp-e.c -o $tmp-b.mir > /dev/null 2>&1
  if cmp -s $tmp-a.mir $tmp-b.mir; then same=`expr $same + 1`
  else echo "$f: -E output reads back differently -- FAIL"; fail=1; fi
done
echo "prepro-split: $same c-tests/new programs read back unchanged"
rm -f $tmp-*
exit $fail
