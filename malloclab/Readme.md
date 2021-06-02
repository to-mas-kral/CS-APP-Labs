# Malloc lab

The code has typical homework quality.

# Results

|trace|valid|util|ops|secs|kops|
|---|---|---|---|---|---|
|0|yes|99%| 5694|0.000232|24564|
|1|yes|99%| 5848|0.000256|22879|
|2|yes|99%| 6648|0.000292|22767|
|3|yes|99%| 5380|0.000211|25510|
|4|yes|89%|14400|0.000287|50209|
|5|yes|95%| 4800|0.006059|  792|
|6|yes|94%| 4800|0.005587|  859|
|7|yes|54%|12000|0.054974|  218|
|8|yes|47%|24000|0.083762|  287|
|9|yes|37%|14401|0.000649|22186|
|10|yes|45%|14401|0.000352|40877|
|total|all|78%|112372|0.152660|736|

Perf index = 47 (util) + 40 (thru) = 87/100

# Solution

|Block Layout||
|---|---|
|4 bytes|header|
|4 bytes|previous free block pointer|
|4 bytes|next free block pointer|
|8 byte aligned|block itself|
|4 bytes|footer|

I use this layout for both free and allocated blocks.
The previous and next pointers are redundant within the allocated blocks,
but I don't want to work on this anymore.

I guess that expanding this to a segregated free list wouldn't be difficult
and would improve the results a lot.
