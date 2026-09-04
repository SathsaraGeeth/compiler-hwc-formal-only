# SystemVerilog compiler

This directory contains part of a SystemVerilog compiler.

This is a work in progress project.

Project page: [Hardware emulation](https://geethsathsara.com/projects/hw_emultion/index.html)

This repository currently hosts the compiler components needed for the formal
verification flow. The simulation and emulation platform components are not
part of the published project.

Read [`docs/specifications`](docs/specifications) to understand the
architecture.

## Main components

| Component | Main task |
| --- | --- |
| `frontend` | Parses and elaborates SystemVerilog and normalizes SVA properties |
| `eir` | Lowers the synthesizable part of SystemVerilog |
| `vir` | Lowers the unsynthesizable part of SystemVerilog |
| `btor2` | Builds a formal transition system from EIR and `frontend::SVA` output |
| `transport` | Communicates between EIR generated code and the VIR runtime |
| `formal` | Selects and runs formal solver backends |

## Directory tree

```text
.
├── compiler
│   ├── btor2
│   │   ├── include
│   │   └── lib
│   ├── CMakeLists.txt
│   ├── eir
│   │   ├── include
│   │   └── lib
│   ├── formal
│   │   ├── engine
│   │   ├── logging
│   │   ├── router
│   │   ├── scheduler
│   │   ├── solver
│   │   ├── task
│   │   └── witness
│   ├── frontend
│   │   ├── include
│   │   └── lib
│   ├── target
│   │   ├── include
│   │   └── lib
│   ├── tools
│   │   ├── include
│   │   ├── main.cpp
│   │   └── src
│   └── vir
│       ├── include
│       ├── lib
│       └── runtime
├── docs
│   ├── development
│   │   └── progress.txt
│   ├── specifications
│   │   ├── 1800-2023.pdf
│   │   ├── btor2.yaml
│   │   ├── eir.yaml
│   │   ├── isa.yaml
│   │   ├── llvm_ir.yaml
│   │   ├── mir.yaml
│   │   ├── program.yaml
│   │   └── vir.yaml
│   └── user_manual
│       ├── reference_scripts
│       └── user_manual.txt
├── emulation
│   ├── platform
│   │   ├── build
│   │   ├── build_transport
│   │   ├── CMakeLists.txt
│   │   ├── cpu
│   │   ├── pynq
│   │   └── README.md
│   ├── rtl
│   │   ├── common
│   │   ├── core
│   │   ├── host
│   │   ├── hw_impl_pynq
│   │   ├── imem
│   │   ├── states
│   │   ├── task_board
│   │   └── top
│   ├── tb
│   │   ├── core_pipeline_tb.sv
│   │   └── pynq_bridge_tb.sv
│   └── transport
│       ├── cpu
│       └── fpga
├── external
└── tests
```

## External dependencies

### Frontend

1. [Slang](https://github.com/MikePopoloski/slang)

### Formal solver backends

1. [Z3](https://github.com/Z3Prover/z3)
2. [Spot](https://spot.lip6.fr/)
3. [Pono](https://github.com/stanford-centaur/pono)
4. [Boolector and BtorMC](https://github.com/Boolector/boolector)
5. [rIC3](https://github.com/gipsyh/rIC3)
6. [AVR](https://github.com/aman-goel/avr)
7. [ABC](https://people.eecs.berkeley.edu/~alanmi/abc/)
8. [Avy](https://github.com/abyss-safety/avy)
9. [SuProve](https://github.com/sterin/super-prove-build)
10. [Yosys](https://yosyshq.net/yosys/), including `yosys-smtbmc`

#### Backend

1. LLVM (dont need for formal verification - only for simulations/uvm on CPU)

#### Misc
1. CMake
2. a C++20 compiler
