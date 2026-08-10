
import struct
import os
import hashlib

def sanitize_filename(name: str) -> str:
    """Remove invalid characters from filename."""
    # Replace invalid characters
    invalid = '<>:"/\\|?*'
    for c in invalid:
        name = name.replace(c, '_')
    # Remove control characters
    name = ''.join(c if ord(c) >= 32 and ord(c) < 127 else '_' for c in name)
    return name.strip()

def is_valid_filename_bytes(name_bytes: bytes) -> bool:
    """Check if bytes look like a valid filename (printable ASCII)."""
    # Must have at least one printable character before null
    name = name_bytes.split(b'\x00')[0]
    if len(name) == 0:
        return False
    # Check if all characters are printable ASCII (0x20-0x7E)
    for b in name:
        if b < 0x20 or b > 0x7E:
            return False
    return True

def is_valid_filename_bytes(name_bytes: bytes) -> bool:
    """Check if bytes look like a valid filename (printable ASCII)."""
    # Must have at least one printable character before null
    name = name_bytes.split(b'\x00')[0]
    if len(name) == 0:
        return False
    # Check if all characters are printable ASCII (0x20-0x7E)
    for b in name:
        if b < 0x20 or b > 0x7E:
            return False
    return True

def extract_pac_mdt(pac_name, output_dir):
    
    with open(pac_name, 'rb') as pac:
            data = pac.read()

    """Extract files from a PAC or MDT archive."""
    file_size = len(data)

    if len(data) < 16:
        return 0

    # Detect format: Check if first 12 bytes look like a filename
    first_12_bytes = data[0:12]
    is_named_format = is_valid_filename_bytes(first_12_bytes)

    if 0:#is_named_format:
        # Standard PAC/MDT format: 12-byte name + 4-byte offset per entry
        first_offset = struct.unpack_from('<I', data, 12)[0]

        if first_offset == 0 or first_offset > file_size or first_offset % 16 != 0:
            return 0  # Not a valid PAC/MDT

        num_entries = first_offset // 16

        # Parse entries
        entries = []
        for i in range(num_entries):
            pos = i * 16
            if pos + 16 > len(data):
                break
            name_bytes = data[pos:pos+12]
            offset = struct.unpack_from('<I', data, pos + 12)[0]
            name = name_bytes.split(b'\x00')[0].decode('latin-1', errors='replace')
            name = sanitize_filename(name)
            if name and offset < file_size:
                entries.append({'name': name, 'offset': offset})

        # Calculate sizes using next entry's offset
        for i in range(len(entries)):
            if i < len(entries) - 1:
                entries[i]['size'] = entries[i+1]['offset'] - entries[i]['offset']
            else:
                entries[i]['size'] = file_size - entries[i]['offset']

        # Extract files
        os.makedirs(output_dir, exist_ok=True)
        extracted = 0

        for e in entries:
            if not e['name'] or e['offset'] >= file_size or e['size'] <= 0:
                continue

            out_path = os.path.join(output_dir, e['name'])
            file_data = data[e['offset']:e['offset'] + e['size']]

            with open(out_path, 'wb') as f:
                f.write(file_data)
            extracted += 1

        return extracted

    else:
        # Indexed PAC format: Array of 4-byte offsets (no embedded names)
        # The header is an offset table, data follows after.
        # Strategy: Find the minimum non-zero offset from the FIRST few slots
        # (we only scan initial slots to avoid reading data as offsets)

        # First pass: scan first 64 slots to find a reasonable minimum offset
        # These are most likely to be valid header entries
        first_data_offset = file_size
        for i in range(min(64, file_size // 4)):
            offset = struct.unpack_from('<I', data, i * 4)[0]
            # Valid data offset: non-zero, > 64 (past minimal header), < file_size
            if offset > 0x40 and offset < file_size and offset < first_data_offset:
                first_data_offset = offset

        if first_data_offset >= file_size:
            return 0  # No valid offsets found

        # The header size is determined by the first data offset
        num_slots = first_data_offset // 4

        # Collect all valid entries with their offsets
        # Only include offsets that point to the data section (>= first_data_offset)
        entries = []
        for i in range(num_slots):
            offset = struct.unpack_from('<I', data, i * 4)[0]
            if offset >= first_data_offset and offset < file_size:
                entries.append({'index': i, 'offset': offset, 'name': f'{i:04d}.bin'})

        if not entries:
            return 0

        # Sort by offset to calculate sizes
        entries.sort(key=lambda x: x['offset'])

        # Calculate sizes
        for i in range(len(entries)):
            if i < len(entries) - 1:
                entries[i]['size'] = entries[i+1]['offset'] - entries[i]['offset']
            else:
                entries[i]['size'] = file_size - entries[i]['offset']

        # Extract files
        os.makedirs(output_dir, exist_ok=True)
        extracted = 0

        for e in entries:
            if e['size'] <= 0:
               continue

            out_path = os.path.join(output_dir, e['name'])
            file_data = data[e['offset']:e['offset'] + e['size']]

            with open(out_path, 'wb') as f:
                f.write(file_data)
            extracted += 1

        return extracted

def extract_pac(pac_name, output_dir):
    
    with open(pac_name, 'rb') as pac:
        data = pac.read()

    file_size = len(data)
    
    # Indexed PAC format: Array of 4-byte offsets (no embedded names)
    # The header is an offset table, data follows after.
    # first 4-byte is first data offset that come after header directly

    first_data_offset = int.from_bytes(data[0:4], byteorder='little')

    header = data[:first_data_offset]

    # Collect all valid entries with their offsets
    entries = []
    for i in range(0, first_data_offset, 4):
        offset = int.from_bytes(data[i:i+3], byteorder='little')
        entries.append({'index': i, 'offset': offset, 'name': f'{i//4:04d}.bin'})

    if not entries:
        return 0

    # Sort by offset to calculate sizes
    entries.sort(key=lambda x: x['offset'])

    # Calculate sizes
    for i in range(len(entries)):
        if i < len(entries) - 1:
            entries[i]['size'] = entries[i+1]['offset'] - entries[i]['offset']
        else:
            entries[i]['size'] = file_size - entries[i]['offset']

    # Extract files
    os.makedirs(output_dir, exist_ok=True)
    extracted = 0

    for e in entries:
        if e['size'] <= 0:
           continue

        out_path = os.path.join(output_dir, e['name'])
        file_data = data[e['offset']:e['offset'] + e['size']]

        with open(out_path, 'wb') as f:
            f.write(file_data)
        extracted += 1

    out_path = os.path.join(output_dir, "header.bin")
    with open(out_path, 'wb') as f:
        f.write(header)

    return extracted

def dump_header(pac_name):

    with open(pac_name, 'rb') as pac:
        data = pac.read()

    """Extract files from a PAC or MDT archive."""
    file_size = len(data)

    if len(data) < 16:
        return 0

    # Indexed PAC format: Array of 4-byte offsets (no embedded names)
    # The header is an offset table, data follows after.
    # Strategy: Find the minimum non-zero offset from the FIRST few slots
    # (we only scan initial slots to avoid reading data as offsets)

    # First pass: scan first 64 slots to find a reasonable minimum offset
    # These are most likely to be valid header entries
    first_data_offset = file_size
    for i in range(min(64, file_size // 4)):
        offset = struct.unpack_from('<I', data, i * 4)[0]
        # Valid data offset: non-zero, > 64 (past minimal header), < file_size
        if offset > 0x40 and offset < file_size and offset < first_data_offset:
            first_data_offset = offset

    if first_data_offset >= file_size:
        return 0  # No valid offsets found

    # The header size is determined by the first data offset
    #num_slots = first_data_offset // 4

    with open(os.path.basename(pac_name).split('.')[0] + "_header.bin", 'wb') as file:
        file.write(data[:first_data_offset])

def get_entries(pac_name):

    with open(pac_name, 'rb') as pac:
        data = pac.read()

    """Extract files from a PAC or MDT archive."""
    file_size = len(data)

    if len(data) < 16:
        return 0

    # Indexed PAC format: Array of 4-byte offsets (no embedded names)
    # The header is an offset table, data follows after.
    # Strategy: Find the minimum non-zero offset from the FIRST few slots
    # (we only scan initial slots to avoid reading data as offsets)

    # First pass: scan first 64 slots to find a reasonable minimum offset
    # These are most likely to be valid header entries
    first_data_offset = file_size
    for i in range(min(64, file_size // 4)):
        offset = struct.unpack_from('<I', data, i * 4)[0]
        # Valid data offset: non-zero, > 64 (past minimal header), < file_size
        if offset > 0x40 and offset < file_size and offset < first_data_offset:
            first_data_offset = offset

    if first_data_offset >= file_size:
        return 0  # No valid offsets found

    # The header size is determined by the first data offset
    num_slots = first_data_offset // 4

    entries = []
    for i in range(num_slots):
        offset = struct.unpack_from('<I', data, i * 4)[0]
        if offset >= first_data_offset and offset < file_size:
            entries.append({'index': i, 'offset': offset, 'name': f'{i:04d}.bin'})

    # Sort by offset to calculate sizes
    entries.sort(key=lambda x: x['offset'])

    # Calculate sizes
    for i in range(len(entries)):
        if i < len(entries) - 1:
            entries[i]['size'] = entries[i+1]['offset'] - entries[i]['offset']
        else:
            entries[i]['size'] = file_size - entries[i]['offset']
    
    for i in range(len(entries)-1, -1, -1):
        if entries[i]['size'] <= 0:
           entries.remove(entries[i])

    return entries

def pack_data(dir_name, entries, header_path):

    with open(os.path.basename(dir_name) + ".pac", 'wb') as pac:
        with open(header_path, 'rb') as f:
            data = f.read()
            pac.write(data)
        for e in entries:
            with open(os.path.join(dir_name, e['name']), 'rb') as f:
                  data = f.read()
                  pac.write(data)

def repack_data(data_path):

    entries = os.listdir(data_path)
    entries.remove("header.bin")
    # Sort files by numbers
    entries.sort(key=lambda x: int(x[:4]))

    with open(os.path.basename(data_path) + ".pac", 'wb') as pac:
        with open(os.path.join(data_path, "header.bin") , 'rb') as f:
            data = f.read()
            pac.write(data)
        for e in entries:
            if e[-3:] == "bin":
                with open(os.path.join(data_path, e), 'rb') as f:
                      data = f.read()
                      pac.write(data)

def pack_data_and_discard(dir_name, entries, out_name, header_path, entry_to_discard):

    if(os.path.exists(os.path.join(dir_name, entry_to_discard)) == False):
        print("entry_to_discard:", entry_to_discard, " does not exist !")
    with open(out_name, 'wb') as pac:
        with open(header_path, 'rb') as f:
            data = f.read()
            pac.write(data)
        for e in entries:
            if e['name'] == entry_to_discard:
                with open(os.path.join(dir_name, e['name']), 'rb') as f:
                    data = f.read()
                    empty_data = b'\x00'* len(data)
                    pac.write(empty_data)
            else:
                with open(os.path.join(dir_name, e['name']), 'rb') as f:
                      data = f.read()
                      pac.write(data)

def repack_data_and_discard(data_path, out_name, entry_to_discard):

    if(os.path.exists(os.path.join(data_path, entry_to_discard)) == False):
        print("entry_to_discard:", entry_to_discard, " does not exist !")

    entries = os.listdir(data_path)
    entries.remove("header.bin")
    # Sort files by numbers
    entries.sort(key=lambda x: int(x[:4]))

    with open(out_name, 'wb') as pac:
        with open(os.path.join(data_path, "header.bin") , 'rb') as f:
            data = f.read()
            pac.write(data)
        for e in entries:
            if e == entry_to_discard:
                with open(os.path.join(data_path, e), 'rb') as f:
                    data = f.read()
                    empty_data = b'\x00'* len(data)
                    pac.write(empty_data)
            else:
                if e[-3:] == "bin":
                    with open(os.path.join(data_path, e), 'rb') as f:
                          data = f.read()
                          pac.write(data)


def pack_data_and_discard_range(dir_name, entries, out_name, header_path, entries_discard_range):

    with open(out_name, 'wb') as pac:
        with open(header_path, 'rb') as f:
            data = f.read()
            pac.write(data)
        for e in entries:
            #if e['name'] == entry_to_discard:
            if int(e['name'][:-4]) > entries_discard_range[0] \
            and int(e['name'][:-4]) < entries_discard_range[1]:
                with open(os.path.join(dir_name, e['name']), 'rb') as f:
                    data = f.read()
                    empty_data = b'\x00'* len(data)
                    pac.write(empty_data)
            else:
                with open(os.path.join(dir_name, e['name']), 'rb') as f:
                      data = f.read()
                      pac.write(data)

def check_hash(file_1, file_2):
    with open(file_1, 'rb') as f1:
        f1_data = f1.read()
        f1_hash = hashlib.sha1(f1_data).hexdigest()
        with open(file_2, 'rb') as f2:
            f2_data = f2.read()
            f2_hash = hashlib.sha1(f2_data).hexdigest()
            print("file_1_size:", len(f1_data), " file_2_size:", len(f2_data))
            print(f1_hash, f2_hash, sep='\n')
            print("hash_comparison_result = ", f1_hash == f2_hash)
            if(f1_hash != f2_hash):
                for i in range(len(f1_data)):
                    if f1_data[i] != f2_data[i]:
                        print("data_different_here:", i, " max_size:", len(f1_data))
                        break

def extract_symbols(file):
    with open(file, 'r') as f:
        symbols = ""
        symbols += "// https://github.com/ethteck/splat/wiki/Adding-Symbols\n\n\n"
        symbols += "// function labels\n\n"
        for line in f:
            line = line.split()
            if line[1] == "TEXT":
                if line[0][:3] != "sub":
                    symbols += "{} = 0x{}; // type:func size:{}\n".format(line[0],
                                line[2], int(line[3], base=16))
        with open("symbol_addrs.txt", 'w') as f2:
            f2.write(symbols)

def signed_to_unsigned(num, num_bytes=1):
    max_value = 2 ** (8 * num_bytes) - 1
    return num & max_value

def two_complement(num, num_bytes=1):
    max_value = 2 ** (8 * num_bytes) - 1
    is_signed = num & (1 << (num_bytes*8-1)) != 0
    if(is_signed):
        return -(-num & max_value)
    else:
        return num

def fixed_point_value(num, dec_bits=8, frac_bits=8):
    #scale = 1 << frac_bits
    num_bytes = int(dec_bits / 8)
    #frac_max_value = 2 ** frac_bits - 1
    decimal = num >> frac_bits
    #fractional = num & frac_max_value
    #float_value = num / scale
    if decimal == 0:
            #print("warning: decimal part equal zero ! ")
            #print("this will cause division by zero ! ")
            return 0
    return two_complement(decimal, num_bytes)

def fixed_point_vector(vector, dec_bits=8, frac_bits=8):
    return [fixed_point_value(value, dec_bits, frac_bits) for value in vector]
#204,0,65450; 113,65516,0; 65515,65280, 65450

def float_to_fixed(num, num_bytes=2, scale=1<<8):
    max_value = 2 ** (8 * num_bytes) - 1
    num *= scale
    if num < 0:
            num &= max_value
    return num

def invert_bytes(b):
    if len(b) % 2:
        b = "0" + b
    b2 = ""
    for i in range(len(b)-2, -1, -2):
            b2 += b[i:i+2]
    return b2

def invert_words(words):
    w2 = ""
    for w in words:
        temp_w = invert_bytes(hex(w)[2:])
        if len(temp_w) < 8:
           temp_w = temp_w + "0" * (8 - len(temp_w))
        w2 += temp_w
    return w2

def get_jal_call(address):
	address &= 0x3ffffff
	address >>= 2
	address |= 0x0c000000
	return invert_bytes(hex(address)[2:])

def search_value(val_1, val_2, file):
    with open(file, 'rb') as f:
        data = f.read()
        val_1_count = val_2_count = 0
        val_1_loc = loc = 0
        val_2_loc = dist = 0xffffffff
        result  = [0, 0, 0]
        for n in data:
            if n == val_1:
                val_1_count += 1
                val_1_loc = loc
                if abs(val_1_loc - val_2_loc) < dist:
                    dist = abs(val_1_loc - val_2_loc)
                    result[0] = hex(val_2_loc); result[1] = hex(val_1_loc)
                    result[2] = dist
            elif n == val_2:
                val_2_count += 1
                val_2_loc = loc
                if abs(val_2_loc - val_1_loc) < dist:
                    dist = abs(val_1_loc - val_2_loc)
                    result[0] = hex(val_1_loc); result[1] = hex(val_2_loc)
                    result[2] = dist
            loc += 1
        print(val_1_count, val_2_count, loc, result)

def search_value_2(val_1, val_2, file, max_dist=32):
    with open(file, 'rb') as f:
        data = f.read()
        val_1_count = val_2_count = 0
        val_1_loc = loc = 0
        val_2_loc = 0xffffffff
        max_dist = max_dist
        result  = []
        for n in data:
            if n == val_1:
                val_1_count += 1
                val_1_loc = loc
                if abs(val_1_loc - val_2_loc) < max_dist:
                    dist = abs(val_1_loc - val_2_loc)
                    result.append((hex(val_2_loc), hex(val_1_loc), dist))
            elif n == val_2:
                val_2_count += 1
                val_2_loc = loc
                if abs(val_2_loc - val_1_loc) < max_dist:
                    dist = abs(val_1_loc - val_2_loc)
                    result.append((hex(val_1_loc), hex(val_2_loc), dist))
            loc += 1
        print(val_1_count, val_2_count, loc)
        for r in result:
             print(r)

def get_rotation_angles(binary_value):
    binary_value &= 0XFFFFFFFF
    angles = binary_value & 0XFF
    precisionn_1 = ((binary_value & 0XFF00) >> 8)
    precisionn_2 = ((binary_value & 0XFF0000) >> 16)
    precisionn_3 = ((binary_value & 0XFF000000) >> 24)
    angle_1 = (((angles >> 5) << 8) | precisionn_1) * 2
    angle_2 = ((((angles & 0x1C) >> 2) << 8) | precisionn_2) * 2
    angle_3 = (((angles & 0x03) << 8) | precisionn_3) * 4
    scale = (360.0 / 4096.0)
    return (angle_3*scale, angle_2*scale, angle_1*scale)

def get_initial_translation(value):
    return (((value * 0Xf) & 0XFFFFFFFF) >> 7) &0XFFFF

#search_value_2(0x3F, 0xEF, "3F_EF.bin")

#extract_pac_mdt("PL\\P0C.PAC", "extracted\\PL\\ryu_old")

#extract_pac("PL\\P0C.PAC", "extracted\\PL\\ryu")
repack_data("extracted\\PL\\ryu")
#repack_data_and_discard("extracted\\PL\\ryu", "ryu_mod.pac", "0000.bin")

#dump_header("PL\\P0C.PAC")

#entries = get_entries("PL\\P0C.PAC")

#pack_data("extracted\\PL\\ryu", entries, "P0C_header.bin")

##pack_data_and_discard("extracted\\PL\\ryu", entries, "ryu_mod.pac", "P0C_header.bin", "0030.bin")
#pack_data_and_discard_range("extracted\\PL\\ryu", entries, "ryu_mod.pac", "P0C_header.bin", [28, 48])

check_hash("ryu.pac", "PL\\P0C.PAC")

#check_hash("ryu_mod.pac", "PL\\P0C.PAC")


