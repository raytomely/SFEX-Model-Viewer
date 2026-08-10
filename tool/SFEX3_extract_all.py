#!/usr/bin/env python3
"""
from: https://gist.github.com/Armonte/9ef99098923392cf0e5758153a48b631

Street Fighter EX3 (PS2) Complete Extractor
============================================

Author: Armonté / COPKILLER4FUN

Extracts everything in one go:
1. Decodes ARIKA.DAT (encrypted TOC)
2. Extracts all files from PS2.DAT
3. Recursively extracts all PAC/MDT archives

Supports two PAC/MDT formats:
1. Named format: 12-byte filename + 4-byte offset per entry
   - Used by: fintex.pac, all .mdt files
   - Files extracted with original names

2. Indexed format: Array of 4-byte offsets (no filenames)
   - Used by: pXX.pac (character data)
   - Files extracted as NNNN.bin (slot number)

Usage:
    python extract_sfex3_all.py ARIKA.DAT PS2.DAT ./output
"""

import struct
import os
import sys

def decode_toc(data: bytes) -> bytes:
    """Decode SFEX3 ARIKA.DAT TOC."""
    header = data[:16]
    result = bytearray(header)

    for offset in range(16, len(data)):
        i = (offset - 16) % 16
        byte = data[offset]
        byte = ((byte >> 4) | (byte << 4)) & 0xFF
        byte = ~byte & 0xFF
        byte = (byte - header[i]) & 0xFF
        result.append(byte)

    return bytes(result)

def parse_toc(decoded: bytes):
    """Parse decoded TOC and return file entries."""
    base_off = struct.unpack_from('<I', decoded, 0x14)[0]

    entries = []
    pos = 0x20
    while pos + 32 <= len(decoded):
        name_bytes = decoded[pos:pos+20]
        sector_off = struct.unpack_from('<I', decoded, pos + 20)[0]
        sector_cnt = struct.unpack_from('<I', decoded, pos + 24)[0]
        file_size = struct.unpack_from('<I', decoded, pos + 28)[0]

        if file_size >= 0x80000000:
            break

        name = name_bytes.split(b'\x00')[0].decode('latin-1', errors='replace')
        actual_offset = (sector_off - base_off) * 0x800

        if name:
            entries.append({
                'name': name,
                'offset': actual_offset,
                'size': file_size
            })

        pos += 32

    return entries

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


def extract_pac_mdt(data: bytes, output_dir: str, archive_name: str):
    """Extract files from a PAC or MDT archive."""
    file_size = len(data)

    if len(data) < 16:
        return 0

    # Detect format: Check if first 12 bytes look like a filename
    first_12_bytes = data[0:12]
    is_named_format = is_valid_filename_bytes(first_12_bytes)

    if is_named_format:
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

def is_pac_or_mdt(filename: str) -> bool:
    """Check if file is a PAC or MDT archive."""
    ext = os.path.splitext(filename)[1].lower()
    return ext in ['.pac', '.mdt']

def main():
    if len(sys.argv) < 4:
        print("Street Fighter EX3 Complete Extractor")
        print("=" * 40)
        print()
        print("Usage:")
        print(f"  {sys.argv[0]} ARIKA.DAT PS2.DAT <output_dir>")
        print()
        print("This will:")
        print("  1. Decode the encrypted ARIKA.DAT TOC")
        print("  2. Extract all 582 files from PS2.DAT")
        print("  3. Extract contents of all PAC/MDT archives")
        return

    arika_path = sys.argv[1]
    ps2_path = sys.argv[2]
    output_dir = sys.argv[3]

    # Step 1: Decode ARIKA.DAT
    print("=" * 60)
    print("STEP 1: Decoding ARIKA.DAT...")
    print("=" * 60)

    with open(arika_path, 'rb') as f:
        arika_data = f.read()

    decoded = decode_toc(arika_data)
    entries = parse_toc(decoded)
    print(f"Found {len(entries)} files in TOC")

    # Step 2: Extract from PS2.DAT
    print()
    print("=" * 60)
    print("STEP 2: Extracting files from PS2.DAT...")
    print("=" * 60)

    os.makedirs(output_dir, exist_ok=True)

    pac_mdt_files = []  # Track PAC/MDT files for later extraction

    with open(ps2_path, 'rb') as ps2:
        for i, entry in enumerate(entries):
            # Create directory structure
            out_path = os.path.join(output_dir, entry['name'])
            os.makedirs(os.path.dirname(out_path), exist_ok=True)

            # Extract file
            ps2.seek(entry['offset'])
            data = ps2.read(entry['size'])

            with open(out_path, 'wb') as f:
                f.write(data)

            # Track PAC/MDT files
            if is_pac_or_mdt(entry['name']):
                pac_mdt_files.append({
                    'path': out_path,
                    'name': entry['name'],
                    'data': data
                })

            if (i + 1) % 100 == 0:
                print(f"  Extracted {i+1}/{len(entries)} files...")

    print(f"  Extracted {len(entries)} files")
    print(f"  Found {len(pac_mdt_files)} PAC/MDT archives to process")

    # Step 3: Extract PAC/MDT contents
    print()
    print("=" * 60)
    print("STEP 3: Extracting PAC/MDT archive contents...")
    print("=" * 60)

    total_inner_files = 0

    for pm in pac_mdt_files:
        # Create output directory next to the archive
        base_name = os.path.splitext(pm['path'])[0]
        inner_output = base_name + "_contents"

        extracted = extract_pac_mdt(pm['data'], inner_output, pm['name'])

        if extracted > 0:
            print(f"  {pm['name']}: {extracted} files -> {os.path.basename(inner_output)}/")
            total_inner_files += extracted

    print()
    print("=" * 60)
    print("EXTRACTION COMPLETE!")
    print("=" * 60)
    print(f"  Primary files: {len(entries)}")
    print(f"  PAC/MDT archives processed: {len(pac_mdt_files)}")
    print(f"  Inner files extracted: {total_inner_files}")
    print(f"  Output directory: {output_dir}")

if __name__ == '__main__':
    main()
