#### Docker Setup

First, checkout all the necessary code on all the host machines through

```
git clone https://github.com/Fabr-ce/libhotstuff_erasure.git
cd libhotstuff_erasure/runerasure
```

Build the Docker Images with:

```
docker build -t hotstuff-erasure .
```

On each of the physical machines.

Next, setup docker swarm with:

```
docker swarm init
```

On one of the servers.

Next, let the other machines join with:

```
docker swarm join --token <token> <ip>
```

Based on the token and IP the server where the init command was executed printed on start.

On the main machine, setup a docker network with:

```
docker network create --driver=overlay --subnet=10.1.0.0/16 --attachable erasure_network
```

#### Run Experiments

To run and configure experiments we first take a look at the "experiments" file.

```
# latency bandwidth block-size :  number of internals : number of total : suggested physical machines
['100','25','1000']:89:5
```

Each of the lines represents an experiment, given latency, bandwidth and block-size.
Additionally, it includes the number of nodes and finally the suggested number of physical servers.
(This will be printed in the log, and won't affect the overall execution)

By default, the number of nodes is 100.

To increase the number of nodes, enter the erasure.yaml file and adjust the number of replicas.

Note: To run HotStuff with libsec (vanilla), switch to the latest-libsec branch.

Finally, to run the experiments, simply run:

```
./runexperiment.sh
```

This will run 5 instances of each of the setups defined in the "experiments" file.

#### Interpretation of Results

The above script will result in a regular output similar to:

```
2021-08-17 14:14:43.546142 [hotstuff proto] x now state: <hotstuff hqc=affd30ca8f hqc.height=2700 b_lock=22365a13f8 b_exec=63c209503b vheight=27xx tails=1>
2021-08-17 14:14:43.546145 [hotstuff proto] Average: 200"
```

Where 'hqc.height=2700' presents the last finalized block. Considering the 5 minute interval, that results in 2700/300 blocks per second.
Considering the default of 1000 transactions per block, that results in `2700/300*1000 = 9000` ops per second.
The value next to "average" represents the average block latency.

#### Additional Experiments

To obtain more detailed throughput data over the given execution time, the runexperiment.sh script has to be adjusted to export the entire log to pastebin similarly to:

```
cat log* | grep "proto" | grep "height" | pastebinit -b paste.ubuntu.com
```

In order to parse the time between the receival of a block and it's finalization.

To run the experiments including failures, exchange the branch in the Dockerfile to "reconfiguration" and re-run the docker build. Following that, adjust the server.sh script to launch one client per potential leader (By default only once client is launched that connects to process 0 to reduce the resource requirements). After this is done, one of the processes may be killed manually during execution time after 1 minute warmup.

Detailed throughput data may be extracted as explained above.

#### Troubleshooting

There are several factors that could prevent the system from executing propperly:

-   Firewall settings between the physical machines
-   Insufficient resources

Possible workarounds consist of:

-   Reduceing the number of processes in erasure.yaml
-   Increaseing the timeouts in server.sh and re-executing the build step.
