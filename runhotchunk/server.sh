trap 'exit 0' INT

sleep 2

# Initial Parameter Setup
branch=$1
latency=$2
bandwidth=$3
blocksize=$4
clientasync=$5

echo "startup with l:${latency} b:${bandwidth} blocksize:${blocksize}"

# Get Service-name
service="server-$ERASURE_UUID"

# Make sure correct branch is selected for crypto
cd libhotchunk && git pull && git submodule update --recursive --remote
git checkout "${branch}"

# Do a quick compile of the branch
git pull && cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED=ON -DHOTSTUFF_PROTO_LOG=ON && make

sleep 30

id=0
i=0

# Go through the list of servers of the given services to identify the number of servers and the id of this server.


for ip in $(dig A $service +short | sort -u)
do
  for myip in $(ifconfig -a | awk '$1 == "inet" {print $2}')
  do
    if [ ${ip} == ${myip} ]
    then
      id=${i}
      echo "This is: ${ip}"
    fi
  done
  ((i++))
done

sleep 20

# Store all services in the list of IPs 
dig A $service +short | sort -u | sed -e 's/$/ 1/' >> ips

sleep 5

# Generate the HotStuff config file based on the given parameters
python3 scripts/gen_conf.py --ips "ips" --keygen ./hotstuff-keygen --tls-keygen ./hotstuff-tls-keygen --block-size $blocksize

sleep 20

echo "Starting Application: #${i}"

# Startup Libhotstuff erasure
gdb -ex r -ex bt -ex q --args ./examples/hotstuff-app --conf ./hotstuff.gen-sec${id}.conf > log${id} 2>&1 &

sleep 40

#Configure Network restrictions
sudo tc qdisc add dev eth0 root netem delay ${latency}ms limit 400000 rate ${bandwidth}mbit &

sleep 25

# Start Client on Host Machine
if [ ${id} == 0 ]; then
  gdb -ex r -ex bt -ex q --args ./examples/hotstuff-client --idx ${id} --iter -900 --max-async ${clientasync} > clientlog0 2>&1 &
fi

sleep 180

killall hotstuff-client &
killall hotstuff-app &

# Wait for the container to be manually killed
sleep 3000
