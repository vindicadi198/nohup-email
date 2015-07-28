CFLAGS += -O3 -Wall
LDFLAGS += -lcurl

%: nohup_email

nohup_email: nohup.c nohup_email.c
	gcc $(CFLAGS) nohup.c nohup_email.c $(LDFLAGS) -o nohup

install:
	cp nohup /usr/local/bin

clean:
	rm -rf nohup
