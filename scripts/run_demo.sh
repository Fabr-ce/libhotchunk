#!/bin/bash
rep=({0..3})
if [[ $# -gt 0 ]]; then
    rep=($@)
fi

# avoid stack overflow as in our simple demo the bootstrapping replica will
# potentially have a long chain of promise resolution
ulimit -s unlimited

export LD_LIBRARY_PATH=~/Documents/EPFL/3.Semester/researchProject/libhotstuff/liberasurecode/src/.libs

for i in "${rep[@]}"; do
    echo "starting replica $i"
    #valgrind --leak-check=full ./examples/hotstuff-app --conf hotstuff-sec${i}.conf > log${i} 2>&1 &
    #gdb -ex r -ex bt -ex q --args ./examples/hotstuff-app --conf hotstuff-sec${i}.conf > log${i} 2>&1 &
    ./examples/hotstuff-app --conf ./hotstuff-sec${i}.conf > log${i} 2>&1 &
done
wait
