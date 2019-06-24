#!/bin/bash

##Trap SIGINT and exit
ctrlc()
{
	exit $?
}

trap ctrlc SIGINT
netset()
{
	ip link add eth1 type dummy
	ip addr add 192.168.90.1/24 dev eth1
	ip link set eth1 up
}

##Run DNP3 Generator and Capture Traffic for different scenarios
pkttest()
{
	echo "Generating Traffic"
		tcpdump -B 4096 -n  -i lo  -w steadyfile.pcap&
		sed -i '16s/.*/dataFileName = steadyFile/' RTDSitest.lua
		./dnp3Generator -c Configitest.json > /dev/null 2>&1
		pkill tcpdump
		rm ./dnp3pipe


		tcpdump -B 4096 -n  -i lo  -w tocfile.pcap&
		sed -i '16s/.*/dataFileName = tocFile/' RTDSitest.lua
		./dnp3Generator -c Configitest.json > /dev/null 2>&1
		pkill tcpdump
		rm ./dnp3pipe

		tcpdump -B 4096 -n  -i lo  -w instfile.pcap&
		sed -i '16s/.*/dataFileName = instFile/' RTDSitest.lua
		./dnp3Generator -c Configitest.json > /dev/null 2>&1
		pkill tcpdump 
		rm ./dnp3pipe

		tcpdump -B 4096 -n  -i lo  -w s4ovfile.pcap&
		sed -i '16s/.*/dataFileName = s4ovFile/' RTDSitest.lua
		./dnp3Generator -c Configitest.json > /dev/null 2>&1
		pkill tcpdump 
		rm ./dnp3pipe

		tcpdump -B 4096 -n  -i lo  -w s5ovfile.pcap&
		sed -i '16s/.*/dataFileName = s5ovFile/' RTDSitest.lua
		./dnp3Generator -c Configitest.json > /dev/null 2>&1
		pkill tcpdump 
		rm ./dnp3pipe

		tcpdump -B 4096 -n  -i lo  -w s6uvfile.pcap&
		sed -i '16s/.*/dataFileName = s6uvFile/' RTDSitest.lua
		./dnp3Generator -c Configitest.json > /dev/null 2>&1
		pkill tcpdump 
		rm ./dnp3pipe

		tcpdump -B 4096 -n  -i lo  -w s7uvfile.pcap&
		sed -i '16s/.*/dataFileName = s7uvFile/' RTDSitest.lua
		./dnp3Generator -c Configitest.json > /dev/null 2>&1
		pkill tcpdump 
		rm ./dnp3pipe

		return $?
}

