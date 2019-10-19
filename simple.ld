
MEMORY {
	mem : ORIGIN = 0x80000000, LENGTH = 0x00010000
}

SECTIONS {
	.memory : {
		. = 0x00000000;
		start*(.text);
		*(.text);
		*(*);
		end = .;
		. = ALIGN(4);
	} > mem
}
