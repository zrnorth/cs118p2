all: makesend makereceive

makesend:
	gcc packet.h sender.c -o sender

makereceive:
	gcc packet.h receiver.c -o receiver

clean:
	rm sender receiver
