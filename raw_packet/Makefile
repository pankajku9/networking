all: send recv

send:
	gcc -O2 sender.c -o sender

recv:
	gcc -O2 receiver.c -o receiver

	
#sudo ./receiver lo
#sudo ./sender lo FF:FF:FF:FF:FF:FF '_The contents of a 64-byte Ethernet frame_'
#https://stackoverflow.com/questions/24355597/linux-when-sending-ethernet-frames-the-ethertype-is-being-re-written