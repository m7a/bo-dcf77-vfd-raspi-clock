#!/bin/awk -f

BEGIN {
	character    = "e"
	row          = 1
	col          = 1
	matrix[1][1] = 0
	width        = 0
}

function convert_print() {
	if (character == "a") {
		printf("':' => (")
	} else {
		printf("'%s' => (", character)
	}
	height = row - 1
	for (col = 1; col <= width; col++) {
		if (col != 1)
			printf(",\n\t")
		for (row = 1; row <= height; row += 8) {
			if (row != 1)
				printf(", ")
			# awk doesn't know bit operators it seems. Never mind,
			# compute it in decimal. also it seems we shouldn't
			# break this line. Don't mind that either...
			printf("16#%02x#", matrix[col][row + 7] * 128 + matrix[col][row + 6] * 64 + matrix[col][row + 5] * 32 + matrix[col][row + 4] * 16 + matrix[col][row + 3] * 8 + matrix[col][row + 2] * 4 + matrix[col][row + 1] * 2 + matrix[col][row + 0] * 1)
		}
	}
	printf "),\n"
}

/^0x/ {
	if (row != 1) {
		convert_print()
	}
	character = substr($0, 4, 1)
	row = 1
}

/^    ([.@])+$/ {
	str = substr($0, 5)
	width = length($0) - 4
	for (col = 1; col <= width; col++) {
		if (substr(str, col, 1) == "@") {
			matrix[col][row] = 1
		} else {
			matrix[col][row] = 0
		}
	}
	row++
}

END {
	convert_print()
}
