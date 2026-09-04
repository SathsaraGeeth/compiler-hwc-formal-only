ANVIL_ROOT := ../../anvil/complier/anvil
ANVIL ?=
TOP := $(abspath top/top.anvil)
OUTPUT := $(abspath build/cache_system.sv)
GENERATED := $(OUTPUT).anvil.sv
SOURCES := $(wildcard common/*.anvil cache/*.anvil coherency/*.anvil \
                     main_memory/*.anvil top/*.anvil)

.PHONY: all check clean

all: $(GENERATED)

$(GENERATED): $(SOURCES)
	$(MAKE) check
	mkdir -p build
	@if [ -n "$(ANVIL)" ]; then \
		$(ANVIL) -o $(OUTPUT) $(TOP); \
	else \
		cd $(ANVIL_ROOT) && eval "$$(opam env)" && \
			dune exec anvil -- -o $(OUTPUT) $(TOP); \
	fi

check:
	@if [ -n "$(ANVIL)" ]; then \
		$(ANVIL) -just-check $(TOP); \
	else \
		cd $(ANVIL_ROOT) && eval "$$(opam env)" && \
			dune exec anvil -- -just-check $(TOP); \
	fi

clean:
	$(RM) $(GENERATED)
