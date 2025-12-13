import argparse
import re

#!/usr/bin/env python3
"""
Script to generate a disk.sh file that creates virtual drives
"""

def parse_drive_argument(arg):
    """
    Parse drive argument in format like '10MBD' (10 MB Drive) or '4MBF' (4 MB Floppy)
    Returns tuple: (size_in_mb, drive_type) where drive_type is 'D' or 'F'
    """
    match = re.match(r'(\d+)(MB)([DF])', arg, re.IGNORECASE)
    if not match:
        raise argparse.ArgumentTypeError(f"Invalid format: {arg}. Use format like '10MBD' or '4MBF'")
    
    size, unit, drive_type = match.groups()
    return (int(size), drive_type.upper())

def generate_run_script(output_file="disk.sh", drives=None, floppies=None, lib_path=None):
    """
    Generate a disk.sh script that creates drives
    
    Args:
        output_file: Name of the output shell script
        drives: List of (size_in_mb, name) tuples
        floppies: List of floppy sizes in MB
    """
    if drives is None:
        drives = []
    if floppies is None:
        floppies = []
    
    script_content = "#!/bin/bash\n\n"
    script_content += "# Auto-generated script to create virtual drives (uses lib.sh if provided)\n\n"
    if lib_path:
        script_content += f"source \"{lib_path}\"\n"
    
    for i, size_mb in enumerate(drives, 1):
        drive_name = f"drive_{i}"
        script_content += ("log_build 'Creating {drive} with size {size}MB...'\n" if lib_path else "echo 'Creating {drive} with size {size}MB...'\n").format(drive=drive_name, size=size_mb)
        script_content += f"dd if=/dev/zero of={drive_name}.img bs=1M count={size_mb}\n"
        script_content += ("log_build '{drive} created successfully'\n\n" if lib_path else "echo '{drive} created successfully'\n\n").format(drive=drive_name)
    
    for i, size_mb in enumerate(floppies, 1):
        script_content += ("log_build 'Creating floppy_{idx} with size {size}MB...'\n" if lib_path else "echo 'Creating floppy_{idx} with size {size}MB...'\n").format(idx=i, size=size_mb)
        script_content += f"dd if=/dev/zero of=floppy_{i}.img bs=1M count={size_mb}\n"
        script_content += ("log_build 'floppy_{idx} created successfully'\n\n" if lib_path else "echo 'floppy_{idx} created successfully'\n\n").format(idx=i)
    
    with open(output_file, 'w') as f:
        f.write(script_content)
    
    print(f"Generated {output_file}")

def generate_disk_dir(output_file="disk.dir", drives=None, floppies=None):
    """
    Generate a disk.dir file mapping all drives and their types
    
    Args:
        output_file: Name of the output directory file
        drives: List of drive sizes in MB
        floppies: List of floppy sizes in MB
    """
    if drives is None:
        drives = []
    if floppies is None:
        floppies = []
    
    dir_content = "# Drive Directory Mapping\n\n"
    
    for i, size_mb in enumerate(drives, 1):
        dir_content += f"drive_{i}.img {size_mb}MB D\n"
    
    for i, size_mb in enumerate(floppies, 1):
        dir_content += f"floppy_{i}.img {size_mb}MB F\n"
    
    with open(output_file, 'w') as f:
        f.write(dir_content)
    
    print(f"Generated {output_file}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate a script to create virtual drives")
    parser.add_argument("drives", nargs="*", type=parse_drive_argument,
                        help="Drives in format: 10MBD, 512MBD, etc.")
    parser.add_argument("--lib-path", dest="lib_path", default=None,
                        help="Absolute path to SCRIPTS/lib.sh to source for logging")

    args = parser.parse_args()

    drives = [size for size, dtype in args.drives if dtype == 'D']
    floppies = [size for size, dtype in args.drives if dtype == 'F']

    generate_run_script("disk.sh", drives, floppies, lib_path=args.lib_path)
    generate_disk_dir("disk.dir", drives, floppies)