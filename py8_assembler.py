import sys
import textwrap
import sys
import re
import os

comment_start = '#'
label_start = '$LABEL'
stack_init = '$STACK_INIT'
push_token = '$PUSH'
pop_token = '$POP'

# set the stack pointer back to its proper location
# (can be used after an opcode like Fx29, which sets the address reg
# to the location for digit vx)
reset_stack_token = '$RESET_STACK_POINTER'
required_input_ext = '.chasm'
required_output_ext = '.ch8'
memory_start = 0x200
memory_size = 0x1000
transient_register = 'v0'

class OpCode:
    def __init__(self, name='', args=[], line_num=None, labels={}):
        self.name = name.upper()
        self.args = args
        self.opcode_arity_func_map = {
            'HALT': (0, self.build_halt),
            'CLS': (0, self.build_cls),
            'RET': (0, self.build_ret),
            'SYS': (1, self.build_sys),
            'JP': (1, self.build_jp),
            'CALL': (1, self.build_call),
            'SE_BYTE': (2, self.build_se_byte),
            'SNE_BYTE': (2, self.build_sne_byte),
            'SE_REG': (2, self.build_se_reg),
            'LD_BYTE': (2, self.build_ld_byte),
            'ADD_BYTE': (2, self.build_add_byte_reg),
            'LD_REG': (2, self.build_ld_reg),
            'OR_REG': (2, self.build_or_reg),
            'AND_REG': (2, self.build_and_reg),
            'XOR_REG': (2, self.build_xor_reg),
            'ADD_REG': (2, self.build_add_reg),
            'SUB_REG': (2, self.build_sub_reg),
            'SHR_REG': (1, self.build_shr_reg),
            'SUBN_REG': (2, self.build_subn_reg),
            'SHL_REG': (1, self.build_shl_reg),
            'SNE_REG': (2, self.build_sne_reg),
            'LD_ADDR': (1, self.build_ld_addr),
            'JP_OFFSET': (1, self.build_jp_offset),
            'RND_AND': (1, self.build_rnd_and),
            'DRAW': (3, self.build_draw),
            'SKIP_PRESS': (1, self.build_skip_press),
            'SKIP_NPRESS': (1, self.build_skip_npress),
            'LD_DELAY': (1, self.build_ld_delay),
            'AWAIT_KEY': (1, self.build_await_key),
            'SET_DELAY': (1, self.build_set_delay),
            'SET_SOUND': (1, self.build_set_sound),
            'ADDR_OFFSET': (1, self.build_addr_offset),
            'LD_SPRITE': (1, self.build_ld_sprite),
            'STORE_BCD': (1, self.build_store_bcd),
            'STORE_REGS': (1, self.build_store_regs),
            'LD_REGS': (1, self.build_ld_regs)
        }
        self.line_num = line_num
        self.labels = labels

    def __str__(self):
        if self.line_num is None:
            return '{Name: ' + self.name + ', Args:' + str(self.args) + '}'
        else:
            return '{Name: ' + self.name + ', Args:' + str(self.args) + ', line: ' + str(self.line_num) + '}'

    def build_halt(self):
        return OpCode.build_opcode('0', '0', 'F', 'D')

    def build_cls(self):
        return OpCode.build_opcode('0', '0', 'E', '0')

    def build_ret(self):
        return OpCode.build_opcode('0', '0', 'E', 'E')

    def build_sys(self, address):
        return self.build_address_opcode('0', address)

    def build_jp(self, address):
        return self.build_address_opcode('1', address)

    def build_call(self, address):
        return self.build_address_opcode('2', address)

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

    def build_add_byte_reg(self, register, value):
        reg_num = self.get_register_or_invalid(register)
        value = self.convert_val_to_hex_or_invalid(value, max_len=2)
        return OpCode.build_opcode('7', reg_num, value[0], value[1])

    def build_ld_reg(self, register1, register2):
        reg_num1 = self.get_register_or_invalid(register1)
        reg_num2 = self.get_register_or_invalid(register2)
        return OpCode.build_opcode('8', reg_num1, reg_num2, '0')

    def build_or_reg(self, register1, register2):
        reg_num1 = self.get_register_or_invalid(register1)
        reg_num2 = self.get_register_or_invalid(register2)
        return OpCode.build_opcode('8', reg_num1, reg_num2, '1')

    def build_and_reg(self, register1, register2):
        reg_num1 = self.get_register_or_invalid(register1)
        reg_num2 = self.get_register_or_invalid(register2)
        return OpCode.build_opcode('8', reg_num1, reg_num2, '2')

    def build_xor_reg(self, register1, register2):
        reg_num1 = self.get_register_or_invalid(register1)
        reg_num2 = self.get_register_or_invalid(register2)
        return OpCode.build_opcode('8', reg_num1, reg_num2, '3')

    def build_add_reg(self, register1, register2):
        reg_num1 = self.get_register_or_invalid(register1)
        reg_num2 = self.get_register_or_invalid(register2)
        return OpCode.build_opcode('8', reg_num1, reg_num2, '4')

    def build_sub_reg(self, register1, register2):
        reg_num1 = self.get_register_or_invalid(register1)
        reg_num2 = self.get_register_or_invalid(register2)
        return OpCode.build_opcode('8', reg_num1, reg_num2, '5')

    def build_shr_reg(self, register):
        reg_num = self.get_register_or_invalid(register)
        return OpCode.build_opcode('8', reg_num, '0', '6')

    def build_subn_reg(self, register1, register2):
        reg_num1 = self.get_register_or_invalid(register1)
        reg_num2 = self.get_register_or_invalid(register2)
        return OpCode.build_opcode('8', reg_num1, reg_num2, '7')

    def build_shl_reg(self, register):
        reg_num = self.get_register_or_invalid(register)
        return OpCode.build_opcode('8', reg_num, '0', 'E')

    def build_sne_reg(self, register1, register2):
        reg_num1 = self.get_register_or_invalid(register1)
        reg_num2 = self.get_register_or_invalid(register2)
        return OpCode.build_opcode('9', reg_num1, reg_num2, '0')

    def build_ld_addr(self, address):
        return self.build_address_opcode('A', address)

    def build_jp_offset(self, address):
        return self.build_address_opcode('B', address)

    def build_rnd_and(self, register, value):
        hex_val = self.convert_val_to_hex_or_invalid(value, max_len=2)
        reg_num = self.get_register_or_invalid(register)
        return OpCode.build_opcode('C', reg_num, hex_val[0], hex_val[1])

    def build_draw(self, register1, register2, nibble):
        hex_nib = self.convert_val_to_hex_or_invalid(nibble, max_len=1)
        reg_num1 = self.get_register_or_invalid(register1)
        reg_num2 = self.get_register_or_invalid(register2)
        return OpCode.build_opcode('D', reg_num1, reg_num2, hex_nib)

    def build_skip_press(self, register):
        reg_num = self.get_register_or_invalid(register)
        return OpCode.build_opcode('E', reg_num, '9', 'E')

    def build_skip_npress(self, register):
        reg_num = self.get_register_or_invalid(register)
        return OpCode.build_opcode('E', reg_num, 'A', '1')

    def build_ld_delay(self, register):
        reg_num = self.get_register_or_invalid(register)
        return OpCode.build_opcode('F', reg_num, '0', '7')

    def build_await_key(self, register):
        reg_num = self.get_register_or_invalid(register)
        return OpCode.build_opcode('F', reg_num, '0', 'A')

    def build_set_delay(self, register):
        reg_num = self.get_register_or_invalid(register)
        return OpCode.build_opcode('F', reg_num, '1', '5')

    def build_set_sound(self, register):
        reg_num = self.get_register_or_invalid(register)
        return OpCode.build_opcode('F', reg_num, '1', '8')

    def build_addr_offset(self, register):
        reg_num = self.get_register_or_invalid(register)
        return OpCode.build_opcode('F', reg_num, '1', 'E')

    def build_ld_sprite(self, register):
        reg_num = self.get_register_or_invalid(register)
        return OpCode.build_opcode('F', reg_num, '2', '9')

    def build_store_bcd(self, register):
        reg_num = self.get_register_or_invalid(register)
        return OpCode.build_opcode('F', reg_num, '3', '3')

    def build_store_regs(self, register):
        reg_num = self.get_register_or_invalid(register)
        return OpCode.build_opcode('F', reg_num, '5', '5')

    def build_ld_regs(self, register):
        reg_num = self.get_register_or_invalid(register)
        return OpCode.build_opcode('F', reg_num, '6', '5')

    def encoded(self):
        if self.name not in self.opcode_arity_func_map:
            self.invalid()

        func_arity, func = self.opcode_arity_func_map[self.name]

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

    def resolve_label_or_invalid(self, label):
        source_code_pos = self.labels.get(label, None)
        if source_code_pos is None:
            self.invalid(message="Unable to resolve label '{}'".format(label))

        hex_address = self.convert_val_to_hex_or_invalid(str(source_code_pos + memory_start))

        if int(str(hex_address), base=16) > int(str(memory_size), base=16):
            self.invalid(message="Label '{}' translates to address out of range")

        return hex_address

    @staticmethod
    def is_valid_reg(reg):
        return re.search('^[Vv][0-9a-fA-F]$', reg) is not None

    # expects a register of the form 'V#'
    # returns the hex value of the register
    def get_register_or_invalid(self, register):
        if not OpCode.is_valid_reg(register):
            self.invalid(message="Register number '" + register + "' not of the form V[0-9a-fA-F]")

        return register[1]

    def convert_val_to_hex_or_invalid(self, num, max_len=0):
        try:
            numeric_result = hex(int(num, base=0))
        except ValueError as e:
            self.invalid(message="Unexpected error while converting to hex: '{}'".format(e))

        if int(numeric_result, base=16) < 0:
            self.invalid(message="Cannot give negative numeric literals")
        result = numeric_result[2:].zfill(max_len)
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

    def build_address_opcode(self, prefix, address):
        if OpCode.could_be_label(address):
            hex_addr = self.resolve_label_or_invalid(address)
        else:
            hex_addr = self.get_address_or_invalid(address)

        return OpCode.build_opcode(prefix, hex_addr[0], hex_addr[1], hex_addr[2])

    def build_register_op(self, reg1, reg2, first_nibble, last_nibble):
        reg_num1 = self.get_register_or_invalid(reg1)
        reg_num2 = self.get_register_or_invalid(reg2)
        return OpCode.build_opcode(first_nibble, reg_num1, reg_num2, last_nibble)

    @staticmethod
    def could_be_label(address):
        return re.search('^[a-zA-Z]', address) is not None

    def invalid(self, message=None):
        print 'Invalid opcode: {}'.format(str(self))
        if message is not None:
            print message
        sys.exit()

