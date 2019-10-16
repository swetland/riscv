// Copyright 2019, Brian Swetland <swetland@frotz.net>
// Licensed under the Apache License, Version 2.0.

volatile unsigned *uart = (void*) 0x80001000;

int main() {
	unsigned n;
	for (n = 0; n < 10; n++) {
		*uart = n;
	}
	return 42;
}

