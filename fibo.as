$label main
    ld_byte v0 0
    ld_byte v1 1
    ld_byte v2 0
    # result ends up in v1
    call fibo
    ld_byte va 0xff
    halt
$label fibo
    sne_byte v2 11
    ret
    add_byte v2 1

    # use v3 as a temp register
    ld_reg v3 v0

    add_reg v0 v1

    # get back the previous number in the series
    ld_reg v1 v3

    jp fibo
