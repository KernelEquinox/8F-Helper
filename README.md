8F Helper
=========

A quick and dirty (but fast!) application in C that uses lookup tables to assist in the creation of item lists for use with the 8F item in Pokemon R/B/Y. It's got a small amount of error handling and supports the use of a single label for jumping.

## Download
You can download the latest version of the 8F Helper on the [releases](https://github.com/KernelEquinox/8F-Helper/releases/) page.

## Usage
```
Usage: gbz80aid [options] [hex]

Options:
  -f file    File mode (read input from file)
  -o format  Display output in a specific format
  -w         Disable item warning messages
  -h         Print this help message

Formats:
  asm        GB-Z80 assembly language
  hex        Hexadecimal GB-Z80 opcodes
  gen1       R/B/Y item codes for use with ACE
  gen2       G/S/C item codes for use with ACE

Examples:
  gbz80aid EA14D7C9
  gbz80aid -o asm -f bgb_mem.dump
  gbz80aid -o hex -f zzazz.asm
  gbz80aid -o gen1 0E1626642EBB4140CDD635C9
  gbz80aid -o gen2 -f coin_case.asm
```

## Examples
test_code.asm:
```asm
di               ; Kill all interrupts

.testJump        ; Just to show the label/jump processing
dec a
inc a
inc b
jr c, testJump   ; Relative jump to label

ld h, $FF        ; Set hl to $FF00
inc b
xor l

ld a, $20        ; Enter STOP mode until D-pad is pressed
inc c
ld (hl), a
stop

ret              ; Continue normal execution 
```
### Program outputs
```
root@gbdev:~# gbz80aid -o hex -f test_code.asm

Machine code: F33D3C0438FB26FF04AD3E200C771001C9
```
```
root@gbdev:~# gbz80aid F33D3C0438FB26FF04AD3E200C771001C9

gbz80 Assembly:

   0  F3               di
   1  3D               dec  a
   2  3C               inc  a
   3  04               inc  b
   4  38 FB            jr   c,$FB
   6  26 FF            ld   h,$FF
   8  04               inc  b
   9  AD               xor  l
   A  3E 20            ld   a,$20
   C  0C               inc  c
   D  77               ld   (hl),a
   E  10 01            stop
  10  C9               ret
```
```
root@gbdev:~# gbz80aid -o gen1 -f test_code.asm

Item            Quantity
========================
TM43            x61
Fresh Water     x4
Super Repel     x251
Carbos          x255
Poke Ball       x173
Lemonade        x32
Burn Heal       x119
Full Restore    x1
TM01            xAny
```

## Notes
I've opted to use `10 01` as the `STOP` opcode instead of the correct `10 00`. This is because it's much easier to get 1 of an item rather than 0 of an item. In all tests, the `STOP` instruction executes normally even with a non-zero argument.


