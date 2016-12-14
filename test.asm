ld l,$6E
ld [hl], $36
ld a,$D3
ld ($D36F),a
inc b
ld c,$1c
ld h,$78
ld l,$48 ; 1c:7848: SaveSAVtoSRAM
ld b,c
call $35d6 ; BankSwitch
jp nc,$1f49 ; SoftReset
