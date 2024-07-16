
all:
	mkdir -p ../FirmAE_modify/agent
	g++ -static spy_agent.cpp -o ../FirmAE_modify/agent/spy_agent.arm
	mips-linux-gnu-g++ -static spy_agent.cpp -o ../FirmAE_modify/agent/spy_agent.mipseb
	mipsel-linux-gnu-g++ -static spy_agent.cpp -o ../FirmAE_modify/agent/spy_agent.mipsel
