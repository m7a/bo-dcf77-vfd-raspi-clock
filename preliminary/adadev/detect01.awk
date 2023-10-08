#!/usr/bin/awk -f

/IINFO/ {
	date = substr($0, 1, 12)
	if (date != dateprev) {
		if (dateinfo != "") {
			printf(" => %s\n", dateinfo)
		}
		dateinfo = ""
		printf("-- %s --\n", date)
	}
}

/IINFO ot= ((1[56789][0-9]{4})|(2[0-9]{5})) st= [0-9]+ pd=TRUE/ {
	printf("1 %s\n", $0)
	dateinfo = sprintf("%s1", dateinfo)
}

/IINFO ot= (([0-9]{5})|(1[01234][0-9]{4})) st= [0-9]+ pd=TRUE/ {
	printf("0 %s\n", $0)
	dateinfo = sprintf("%s0", dateinfo)
}

{
	dateprev = date
}

END {
	printf(" => %s (INCOMPLETE)\n", dateinfo)
}
