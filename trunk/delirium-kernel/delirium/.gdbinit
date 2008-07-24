target remote 127.0.0.1:1234
#add-symbol-file apps/eve/eve 0x60000000
add-symbol-file apps/slip/slip 0x65020000
add-symbol-file apps/rtl8139/rtl8139 0x62020000
#signal 0-15 ignore
b kpanic
