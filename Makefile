all: makesend makereceive

makesend:
	gcc -fno-stack-protector -D_FORTIFY_SOURCE=0 packet.c sender.c -o sender

makereceive:
	gcc -fno-stack-protector -D_FORTIFY_SOURCE=0 packet.c receiver.c -o receiver

clean:
	rm sender receiver
