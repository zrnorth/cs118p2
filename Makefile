all: makesend makereceive

makesend:
	gcc packet.c sender.c -o sender

makereceive:
	gcc packet.c receiver.c -o receiver

clean:
	rm sender receiver
