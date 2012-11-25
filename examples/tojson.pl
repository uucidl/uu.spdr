#!/usr/bin/env perl

$quoted_str_re = /\"([^\\\"]|\\.)*\"/;
$prefix = "";

printf "{\"traceEvents\":[";

while (<>) {
    if (/^([0-9]+) ([0-9]+) ([0-9]+) (\"(?:[^\\\"]|\\.)*\") (\"(?:[^\\\"]|\\.)*\") (\"[A-Z]\")(.*)$/) {
	printf $prefix;
	printf "{\"ts\":" . $1;
	printf ",\"pid\":" . $2;
	printf ",\"tid\":" . $3;
	printf ",\"cat\":" . $4;
	printf ",\"name\":" . $5;
	printf ",\"ph\":" . $6;
	printf ",\"args\":{";

	$args = $7;
	$subprefix = "";
	while ($args =~ / (\"(?:[^\\\"]|\\.)*\") ((?:\"(?:[^\\\"]|\\.)*\")|(?:[0-9]+\.[0-9]*)|(?:[0-9]+))/g) {
	    printf "$subprefix$1:$2";
	    $subprefix = ",";
	}

	    # for (i=7 ; i<= NF; i+=2) {
	#	if ($7 ~ /".*"/) {
	#		printf subprefix $i ": " $(i+1)
	#		subprefix = ","
	#	}
	# }
	printf "}}";
	$prefix = ",";
   }
}

printf "]}";
