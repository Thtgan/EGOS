;;+---------------+--------------+-------------+
;;| Range(in bit) | Size(in bit) | Description |
;;+---------------+--------------+-------------+
;;|     00-15     |      16      | Limit 0:15  |
;;|     16-31     |      16      | Base 0:15   |
;;|     32-39     |      8       | Base 16:23  |
;;|     40-47     |      8       | Access Byte |
;;|     48-51     |      4       | Limit 16:19 |
;;|     52-55     |      4       | Flags       |
;;|     56-63     |      8       | Base 24:31  |
;;+---------------+--------------+-------------+

gdt_begin:
gdt_null_desc:
    dd 0x00000000
    dd 0x00000000

gdt_code_desc:
    dw 0xFFFF       ; Limit
    dw 0x0000       ; Base (low 16 bits)
    db 0x00         ; Base (mid 8 bits)
    db 10011010b    ; Access
    db 11001111b    ; Granularity
    db 0x00         ; Base (high 8 bits)

gdt_data_desc:
    dw 0xFFFF       ; Limit
    dw 0x0000       ; Base (low 16 bits)
    db 0x00         ; Base (mid 8 bits)
    db 10010010b    ; Access
    db 11001111b    ; Granularity
    db 0x00         ; Base (high 8 bits)
gdt_end:

gdt_desc:
    dw gdt_end - gdt_begin - 1
    dd gdt_begin
    
gdt_code_seg: equ gdt_code_desc - gdt_begin
gdt_data_seg: equ gdt_data_desc - gdt_begin