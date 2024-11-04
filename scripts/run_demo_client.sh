#!/bin/bash
# Try to run the replicas as in run_demo.sh first and then run_demo_client.sh.
# Use Ctrl-C to terminate the proposing replica (e.g. replica 0). Leader
# rotation will be scheduled. Try to kill and run run_demo_client.sh again, new
# commands should still get through (be replicated) once the new leader becomes
# stable.
export LD_LIBRARY_PATH=~/Documents/EPFL/3.Semester/researchProject/libhotstuff/liberasurecode/src/.libs

./examples/hotstuff-client --idx 0 --iter -1 --max-async 4
