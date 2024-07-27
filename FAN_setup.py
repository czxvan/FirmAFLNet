import sys
import os

firm_id = sys.argv[1]

run_src = "FirmAE/scratch/%s/run.sh" % firm_id
scripts_src = "FirmAFLNet_config/%s/scripts" % firm_id
keywords_src = "FirmAFLNet_config/%s/keywords" % firm_id

dst = "image_%s/" %firm_id
seed_src = "FirmAFLNet_config/%s/seed" %(firm_id)
dst_input = "image_%s/inputs/" %firm_id

cmd = []
cmd.append("mkdir -p image_%s/inputs" % firm_id)
cmd.append("cp %s %s" %(run_src, dst))
cmd.append("cp %s %s" %(keywords_src, dst)) 
cmd.append("cp %s %s" %(seed_src, dst_input)) 

for i in range(0, len(cmd)):
	os.system(cmd[i])