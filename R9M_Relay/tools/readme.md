# ppm_parse.js 
Decodes single channel PPM signal from CSV file captured by Saleae8 Logic application.

### Run command:

cscript ppm_parse.js INPUT_FILE.csv CHANNEL-NUMBER

INPUT_FILE.csv should contain Time column and captured logic channel(s) columns;
CHANNEL-NUMBER is a zero-based number of the column (excluding Time column);

### CSV file example:

```
Time[s], Channel 0, Channel 1, Channel 2
0.000000000000000, 1, 1, 1
0.000882958333333, 1, 0, 1
0.001080541666667, 1, 0, 0
0.001171416666667, 0, 0, 0
0.001186958333333, 0, 1, 0
```

### Command example: 

cscript ppm_parse.js sample.csv 2

### Result file sample\ppm_2.csv

```
0,1081
1,1505
2,1505
3,1504
4,1504
```

# cppm_parse.js  

Decodes CPPM train from CSV file captured by Saleae8 Logic application.

### Run command:

cscript cppm_parse.js INPUT_FILE.csv CHANNEL-NUMBER

INPUT_FILE.csv should contain Time column and captured logic channel(s) columns;
CHANNEL-NUMBER is a zero-based number of the column (excluding Time column);

### CSV file example:

```
Time[s], Channel 0, Channel 1, Channel 2
0.000000000000000, 1, 1, 1
0.000882958333333, 1, 0, 1
0.001080541666667, 1, 0, 0
0.001171416666667, 0, 0, 0
0.001186958333333, 0, 1, 0
```

### Command example: 

cscript ppm_parse.js sample.csv 1

### Result file sample\cppm_1.csv

```
0,1504,1504,1505,1504,1504,1504,1504,1504,1505
1,1504,1504,1504,1504,1504,1504,1505,1504,1504
2,1504,1504,1504,1504,1505,1504,1504,1504,1504
3,1504,1504,1505,1504,1504,1504,1504,1504,1505
4,1505,1504,1504,1504,1504,1504,1505,1504,1504
5,1504,1504,1504,1504,1505,1504,1504,1504,1504
```

# pxx_parse.js 

Decodes PXX packets from CSV file captured by Saleae8 Logic application. Fills the folder with the raw packets and their decoded representation.

### Run command:

cscript cppm_parse.js

### CSV file example:

```
Time[s], Channel 0, Channel 1, Channel 2
0.000000000000000, 1, 1, 1
0.000882958333333, 1, 0, 1
0.001080541666667, 1, 0, 0
0.001171416666667, 0, 0, 0
0.001186958333333, 0, 1, 0
```

### Command example: 

cscript ppm_parse.js sample.csv 1

### CSV file example:

```
Time[s], Channel 0
0.000000000000000, 1
0.000005875000000, 0
0.000016000000000, 1
0.000021875000000, 0
0.000032000000000, 1
0.000037875000000, 0
0.000048000000000, 1
```

### Result file sample\0001.raw

```
0,1,0
10.166666666977107,0,10.166666666977107
16,1,5.8333333330228925
26.166666666977107,0,10.166666666977107
40,1,13.833333333022892
50.16666666697711,0,10.166666666977107
64.04166666697711,1,13.875
74.16666666697711,0,10.125
88.04166666697711,1,13.875
98.16666666697711,0,10.125
```

### Result file sample\0001.decoded

```
0111111000000111000000010000000000000000000010011001000000000000000010011100000000000000000011001100000000000000000011001100000000010010101000100000100001111110
000001110000000100000000000000000000100110010000000000000000100111000000000000000000110011000000000000000000110011000000000100101010001000001000


Length 144
rx_num 7
flag1 00000001
   BIND US(0)
   PROTO X16(0)
flag2 00000000
00000000 00001001 10010000
   2304 2304
00000000 00001001 11000000
   2304 3072
00000000 00001100 11000000
   3072 3072
00000000 00001100 11000000
   3072 3072
extra_flags 00010010
   RX_TELEM_OFF
   POWER 2
crc 162 8
```

