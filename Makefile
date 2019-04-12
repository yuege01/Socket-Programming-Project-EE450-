
all:
		gcc -o client client.c client.h 
		gcc -o aws aws.c client.h 
		gcc -o serverA serverA.c client.h 
		gcc -o serverB serverB.c client.h -lm
		gcc -o monitor monitor.c client.h 

clean:
	rm -f *.o client aws serverA serverB monitor server database.txt

.PHONY: serverA
serverA: serverA
	./serverA

.PHONY: serverB
serverB: serverB
	./serverB

.PHONY: aws
aws: aws
	./aws

.PHONY: monitor
monitor: monitor
	./monitor

