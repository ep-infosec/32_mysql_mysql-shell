
#@#Source data is utf8mb4, but double-encoded in latin1 (preparation)
$a	txt$
$1	á$
$2	é$
$3	ã$
$4	ê$
$5	💩$
$5$
$$
$a	txt$
$1	Ã¡$
$2	Ã©$
$3	Ã£$
$4	Ãª$
$5	ðŸ’©$
$5$

#@# Preserve double-encoding as latin1
$a	txt$
$1	á$
$2	é$
$3	ã$
$4	ê$
$5	💩$
$5$

#@# Fix double-encoding so it can be queried as utf8mb4
$a	txt$
$1	á$
$2	é$
$3	ã$
$4	ê$
$5	💩$
$5$
