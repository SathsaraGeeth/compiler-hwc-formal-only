`ifndef HWC_RUNTIME_SVH
`define HWC_RUNTIME_SVH
module emul_runtime_import #(parameter int WIDTH = 1, parameter int FD = 0)
    (input logic [WIDTH-1:0] value);
endmodule
module emul_runtime_export #(parameter int WIDTH = 1, parameter int FD = 1)
    (input logic [WIDTH-1:0] value);
endmodule
`define runtime_import(SIGNAL, DESCRIPTOR) \
    emul_runtime_import #(.WIDTH($bits(SIGNAL)), .FD(DESCRIPTOR)) \
        emul_import_``SIGNAL (.value(SIGNAL));
`define runtime_export(SIGNAL, DESCRIPTOR) \
    emul_runtime_export #(.WIDTH($bits(SIGNAL)), .FD(DESCRIPTOR)) \
        emul_export_``SIGNAL (.value(SIGNAL));
`define runtime_stdin(SIGNAL) `runtime_import(SIGNAL, 0)
`define runtime_stdout(SIGNAL) `runtime_export(SIGNAL, 1)
`endif
