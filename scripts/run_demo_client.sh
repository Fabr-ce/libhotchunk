#!/bin/bash
# Try to run the replicas as in run_demo.sh first and then run_demo_client.sh.

./examples/hotstuff-client --idx 0 --iter -1 --max-async 4
