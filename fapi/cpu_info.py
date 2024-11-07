import subprocess
import platform


def get_cpu_info():
    # Determine the OS
    os_type = platform.system()

    if os_type == "Linux":
        # Get CPU name
        cpu_name = subprocess.check_output(
            "lscpu | grep 'Model name'", shell=True)
        cpu_name = cpu_name.decode().strip().split(": ")[1]

        # Get the number of cores
        num_cores = subprocess.check_output("nproc", shell=True)
        num_cores = num_cores.decode().strip()

    elif os_type == "Darwin":  # macOS
        # Get CPU name
        cpu_name = subprocess.check_output(
            "sysctl -n machdep.cpu.brand_string", shell=True)
        cpu_name = cpu_name.decode().strip()

        # Get the number of cores
        num_cores = subprocess.check_output("sysctl -n hw.ncpu", shell=True)
        num_cores = num_cores.decode().strip()

    else:
        return "Unsupported OS"

    return f"{cpu_name} ({num_cores} cores)".strip()
