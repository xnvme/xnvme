import json
import logging as log
import re
from typing import Dict, Iterator, List, Tuple

HardwareSpec = Tuple[str, str, List[str], Dict[str, str]]


def lazy_read(lines) -> Iterator[str]:
    for line in lines:
        yield line.strip()


def read_hardware_linux(cijoe) -> Tuple[int, HardwareSpec]:
    PRODUCT = "product: "
    VENDOR = "vendor: "
    DESCRIPTION = "description: "
    SIZE = "size: "
    NAME = "logical name: "

    def get_motherboard(lines: Iterator[str]) -> str:
        name = ""
        vendor = ""
        for line in lines:
            if PRODUCT in line:
                name = line.replace(PRODUCT, "")
            elif VENDOR in line:
                vendor = line.replace(VENDOR, "")
                break

        return f"{vendor} {name}"

    def get_cpu(lines: Iterator[str]) -> str:
        cpu = ""
        for line in lines:
            if PRODUCT in line:
                cpu = line.replace(PRODUCT, "")
                break

        return cpu

    def get_ram(lines: Iterator[str]) -> str:
        description = ""
        size = ""
        for line in lines:
            if DESCRIPTION in line:
                if "[empty]" in line:
                    return ""
                else:
                    description = line.replace(DESCRIPTION, "")
            elif SIZE in line:
                size = line.replace(SIZE, "")
                break

        return f"{size} {description}"

    def get_nvme(lines: Iterator[str]) -> Tuple[str, str]:
        product = ""
        name = ""
        for line in lines:
            if PRODUCT in line:
                product = line.replace(PRODUCT, "")
            elif NAME in line:
                name = line.replace(NAME, "")
                break

        return product, name

    motherboard = ""
    cpu = ""
    memory = []
    drives = {}

    err, state = cijoe.run("lshw")
    if err:
        log.error(f"failed(lshw); err({err})")
        return err, None
    lines = lazy_read(state.output().strip().split("\n"))

    for line in lines:
        if "*-core" in line:
            motherboard = get_motherboard(lines)
        elif "*-cpu" in line:
            cpu = get_cpu(lines)
        elif "*-bank" in line:
            bank = get_ram(lines)
            if bank:
                memory.append(bank)
        elif "*-nvme" in line:
            product, name = get_nvme(lines)
            drives[name] = product

    return 0, (motherboard, cpu, memory, drives)


def read_hardware_freebsd(cijoe) -> Tuple[int, HardwareSpec]:
    MANUFACTURER = "Manufacturer: "
    PR_NAME = "Product Name: "
    VERSION = "Version: "
    SIZE = "Size: "
    FORM_FACTOR = "Form Factor: "
    TYPE_DETAIL = "Type Detail: "
    SPEED = "Speed: "

    def get_motherboard(lines: Iterator[str]) -> str:
        manufacturer = ""
        pr_name = ""
        for line in lines:
            if MANUFACTURER in line:
                manufacturer = line.replace(MANUFACTURER, "")
            elif PR_NAME in line:
                pr_name = line.replace(PR_NAME, "")
                break

        return f"{manufacturer} {pr_name}"

    def get_cpu(lines: Iterator[str]) -> str:
        cpu = ""
        for line in lines:
            if VERSION in line:
                cpu = line.replace(VERSION, "")
                break

        return cpu

    def get_ram(lines: Iterator[str]) -> str:
        size = ""
        formfactor = ""
        typedetail = ""
        speed = ""

        for line in lines:
            if SIZE in line:
                if "No Module Installed" in line:
                    return ""
                size = line.replace(SIZE, "")
            if FORM_FACTOR in line:
                formfactor = line.replace(FORM_FACTOR, "")
            if TYPE_DETAIL in line:
                typedetail = line.replace(TYPE_DETAIL, "")
            if SPEED in line:
                speed = line.replace(SPEED, "")
                break

        return f"{size} {formfactor} {typedetail} {speed}"

    motherboard = ""
    cpu = ""
    memory = []
    drives = {}

    err, state = cijoe.run("dmidecode")
    if err:
        log.error(f"failed(dmidecode); err({err})")
        return err, None
    lines = lazy_read(state.output().strip().split("\n"))

    for line in lines:
        if "Base Board Information" in line:
            motherboard = get_motherboard(lines)
        elif "Processor Information" in line:
            cpu = get_cpu(lines)
        elif "Memory Device" == line.strip():
            device = get_ram(lines)
            if device:
                memory.append(device)

    err, state = cijoe.run("nvmecontrol devlist")
    if err:
        log.error(f"failed(nvmecontrol devlist); err({err})")
        return err, None
    lines = lazy_read(state.output().strip().split("\n"))
    for line in lines:
        name, description = map(lambda x: x.strip(), line.split(":"))
        size = re.findall(r"\(\d+\w+\)", next(lines))[0]
        drives[f"/dev/{name}"] = f"{description} {size}"

    return 0, (motherboard, cpu, memory, drives)


