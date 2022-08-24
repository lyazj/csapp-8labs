#/bin/bash

answer_12345=answer_12345.txt
answer_6=answer_6.txt
rm -f $answer_6
for a in `seq 1 6`; do
for b in `seq 1 6`; do
for c in `seq 1 6`; do
for d in `seq 1 6`; do
for e in `seq 1 6`; do
for f in `seq 1 6`; do
    echo $a $b $c $d $e $f
    echo $a $b $c $d $e $f | ./bomb $answer_12345 > /dev/null
    if [ $? = 0 ]; then
        echo $a $b $c $d $e $f >> $answer_6
        echo success!
        sleep 1
    fi
done
done
done
done
done
done