##Analyze pcap output with tshark
pktcheck()
{
	echo "ANALYZING PCAPS"
		sleep 5
		date > TEST_RESULTS.txt
		echo "########################################">> TEST_RESULTS.txt
		echo "Changes to Index 16" >> TEST_RESULTS.txt
		echo "########################################">> TEST_RESULTS.txt
		echo "----------------------------------------" >> TEST_RESULTS.txt
		steadycnt=$(tshark -r steadyfile.pcap "dnp3.al.point_index == 16" | tee >(wc -l) >> TEST_RESULTS.txt)
		echo "Total Events in Steady File: $steadycnt" >> TEST_RESULTS.txt
		echo "----------------------------------------" >> TEST_RESULTS.txt
		toccnt=$(tshark -r tocfile.pcap "dnp3.al.point_index == 16" | tee >(wc -l) >> TEST_RESULTS.txt)
		echo "Total Events in TOC File: $toccnt" >> TEST_RESULTS.txt
		echo "----------------------------------------" >> TEST_RESULTS.txt
		instcnt=$(tshark -r instfile.pcap "dnp3.al.point_index == 16" | tee >(wc -l) >> TEST_RESULTS.txt)
		echo "Total Events in Inst File: $instcnt" >> TEST_RESULTS.txt
		echo "----------------------------------------" >> TEST_RESULTS.txt
		s4ovcnt=$(tshark -r s4ovfile.pcap "dnp3.al.point_index == 16" | tee >(wc -l) >> TEST_RESULTS.txt)
		echo "Total Events in S4ov File: $s4ovcnt" >> TEST_RESULTS.txt
		echo "----------------------------------------" >> TEST_RESULTS.txt
		s5ovcnt=$(tshark -r s5ovfile.pcap "dnp3.al.point_index == 16" | tee >(wc -l) >> TEST_RESULTS.txt)
		echo "Total Events in S5ov File: $s5ovcnt" >> TEST_RESULTS.txt
		echo "----------------------------------------" >> TEST_RESULTS.txt
		s6uvcnt=$(tshark -r s6uvfile.pcap "dnp3.al.point_index == 16" | tee >(wc -l) >> TEST_RESULTS.txt)
		echo "Total Events in S6uv File: $s6uvcnt" >> TEST_RESULTS.txt
		echo "----------------------------------------" >> TEST_RESULTS.txt
		s7uvcnt=$(tshark -r s7uvfile.pcap "dnp3.al.point_index == 16" | tee >(wc -l) >> TEST_RESULTS.txt)
		echo "Total Events in S7uv File: $s7uvcnt" >> TEST_RESULTS.txt
		echo "----------------------------------------" >> TEST_RESULTS.txt
		echo "----------------------------------------" >> TEST_RESULTS.txt
		totalcnt=$(($steadycnt+$toccnt+$instcnt+$s4ovcnt+$s5ovcnt+$s6uvcnt+$s7uvcnt))
		echo "Total Events: $totalcnt" >> TEST_RESULTS.txt
		echo "----------------------------------------" >> TEST_RESULTS.txt

		
		##cat TEST_RESULTS.txt

		if [ $totalcnt -gt 250 ]
		then
		echo "Total Event Changes for Index 16 do not match. $totalcnt/232"
			exit 1
		fi


		echo "########################################">> TEST_RESULTS.txt
		echo "Total Buffer OverFlows" >> TEST_RESULTS.txt
		echo "########################################">> TEST_RESULTS.txt
		echo "----Steady State---------------------------------------------------" >> TEST_RESULTS.txt
		tshark -r steadyfile.pcap "dnp3.al.iin.ebo == 1" >> TEST_RESULTS.txt
		echo "----Over Current Fault 4-------------------------------------------" >> TEST_RESULTS.txt
		tshark -r tocfile.pcap  "dnp3.al.iin.ebo == 1" >> TEST_RESULTS.txt
		echo "----Overcurrent Instant Fault 6------------------------------------" >> TEST_RESULTS.txt
		tshark -r instfile.pcap "dnp3.al.iin.ebo == 1" >> TEST_RESULTS.txt
		echo "----Overvoltage Tripping-------------------------------------------" >> TEST_RESULTS.txt
		tshark -r s4ovfile.pcap "dnp3.al.iin.ebo == 1" >> TEST_RESULTS.txt
		echo "----Overvoltage Warning--------------------------------------------" >> TEST_RESULTS.txt
		tshark -r s5ovfile.pcap "dnp3.al.iin.ebo == 1" >> TEST_RESULTS.txt
		echo "----Undervoltage Warning-------------------------------------------" >> TEST_RESULTS.txt
		tshark -r s6uvfile.pcap "dnp3.al.iin.ebo == 1" >> TEST_RESULTS.txt
		echo "----Undervoltage Tripping------------------------------------------" >> TEST_RESULTS.txt
		tshark -r s7uvfile.pcap "dnp3.al.iin.ebo == 1" >> TEST_RESULTS.txt
		echo "----------------------------------------" >> TEST_RESULTS.txt


		echo "########################################">> TEST_RESULTS.txt
		echo "Malformed Packets" >> TEST_RESULTS.txt
		echo "########################################">> TEST_RESULTS.txt
		echo "----Steady State---------------------------------------------------" >> TEST_RESULTS.txt
		tshark -r steadyfile.pcap "_ws.malformed" >> TEST_RESULTS.txt
		echo "----Over Current Fault 4-------------------------------------------" >> TEST_RESULTS.txt
		tshark -r tocfile.pcap  "_ws.malformed" >> TEST_RESULTS.txt
		echo "----Overcurrent Instant Fault 6------------------------------------" >> TEST_RESULTS.txt
		tshark -r instfile.pcap "_ws.malformed" >> TEST_RESULTS.txt
		echo "----Overvoltage Tripping-------------------------------------------" >> TEST_RESULTS.txt
		tshark -r s4ovfile.pcap "_ws.malformed" >> TEST_RESULTS.txt
		echo "----Overvoltage Warning--------------------------------------------" >> TEST_RESULTS.txt
		tshark -r s5ovfile.pcap "_ws.malformed" >> TEST_RESULTS.txt
		echo "----Undervoltage Warning-------------------------------------------" >> TEST_RESULTS.txt
		tshark -r s6uvfile.pcap "_ws.malformed" >> TEST_RESULTS.txt
		echo "----Undervoltage Tripping------------------------------------------" >> TEST_RESULTS.txt
		tshark -r s7uvfile.pcap "_ws.malformed" >> TEST_RESULTS.txt
		echo "----------------------------------------" >> TEST_RESULTS.txt
		echo "EOF">> TEST_RESULTS.txt

		cat TEST_RESULTS.txt
		return $?
}
netset
pkttest
pktcheck