def read_os_windows(cijoe) -> Tuple[int, Tuple[str, str]]:
    OSNAME = "OS Name:"
    OSVERSION = "OS Version:"

    operating_system = ""
    kernel = ""

    err, state = cijoe.run('systeminfo | grep "^OS"')
    if err:
        log.error(f"failed(systeminfo): err({err})")
        return err, None
    lines = lazy_read(state.output().strip().split("\n"))

    for line in lines:
        if OSNAME in line:
            operating_system = line.replace(OSNAME, "").strip()
        elif OSVERSION in line:
            kernel = line.replace(OSVERSION, "").strip()
            break

    return 0, (operating_system, kernel)


def read_hardware_windows(cijoe) -> HardwareSpec | int:
    CS_PROCESSORS = "CsProcessors"
    CS_MANUFACTURER = "CsManufacturer"
    CS_MODEL = "CsModel"
    SPEED = "Speed"
    FORMFACTOR = "FormFactor"
    CAPACITY = "Total Physical Memory:"

    def get_motherboard_cpu() -> Tuple[int, Tuple[str, str]]:
        manufacturer = ""
        model = ""
        cpu = ""

        err, state = cijoe.run('powershell "get-computerinfo"')
        if err:
            log.error(f"failed(get-computerinfo): err({err})")
            return err, None
        lines = lazy_read(state.output().strip().split("\n"))

        for line in lines:
            if CS_PROCESSORS in line:
                cpu = line.replace(CS_PROCESSORS, "").strip()[1:].strip()
                cpu = re.sub(r"\{|\}", "", cpu)
            elif CS_MANUFACTURER in line:
                manufacturer = line.replace(CS_MANUFACTURER, "").strip()[1:].strip()
            elif CS_MODEL in line:
                model = line.replace(CS_MODEL, "").strip()[1:].strip()

            # if all values have been found, break the loop
            if len(manufacturer) and len(model) and len(cpu):
                break

        return 0, (f"{manufacturer} {model}", cpu)

    def get_memory() -> Tuple[int, str]:
        speed = ""
        capacity = ""
        formfactor = ""

        err, state = cijoe.run('powershell "get-wmiobject Win32_PhysicalMemory"')
        if err:
            log.error(f"failed(Get-WmiObject Win32_PhysicalMemory): err({err})")
            return err, None
        lines = lazy_read(state.output().strip().split("\n"))

        formfactor2name = {
            "0": "Unknown",
            "1": "Other",
            "2": "SIP",
            "3": "DIP",
            "4": "ZIP",
            "5": "SOJ",
            "6": "Proprietary",
            "7": "SIMM",
            "8": "DIMM",
            "9": "TSOP",
            "10": "PGA",
            "11": "RIMM",
            "12": "SODIMM",
            "13": "SRIMM",
            "14": "SMD",
            "15": "SSMP",
            "16": "QFP",
            "17": "TQFP",
            "18": "SOIC",
            "19": "LCC",
            "20": "PLCC",
            "21": "BGA",
            "22": "FPBGA",
            "23": "LGA",
            "24": "FB - DIMM",
        }

        for line in lines:
            if SPEED in line:
                speed = line.replace(SPEED, "").strip()[1:].strip()
            elif FORMFACTOR in line:
                formfactor = line.replace(FORMFACTOR, "").strip()[1:].strip()
                formfactor = formfactor2name.get(formfactor, "Unknown")

            # if all values have been found, break the loop
            if len(speed) and len(formfactor):
                break

        err, state = cijoe.run(
            f'powershell "systeminfo | findstr /B /C:\\"{CAPACITY}\\""'
        )
        if err:
            log.error(f"failed(systeminfo): err({err})")
            return err, None
        capacity = (
            state.output()
            .strip()
            .replace(CAPACITY, "")
            .replace(".", "")
            .strip()
            .split()
        )
        if "MB" in capacity:
            capacity = int(capacity[0]) // 1000
        elif "GB" in capacity:
            capacity = capacity[0]

        return 0, f"{capacity} GB {formfactor} {speed} MHz"

    def get_drives() -> Tuple[int, Dict[str, str]]:
        drives = {}

        err, state = cijoe.run(
            'powershell "get-disk | format-table Number, FriendlyName"'
        )
        if err:
            log.error(f"failed(get-disk): err({err})")
            return err, None
        lines = lazy_read(state.output().strip().split("\n")[2:])

        for line in lines:
            match = re.search(r"^\s*(\d+) (.*)$", line)
            if match is None:
                continue
            num, drive = match.groups(0)
            drives[num] = drive

        return 0, drives

    err, motherboard_and_cpu = get_motherboard_cpu()
    if err:
        return err, None

    motherboard, cpu = motherboard_and_cpu

    err, memory = get_memory()
    if err:
        return err, None

    err, drives = get_drives()
    if err:
        return err, None

    return 0, (motherboard, cpu, memory, drives)


