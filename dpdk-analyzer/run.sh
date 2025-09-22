#!/bin/bash

./dpdk-analyzer -c7 --vdev=net_pcap0,iface=enp0s8 -- -i --nb-cores=2 --nb-ports=2 --total-num-mbufs=2048
