import sys
import textwrap
import sys
import re

comment_start = '#'
label_start = '$LABEL'

class OpCode:
    def __init__(self, name='', args=[], line_num=None, labels={}):
        self.name = name.upper()
        self.args = args
        self.opcode_parity_func_map = {
            'CLS': (0, self.build_cls),
            'RET': (0, self.build_ret),
            'SYS': (1, self.build_sys),
            'JP': (1, self.build_jp),
            'CALL': (1, self.build_call),
            'SE_BYTE': (2, self.build_se_byte),
            'SNE_BYTE': (2, self.build_sne_byte),
            'SE_REG': (2, self.build_se_reg),
            'LD_BYTE': (2, self.build_ld_byte),
            'ADD_BYTE_REG': (2, self.build_add_byte_reg)
        }
        self.line_num = line_num
        self.labels = labels

    def __str__(self):
        if self.line_num is None:
            return '{Name: ' + self.name + ', Args:' + str(self.args) + '}'
        else:
            return '{Name: ' + self.name + ', Args:' + str(self.args) + ', line: ' + str(self.line_num) + '}'

    def build_cls(self):
        return OpCode.build_opcode('0', '0', 'E', '0')

    def build_ret(self):
        return OpCode.build_opcode('0', '0', 'E', 'E')

    def build_sys(self, address):
        hex_addr = self.get_address_or_invalid(address)
        return OpCode.build_opcode('0', hex_addr[0], hex_addr[1], hex_addr[2])

    def build_jp(self, address):
        hex_addr = self.get_address_or_invalid(address)
        return OpCode.build_opcode('1', hex_addr[0], hex_addr[1], hex_addr[2])

    def build_call(self, address):
        hex_addr = self.get_address_or_invalid(address)
        return OpCode.build_opcode('2', hex_addr[0], hex_addr[1], hex_addr[2])

    def build_se_byte(self, register, value):
        reg_num = self.get_register_or_invalid(register)
        value = self.convert_val_to_hex_or_invalid(value, max_len=2)
        return OpCode.build_opcode('3', reg_num, value[0], value[1])

    def build_sne_byte(self, register, value):
        reg_num = self.get_register_or_invalid(register)
        value = self.convert_val_to_hex_or_invalid(value, max_len=2)
        return OpCode.build_opcode('4', reg_num, value[0], value[1])

    def build_se_reg(self, register1, register2):
        reg_num1 = self.get_register_or_invalid(register1)
        reg_num2 = self.get_register_or_invalid(register2)
        return OpCode.build_opcode('5', reg_num1, reg_num2, '0')

    def build_ld_byte(self, register, value):
        reg_num = self.get_register_or_invalid(register)
        value = self.convert_val_to_hex_or_invalid(value, max_len=2)
        return OpCode.build_opcode('6', reg_num, value[0], value[1])

    def build_add_byte_reg(self):
        pass

    def encoded(self):
        if self.name not in self.opcode_parity_func_map:
            self.invalid()

        func_arity, func = self.opcode_parity_func_map[self.name]

        if len(self.args) != func_arity:
            self.invalid(message='Mismatched opcode arity; expected {} args'.format(func_arity))

        return func(*self.args)

    # expects prefix to be a single hex character
    # guarantees to return a 3-digit long hex value
    def get_address_or_invalid(self, address):
        hex_addr = self.convert_val_to_hex_or_invalid(address, max_len=3)
        if len(hex_addr) > 3:
            self.invalid(message="Invalid address: '{}'".format(address))
        return hex_addr

    @staticmethod
    def is_valid_reg(reg):
        return re.search('^[Vv][0-9a-fA-F]$', reg) is not None

    # expects a register of the form 'V#'
    # returns the hex value of the register
    def get_register_or_invalid(self, register):
        if not OpCode.is_valid_reg(register):
            self.invalid(message="Register number '" + reg_number + "' not of the form V[0-9a-fA-F]")

        return register[1]

    def convert_val_to_hex_or_invalid(self, num, max_len=0):
        result = hex(int(num, base=0))[2:].zfill(max_len)
        if max_len != 0 and len(result) > max_len:
            self.invalid(message="Invalid conversion to hex value: '{}'; must be {} hex digits".format(num, max_len))

        return result

    # given four characters, combine them as hex digits
    # e.g., build_opcode('a', 'b', 'f', '3') == 0xabf3
    @staticmethod
    def build_opcode(dig1, dig2, dig3, dig4):
        # ex: when passed '3', 'f', builds 0x3f
        def build_byte(hex_digit_1, hex_digit_2):
            return chr(int(hex_digit_1, base=16) * 16 + int(hex_digit_2, base=16))

        return build_byte(dig1, dig2) + build_byte(dig3, dig4)

    def invalid(self, message=None):
        print 'Invalid opcode: {}'.format(str(self))
        if message is not None:
            print message
        sys.exit()


def assemble(input_file, output_file):
    def parse_label(line, opcodes, labels):
        first_token = line[0].upper()
        if first_token == label_start and len(line) == 2:
            labels[line[1]] = len(opcodes) + 1
            return True
        elif first_token == label_start:
            print 'Fatal error: Invalid label format: {}'.format(line)
            sys.exit()

        return False
    def parse_opcode(line, opcodes, labels):
        new_opcode = OpCode(name=line[0], args=line[1:], labels=labels)
        opcodes.append(new_opcode)

    opcodes = []
    labels = {}
    with open(input_file, 'r') as f:
        for index, line in enumerate(f):
            while len(line) > 0 and line[0].isspace():
                line = line[1:]

            if not line or line[0] == comment_start:
                continue

            line = line.split()
            first_token = line[0].upper()
            if first_token == label_start:
                parse_label(line, opcodes, labels)
            else:
                parse_opcode(line, opcodes, labels)

    if line[0].upper() == label_start:
        print 'Fatal error: Cannot end .as file with a label'
        sys.exit()

    print 'labels:'
    print labels
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
        ADD_BYTE_REG {register} {value} : add the value to the value in
            the register and store back in the same register
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