def assemble(input_file, output_file):
    global address_register
    address_register = None
    def parse_label(line, opcodes, labels, first_line):
        first_token = line[0].upper()
        if first_token == label_start and len(line) == 2:
            if not OpCode.could_be_label(line[1]):
                print 'Fatal error: Label must start with alphabetical chracter'
                sys.exit()

            if not first_line:
                labels[line[1]] = len(opcodes)
            else:
                labels[line[1]] = 0
            return True
        elif first_token == label_start:
            print 'Fatal error: Invalid label format: {}'.format(line)
            sys.exit()

        return False

    def parse_opcode(line, opcodes, labels):
        new_opcode = OpCode(name=line[0], args=line[1:], labels=labels)
        opcodes.append(new_opcode)

    # assumes that we have finished reading in the opcodes
    def generate_stack_init(args, opcodes):
        global address_register
        if len(args) != 0:
            print '"' + stack_init + '" does not take any arguments'
            sys.exit()

        address_register = memory_size - 1
        # account for all existing opcodes plus this new one
        set_addr = OpCode(name='LD_ADDR', args=[hex(address_register)])
        opcodes.append(set_addr)

    def generate_push(args, opcodes):
        global address_register
        if len(args) > 15:
            print 'Fatal error: Cannot push more than 15 registers at a time'
            sys.exit()

        if address_register is None:
            print 'Fatal error: a push occurred before the address register was initialized'
            sys.exit()

        store_opcode = OpCode(name='STORE_REGS', args=[transient_register])
        for arg in args:
            load_transient = OpCode(name='LD_REG', args=[transient_register, arg])
            opcodes.append(load_transient)
            opcodes.append(store_opcode)

        address_register -= len(args)
        update_addr_reg = OpCode(name='LD_ADDR', args=[hex(address_register)])
        opcodes.append(update_addr_reg)

    def generate_pop(args, opcodes):
        global address_register
        if address_register is None:
            print 'Fatal error: a pop occurred before the address register was initialized'
            sys.exit()

        if len(args) != 1:
            print 'Fatal error: must pop into exactly one register'
            sys.exit()

        register = args[0]
        address_register += 1
        mv_addr = OpCode(name='LD_ADDR', args=[hex(address_register)])
        opcodes.append(mv_addr)
        ld_transient = OpCode(name='LD_REGS', args=[transient_register])
        opcodes.append(ld_transient)
        ld_reg = OpCode(name='LD_REG', args=[register, transient_register])
        opcodes.append(ld_reg)

    def generate_reset(args, opcodes):
        global address_register
        if len(args) != 0:
            print '"' + reset_stack_token + '" does not take any args'
            sys.exit()

        reset_opcode = OpCode(name='LD_ADDR', args=[hex(address_register)])
        opcodes.append(reset_opcode)

    def parse_line(line, opcodes, labels, first_line):
        if len(line) == 0:
            return None

        first_token = line[0].upper()
        args = line[1:]
        if first_token == label_start:
            parse_label(line, opcodes, labels, first_line)
        elif first_token == stack_init:
            if not first_line:
                print 'Must initialize a stack on the first line of ' + required_input_ext + ' file'
                sys.exit()

            generate_stack_init(args, opcodes)
        elif first_token == push_token:
            generate_push(args, opcodes)
        elif first_token == pop_token:
            generate_pop(args, opcodes)
        elif first_token == reset_stack_token:
            generate_reset(args, opcodes)
        else:
            # TODO: change params to first_token, args
            parse_opcode(line, opcodes, labels)

    opcodes = []
    labels = {}
    with open(input_file, 'r') as f:
        # used to determine how to set a label on the first line
        first_line = True

        for index, line in enumerate(f):
            while len(line) > 0 and line[0].isspace():
                line = line[1:]

            if not line or line[0] == comment_start:
                continue

            line = line.split()
            parse_line(line, opcodes, labels, first_line)
            first_line = False

    if opcodes and line[0].upper() == label_start:
        print 'Fatal error: Cannot end .as file with a label'
        sys.exit()

    encoded = map(lambda opcode: opcode.encoded(), opcodes)
    print 'Writing opcodes:'
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
        ADD_BYTE {register} {value} : add the value to the value in
            the register and store back in the same register
        """)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print_usage()
        sys.exit()

    input_name = sys.argv[1]
    output_name = sys.argv[2]
    _, input_ext = os.path.splitext(input_name)
    _, output_ext = os.path.splitext(output_name)

    if input_ext != required_input_ext:
        print 'Unrecognized input assembly filetype: {} (expected {})'.format(input_ext, required_input_ext)
    elif output_ext != required_output_ext:
        print 'Unrecognized output filetype: {} (expected {})'.format(output_ext, required_output_ext)
    else:
        assemble(input_name, output_name)
