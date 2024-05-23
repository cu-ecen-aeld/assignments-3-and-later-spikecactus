ifeq ($(CROSS_COMPILE),)
    CC := gcc  # Default compiler is gcc
else
    CC := $(CROSS_COMPILE)gcc
endif

all: aesdsocket

clean:
	rm -f aesdsocket *.o