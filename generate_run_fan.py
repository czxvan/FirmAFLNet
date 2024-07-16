import os
import sys

def generate_run_full(image_id):
	script_src = "FirmAE/scratch/%s/run.sh" %image_id
	script_dst = "FirmAE/scratch/%s/run_fan.sh" %image_id
	file_src = open(script_src)
	file_dst = open(script_dst, "w+")
	for line in file_src.readlines():
		if "Starting emulation of firmware..." in line:
			file_dst.write("IP=$(cat ${WORKDIR}/ip)\n")
			file_dst.write(line)
			file_dst.write("../afl-fuzz -i ${WORK_DIR}/inputs -o ${WORK_DIR}/outputs -P HTTP -N ${IP} -m 4096M -QQ\n")
			file_dst.write("    -d -q 3 -s 3  -R -W 5 -w 50000 -t 50000\n")
			file_dst.write("    ")
		elif "-display none" in line:
			file_dst.write(line)
			file_dst.write("    -d plugin\n")
			file_dst.write("    -plugin libaflspy.so")
		else:
			file_dst.write(line)
	file_src.close()
	file_dst.close()
	chmod_str = "chmod 777 %s" % script_dst
	os.system(chmod_str)

image_id = sys.argv[1]	
generate_run_full(image_id)