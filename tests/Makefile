EMUL := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
COMPILER := $(EMUL)/compiler
BUILD := $(COMPILER)/build
HWC := $(BUILD)/hwc

EXAMPLE ?= adder
EXAMPLE_DIR := $(EMUL)/tests/$(EXAMPLE)
SCRIPT ?= run_sim.tcl

.PHONY: build run sim cpu fpga uvm formal cover

build:
	cmake -S $(COMPILER) -B $(BUILD) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD) --target hwc -j

run: build
	@test -f "$(EXAMPLE_DIR)/$(SCRIPT)" || \
		{ echo "missing $(EXAMPLE_DIR)/$(SCRIPT)"; exit 2; }
	$(HWC) -f $(EXAMPLE_DIR)/$(SCRIPT)

sim: SCRIPT=run_sim.tcl
sim: run

cpu: SCRIPT=run_cpu.tcl
cpu: run

fpga: SCRIPT=run_emul.tcl
fpga: build
	@test -n "$(HWC_FPGA_SERVER)" || \
		{ echo "set HWC_FPGA_SERVER=HOST:PORT"; exit 2; }
	@test -f "$(EXAMPLE_DIR)/$(SCRIPT)" || \
		{ echo "missing $(EXAMPLE_DIR)/$(SCRIPT)"; exit 2; }
	HWC_FPGA_SERVER="$(HWC_FPGA_SERVER)" \
		$(HWC) -f $(EXAMPLE_DIR)/$(SCRIPT)

uvm: SCRIPT=run_uvm.tcl
uvm: run

formal: SCRIPT=run_formal.tcl
formal: run

cover: SCRIPT=run_cover.tcl
cover: run
