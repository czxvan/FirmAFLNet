import os
import sys


exit_function_str = \
"""delete_device() {
	echo "Bringing down TAP device..."
	sudo ip link set ${TAPDEV_0} down


	echo "Deleting TAP device ${TAPDEV_0}..."
	sudo tunctl -d ${TAPDEV_0}
	echo "Done!"
}

trap delete_device EXIT

"""

def generate_run_full(image_id):
	script_src = "FirmAE/scratch/%s/run.sh" %image_id
	script_dst = "image_%s/run_fan.sh" %image_id
	file_src = open(script_src)
	file_dst = open(script_dst, "w+")
	for line in file_src.readlines():
		if "TAPDEV_0=" in line:
			file_dst.write(exit_function_str)
			file_dst.write(line)
		elif "Error:" in line or line in exit_function_str:
			continue
		elif "exit" in line:
			file_dst.write("    source ../FirmAE/firmae.config\n")
		elif "QEMU=" in line:
			file_dst.write("QEMU=$(get_spy_qemu ${ARCHEND})\n")
			file_dst.write("PLUGIN=$(get_spy_plugin ${ARCHEND})\n")
			file_dst.write("FUZZ_DIR=$(get_fuzz_dir ${IID})\n")
		elif "Starting emulation of firmware..." in line:
			file_dst.write("IP=$(cat ${WORK_DIR}/ip)\n")
			file_dst.write(line)
			file_dst.write("../afl-fuzz -i ${FUZZ_DIR}/inputs -o ${FUZZ_DIR}/outputs -P HTTP -N tcp://${IP}/80 -m none -QQ\\\n")
			file_dst.write("    -d -q 3 -s 3  -R -W 5 -w 50000 -t 50000  -x keywords \\\n")
			file_dst.write("    ")
		elif "QEMU_AUDIO_DRV=none" in line:
			file_dst.write(line.replace("QEMU_AUDIO_DRV=none ", ""))
		elif "-serial file:${WORK_DIR}/qemu.final.serial.log" in line:
			file_dst.write(line.replace("WORK_DIR", "FUZZ_DIR"))
		elif "-display none" in line:
			file_dst.write(line)
			file_dst.write("    -d plugin \\\n")
			file_dst.write("    -plugin ${PLUGIN} \\\n")
			file_dst.write("    -D qemu_log.txt \\\n")
		elif "| true" in line:
			file_dst.write(line.replace(" | true", ""))
		else:
			file_dst.write(line)
	file_src.close()
	file_dst.close()
	chmod_str = "chmod 777 %s" % script_dst
	os.system(chmod_str)

image_id = sys.argv[1]	
generate_run_full(image_id)