def main(args, cijoe, step):
    operating_system = ""
    kernel = ""
    motherboard = ""
    cpu = ""
    memory = []
    drives = {}

    bios = ""
    bios_version = ""

    shell = cijoe.config.options.get("cijoe", {}).get("run", {}).get("shell", "sh")
    os = cijoe.config.options.get("os", {}).get("name", "")

    if os == "windows":
        err, os = read_os_windows(cijoe)
        if err:
            return err
        operating_system, kernel = os

        err, hardware = read_hardware_windows(cijoe)
        if err:
            return err
        motherboard, cpu, memory, drives = hardware

        # Get BIOS info
        err, state = cijoe.run('powershell "wmic bios get biosversion"')
        if err:
            log.error(f"failed(wmic bios get biosversion); err({err})")
            return err
        # Remove empty line in between bios and version
        bios_list = state.output().strip().split("\n")[-1]
        bios_list = bios_list[2:-2].split('", "')
        bios = bios_list[2]
        bios_version = bios_list[1]

    else:
        # Get operating system
        err, state = cijoe.run('cat /etc/os-release | grep "PRETTY_NAME"')
        if err:
            log.error(f"failed(cat /etc/os-release); err({err})")
            return err
        # Output format is PRETTY_NAME="Debian GNU/Linux 12 (bookworm)"
        operating_system = state.output().strip().split('"')[1]

        # Get Kernel
        err, state = cijoe.run("hostname")
        if err:
            log.error(f"failed(hostname); err({err})")
            return err
        hostname = state.output().strip()

        err, state = cijoe.run("uname -a")
        if err:
            log.error(f"failed(uname -a); err({err})")
            return err
        # Remove the hostname from the output
        kernel = " ".join([s for s in state.output().strip().split() if s != hostname])

        # Get Hardware specs
        if shell == "csh":
            err, hardware = read_hardware_freebsd(cijoe)
        else:
            err, hardware = read_hardware_linux(cijoe)

        if err:
            return err

        motherboard, cpu, memory, drives = hardware

        # Get BIOS

        prefix = "" if shell == "csh" else "sudo"

        err, state = cijoe.run(f"{prefix} dmidecode -s bios-vendor")
        if err:
            log.error(f"failed(sudo dmidecode -s bios-vendor); err({err})")
            return err
        bios = state.output().strip()

        err, state = cijoe.run(f"{prefix} dmidecode -s bios-version")
        if err:
            log.error(f"failed(sudo dmidecode -s bios-version); err({err})")
            return err
        bios_version = state.output().strip()

    sysinfo = {
        "operating_system": operating_system,
        "kernel": kernel,
        "motherboard": motherboard,
        "cpu": cpu,
        "memory": memory,
        "drives": drives,
    }

    biosinfo = {"bios": bios, "bios_version": bios_version}

    out = json.dumps(sysinfo)
    with open(args.output / "artifacts" / "sysinfo.json", "x") as f:
        f.write(out)
    out = json.dumps(biosinfo)
    with open(args.output / "artifacts" / "biosinfo.json", "x") as f:
        f.write(out)
