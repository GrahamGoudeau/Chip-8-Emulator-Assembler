import sys
import textwrap
import sys
import re

# register numbers must be given in hexadecimal
opcode_num_args_pairs = {
    'CLS': 0,
    'RET': 0,
    'SYS': 1,
    'JP': 1,
    'CALL': 1,
    'SE_BYTE': 2,
    'SNE_BYTE': 2,
    'SE_REG': 2,
    'LD_BYTE': 2
}

def is_valid_reg(reg):
    return re.search('^[Vv][0-9a-fA-F]$', reg) is not None

# arg: string
def convert_val_to_hex(num, zfill=0):
    return hex(int(num, base=0))[2:].zfill(zfill)

# example: append_address('0', '145')
# expects len(address) == 3
def append_address(prefix, address):
    first_byte = build_byte(prefix, address[0])
    second_byte = build_byte(address[1], address[2])
    return first_byte + second_byte

# when passed '3', 'f', builds 0x3f
def build_byte(hex_digit_1, hex_digit_2):
    return chr(int(hex_digit_1, base=16) * 16 + int(hex_digit_2, base=16))

class OpCode:
    def __init__(self, name='', args=[]):
        self.name = name.upper()
        #self.num_args = num_args
        self.args = args

    def __str__(self):
        return self.name + ' ' + str(self.args)

    def encoded(self):
        if len(self.args) == opcode_num_args_pairs.get(self.name, None):
            # 00E0
            if self.name == 'CLS':
                return chr(0x00) + chr(0xE0)
            # 00EE
            if self.name == 'RET':
                return chr(0x00) + chr(0xEE)
            # not implemented
            if self.name == 'SYS':
                address = convert_val_to_hex(self.args[0], zfill=3)
                if len(address) > 3:
                    self.invalid()
                return append_address('0', address)
            # 1nnn
            if self.name == 'JP':
                address = convert_val_to_hex(self.args[0], zfill=3)
                if len(address) > 3:
                    self.invalid()
                return append_address('1', address)
            # 2nnn
            if self.name == 'CALL':
                address = convert_val_to_hex(self.args[0], zfill=3)
                return append_address('2', address)
            # 3xkk
            if self.name == 'SE_BYTE':
                reg_number = self.args[0]
                if not is_valid_reg(reg_number):
                    self.invalid(message="Register number '" + reg_number + "' not of the form V[0-9a-fA-F]")
                reg_number = reg_number[1]
                value = convert_val_to_hex(self.args[1], zfill=2)
                if len(value) > 2:
                    self.invalid(message="SE compare value must be less than 0xff")
                return build_byte('3', reg_number) + build_byte(value[0], value[1])

            if self.name == 'SNE_BYTE':
                reg_number = self.args[0]
                if not is_valid_reg(reg_number):
                    self.invalid(message="Register number '" + reg_number + "' not of the form V[0-9a-fA-F]")

                reg_number = reg_number[1]
                value = convert_val_to_hex(self.args[1], zfill=2)
                if len(value) > 2:
                    self.invalid(message="SE compare value must be less than 0xff")

                return build_byte('4', reg_number) + build_byte(value[0], value[1])

            if self.name == 'SE_REG':
                reg_number_1 = self.args[0]
                reg_number_2 = self.args[1]
                if not is_valid_reg(reg_number_1):
                    self.invalid(message="Register number '" + reg_number_1 + "' not of the form V[0-9a-fA-F]")
                if not is_valid_reg(reg_number_2):
                    self.invalid(message="Register number '" + reg_number_2 + "' not of the form V[0-9a-fA-F]")

                return build_byte('5', reg_number_1[1]) + build_byte(reg_number_2[1], '0')

            if self.name == 'LD_BYTE':
                reg_number = self.args[0]
                if not is_valid_reg(reg_number):
                    self.invalid(message="Register number '" + reg_number + "' not of the form V[0-9a-fA-F]")
                value = convert_val_to_hex(self.args[1], zfill=2)
                return build_byte('6', reg_number[1]) + build_byte(value[0], value[1])

        else:
            self.invalid()

    def invalid(self, message=None):
        print 'Invalid opcode: {}'.format(str(self))
        if message is not None:
            print message
        sys.exit()

def convert_line_to_opcode(line):
    if len(line) == 0:
        return None
    return OpCode(name=line[0], args=line[1:])

def assemble(input_file, output_file):
    instructions = []
    with open(input_file, 'r') as f:
        instructions = [line.split() for line in f]

    opcodes = []
    for instr in instructions:
        opcode = convert_line_to_opcode(instr)
        if opcode is not None:
            opcodes.append(opcode)

    encoded = map(lambda opcode: opcode.encoded(), opcodes)
    for opcode in opcodes:
        print str(opcode)
    with open(output_file, 'wb') as f:
        for code in encoded:
            f.write(code)

def print_usage():
    print textwrap.dedent("""\
        Usage: [script_name] [input.as] [output.ch8]

        (input and output args are sensitive to file extension)

        Grammar (not case-sensitive):

        Types listed below:
            * register: of the form V[0-9a-fA-F]
            * value: two byte hex or decimal literal
                (hex values must be preceded by 0x, otherwise assumed decimal)
            * address: 3-byte memory address

        CLS : clear the screen
        RET : return to the address on top of the call stack
        JP {address} : jump to the given address
        CALL {address} : jump to the address, and
            put current program counter on top of call stack
        SE_BYTE {register} {value} : skip the next instruction
            if register V# is equal to the value
        SNE_BYTE {register} {value} : same as above, but if NOT equal
        SE_REG {register} {register} : skip the next instruction if the two registers are equal
        LD_BYTE {register} {value} : load the value into the given register
        """)
if __name__ == '__main__':
    if len(sys.argv) != 3:
        print_usage()
        sys.exit()

    if sys.argv[1][-3:] == '.as' and sys.argv[2][-4:] == '.ch8':
        assemble(sys.argv[1], sys.argv[2])
    elif sys.argv[1][-3:] != '.as':
        print 'Unrecognized input assembly filetype: {} (expected .as)'.format(sys.argv[1][-3:])
    elif sys.argv[2][-4:] != '.ch8':
        print 'Unrecognized output filetype: {} (expected .ch8)'.format(sys.argv[2][-4:])
