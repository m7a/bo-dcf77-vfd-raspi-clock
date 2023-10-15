#!/usr/bin/awk -f

/firsttel\[/ {
	openbrack  = index($0, "firsttel[")
	closebrack = index($0, "l]")
	aidx       = substr($0, openbrack + 9, closebrack - openbrack - 9)
	equ        = index($0, "]=")
	tailspc    = index(substr($0, equ), " ")
	aval       = substr($0, equ + 2, tailspc - 2)
	firsttel[aidx] = aval
}

/secondtel\[/ {
	openbrack  = index($0, "secondtel[")
	closebrack = index($0, "l]")
	aidx       = substr($0, openbrack + 10, closebrack - openbrack - 10)
	equ        = index($0, "]=")
	tailspc    = index(substr($0, equ), " ")
	aval       = substr($0, equ + 2, tailspc - 2)
	secondtel[aidx] = aval
}

END {
	printf "firsttel = {"
	for(i = 0; i < 15; i++) {
		printf "0x%02x,", firsttel[i]
	}
	printf "0x00}\nsecondttel = {"
	for(i = 0; i < 15; i++) {
		printf "0x%02x,", firsttel[i]
	}
	printf "0x00}\n"
}
