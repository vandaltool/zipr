Using sleds for handling consecutive pins. 

Consider that there are consecutive pins at 00, 01, 02, 03, 04 and 05. We would lay down the following bytes at the addresses 00 through 09:
00: 68
01: 68
02: 68
03: 68
04: 68
05: 68
06: 90
07: 90
08: 90
09: 90

If I jump to 00, the code looks like this:
push 68686868
00: 68
01: 68
02: 68
03: 68
04: 68
push 68909090
05: 68
06: 90
07: 90
08: 90
09: 90
and the stack looks like this:
rsp+0x8: 68686868
rsp    : 68909090

If I jump to 01, the code looks like this:
push 68686868
01: 68
02: 68
03: 68
04: 68
05: 68

nop
06: 90
nop
07: 90
nop
08: 90
nop
09: 90
and the stack looks like this:
rsp    : 68686868

If I jump to 02, the code looks like this:
push 68686890
02: 68
03: 68
04: 68
05: 68
06: 90

nop
07: 90
nop
08: 90
nop
09: 90
and the stack looks like this:
rsp    : 68686890

If I jump to 03, the code looks like this:
push 68689090
03: 68
04: 68
05: 68
06: 90
07: 90

nop
08: 90
nop
09: 90
and the stack looks like this:
rsp    : 68689090

If I jump to 04, the code looks like this:
push 68909090
04: 68
05: 68
06: 90
07: 90
08: 90

nop
09: 90
and the stack looks like this:
rsp    : 68909090

If I jump to 05, the code looks like this:
push
05: 68
06: 90
07: 90
08: 90
09: 90

and the stack looks like this:
rsp    : 90909090

D: (00 target)
cmp rsp+0x8, 68686868
jne X
cmp rsp+0x0, 68909090
jne X
lea rsp, rsp+0x10
jmp L00 (L00 is the code originally located at 00)

X: (01 target)
cmp rsp+0x0, 68686868 (we know that rsp+0x8 != 68686868)
jne Y
lea rsp, rsp+0x8
jmp L01

Y: (02 target)
cmp rsp+0x0, 68686890 (we know that rsp+0x8 != 68686868)
jne Z
lea rsp, rsp+0x8
jmp L02

Z: (03 target)
cmp rsp+0x0, 68689090 (we know that rsp+0x8 != 68686868)
jne AA
lea rsp, rsp+0x8
jmp L03

AA: (04 target)
cmp rsp+0x0, 68909090 (we know that rsp+0x8 != 68686868)
jne BB
lea rsp, rsp+0x8
jmp L04

AA: (05 target)
cmp rsp+0x0, 90909090 (we know that rsp+0x8 != 68686868)
jne FAIL
lea rsp, rsp+0x8
jmp L05

And here is an example of the code that is generated (it is
formatted for easier reading!

# Sleds are needed for pins between 0x400110 and 0x400115 (inclusive)
	400110:	68 68 68 68 68       	push   0x68686868
  400115:	68 90 90 90 90       	push   0xffffffff90909090
  40011a:	e9 00 00 00 00       	jmp    0x40011f

# Target 00
  40011f:	48 81 7c 24 08 68 68 	cmp    QWORD PTR [rsp+0x8],0x68686868
  400126:	68 68 
  400128:	0f 85 d2 0e 20 00    	jne    0x601000
  40012e:	48 81 3c 24 90 90 90 	cmp    QWORD PTR [rsp],0xffffffff90909090
  400135:	90 
  400136:	0f 85 c4 0e 20 00    	jne    0x601000
  40013c:	48 8d 64 24 10       	lea    rsp,[rsp+0x10]
  400141:                       jmp    <L00>

# Target 01
	601000:	48 81 3c 24 68 68 68 	cmp    QWORD PTR [rsp],0x68686868
  601007:	68 
  601008:	0f 85 4f f1 df ff    	jne    0x40015d
  60100e:	48 8d 64 24 08       	lea    rsp,[rsp+0x8]
  601013:	e9 2a f1 df ff       	jmp    <L01>

# Target 02
  40015d:	48 81 3c 24 68 68 68 	cmp    QWORD PTR [rsp],0xffffffff90686868
  400164:	90 
  400165:	0f 85 ad 0e 20 00    	jne    0x601018
  40016b:	48 8d 64 24 08       	lea    rsp,[rsp+0x8]
  400170:	e9 ce ff ff ff       	jmp    <L02>

# Target 03
  601018:	48 81 3c 24 68 68 90 	cmp    QWORD PTR [rsp],0xffffffff90906868
  60101f:	90 
  601020:	0f 85 4f f1 df ff    	jne    0x400175
  601026:	48 8d 64 24 08       	lea    rsp,[rsp+0x8]
  60102b:	e9 14 f1 df ff       	jmp    <L03>

# Target 04
  400175:	48 81 3c 24 68 90 90 	cmp    QWORD PTR [rsp],0xffffffff90909068
  40017c:	90 
  40017d:	0f 85 0a 00 00 00    	jne    0x40018d
  400183:	48 8d 64 24 08       	lea    rsp,[rsp+0x8]
  400188:	e9 b8 ff ff ff       	jmp    <L04>

# Target 05
  40018d:	48 81 3c 24 90 90 90 	cmp    QWORD PTR [rsp],0xffffffff90909090
  400194:	90 
  400195:	0f 85 0a 00 00 00    	jne    <FAIL>
  40019b:	48 8d 64 24 08       	lea    rsp,[rsp+0x8]
  4001a0:	e9 a1 ff ff ff       	jmp    <L05>
