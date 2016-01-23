gdb -batch -ex r -ex bt $1 >/tmp/$1 2>&1
OK=$(tail -1 /tmp/$1|grep "exited normally")
if [ -z "$OK" ]; then
    cat /tmp/$1;
    exit 1
fi
