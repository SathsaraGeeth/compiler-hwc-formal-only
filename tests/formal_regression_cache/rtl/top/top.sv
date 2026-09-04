/*
 * top.sv
 *
 * 2026
 */

/*
 * Comments:
 * 1. Top level module for the memory subsystem for simulation purposes
 * 2. High level architecture
 *
 *      CPU 0                 CPU 1
 *       |                       |
 *     Cache 0 <-----------> Cache 1
 *        \    \          /     /
 *         \    Directory      /
 *          \                 /
 *                Arbiter
 *                   |
 *              Main Memory
 */

import core_pkg::*;
import cache_pkg::*;
import interface_pkg::*;
import coherency_pkg::*;

module mem_subsystem_top #(
    parameter int DIR_ENTRIES = NUM_DIR_ENTRIES,
    parameter int MEM_LINES   = MEM_SIZE,
    parameter int MEM_DELAY   = 200,
    parameter int CACHE_MSHR_ENTRIES = cache_pkg::NUM_MSHR_ENTRY,
    parameter int CACHE_SET_COUNT    = cache_pkg::NUM_SETS,
    parameter int CACHE_DATA_DEPTH   = cache_pkg::DATA_SRAM_DEPTH
) (
    input  logic       i_clk,
    input  logic       i_rst_n,

    input  cpu_req_t   i_cpu_req       [NUM_CACHES-1:0],
    input  logic       i_cpu_req_valid [NUM_CACHES-1:0],
    output logic       o_cpu_req_ready [NUM_CACHES-1:0],
    output cpu_rsp_t   o_cpu_rsp       [NUM_CACHES-1:0],
    output logic       o_cpu_rsp_valid [NUM_CACHES-1:0],
    input  logic       i_cpu_rsp_ready [NUM_CACHES-1:0]
);
    localparam int NUM_MEM_PORTS = NUM_CACHES + 1;
    localparam int LOCK_MEM_PORT = NUM_CACHES;

    logic w_cpu_req_ready [NUM_CACHES-1:0];
    logic w_cpu_req_allow [NUM_CACHES-1:0];

    for (genvar i = 0; i < NUM_CACHES; i++) begin
        assign o_cpu_req_ready[i] = w_cpu_req_ready[i] && w_cpu_req_allow[i];
    end

    cpu_req_t   w_lock_req              [NUM_CACHES-1:0];
    logic       w_lock_req_valid        [NUM_CACHES-1:0];
    logic       w_lock_req_ready        [NUM_CACHES-1:0];
    cpu_rsp_t   w_lock_rsp              [NUM_CACHES-1:0];
    logic       w_lock_rsp_valid        [NUM_CACHES-1:0];
    logic       w_lock_rsp_ready        [NUM_CACHES-1:0];


    coh_req_t   w_coh_req               [NUM_CACHES-1:0];
    logic       w_coh_req_valid         [NUM_CACHES-1:0];
    logic       w_coh_req_ready         [NUM_CACHES-1:0];
    coh_rsp_t   w_coh_rsp               [NUM_CACHES-1:0];
    logic       w_coh_rsp_valid         [NUM_CACHES-1:0];
    logic       w_coh_rsp_ready         [NUM_CACHES-1:0];
    coh_snp_t   w_coh_snp               [NUM_CACHES-1:0];
    logic       w_coh_snp_valid         [NUM_CACHES-1:0];
    logic       w_coh_snp_ready         [NUM_CACHES-1:0];
    coh_ack_t   w_coh_ack               [NUM_CACHES-1:0];
    logic       w_coh_ack_valid         [NUM_CACHES-1:0];
    logic       w_coh_ack_ready         [NUM_CACHES-1:0];

    cache_fwd_t w_cache_data            [NUM_CACHES-1:0];
    logic       w_cache_data_valid      [NUM_CACHES-1:0];
    logic       w_cache_data_ready      [NUM_CACHES-1:0];

    mem_req_t   w_arb_req               [NUM_MEM_PORTS-1:0];
    logic       w_arb_req_valid         [NUM_MEM_PORTS-1:0];
    logic       w_arb_req_ready         [NUM_MEM_PORTS-1:0];
    mem_rsp_t   w_arb_rsp               [NUM_MEM_PORTS-1:0];
    logic       w_arb_rsp_valid         [NUM_MEM_PORTS-1:0];
    logic       w_arb_rsp_ready         [NUM_MEM_PORTS-1:0];

    mem_req_t   w_mem_req;
    logic       w_mem_req_valid;
    logic       w_mem_req_ready;
    mem_rsp_t   w_mem_rsp;
    logic       w_mem_rsp_valid;
    logic       w_mem_rsp_ready;

    for (genvar i = 0; i < NUM_CACHES; i++) begin: g_cache
        localparam logic [CACHE_ID_W-1:0] CACHE_ID    = i;
        localparam int                    OTHER_CACHE = (i == 0) ? 1 : 0;

        cache #(
            .CACHE_ID     (CACHE_ID),
            .MSHR_ENTRIES (CACHE_MSHR_ENTRIES),
            .CACHE_SETS   (CACHE_SET_COUNT),
            .DATA_DEPTH   (CACHE_DATA_DEPTH)
        ) u_cache (
            .i_clk                    (i_clk                             ),
            .i_rst_n                  (i_rst_n                           ),
            .i_cpu_req                (i_cpu_req            [i]          ),
            .i_cpu_req_VALID          (i_cpu_req_valid      [i] &&
                                       w_cpu_req_allow      [i]          ),
            .o_cpu_req_READY          (w_cpu_req_ready      [i]          ),
            .o_cpu_resp               (o_cpu_rsp            [i]          ),
            .o_cpu_resp_VALID         (o_cpu_rsp_valid      [i]          ),
            .i_cpu_resp_READY         (i_cpu_rsp_ready      [i]          ),
            .o_lock_req               (w_lock_req           [i]          ),
            .o_lock_req_VALID         (w_lock_req_valid     [i]          ),
            .i_lock_req_READY         (w_lock_req_ready     [i]          ),
            .i_lock_rsp               (w_lock_rsp           [i]          ),
            .i_lock_rsp_VALID         (w_lock_rsp_valid     [i]          ),
            .o_lock_rsp_READY         (w_lock_rsp_ready     [i]          ),
            .o_coh_req                (w_coh_req            [i]          ),
            .o_coh_req_VALID          (w_coh_req_valid      [i]          ),
            .i_coh_req_READY          (w_coh_req_ready      [i]          ),
            .i_coh_rsp                (w_coh_rsp            [i]          ),
            .i_coh_rsp_VALID          (w_coh_rsp_valid      [i]          ),
            .o_coh_rsp_READY          (w_coh_rsp_ready      [i]          ),
            .i_coh_snp                (w_coh_snp            [i]          ),
            .i_coh_snp_VALID          (w_coh_snp_valid      [i]          ),
            .o_coh_snp_READY          (w_coh_snp_ready      [i]          ),
            .o_coh_ack                (w_coh_ack            [i]          ),
            .o_coh_ack_VALID          (w_coh_ack_valid      [i]          ),
            .i_coh_ack_READY          (w_coh_ack_ready      [i]          ),
            .o_inter_cache_data       (w_cache_data         [i]          ),
            .o_inter_cache_data_VALID (w_cache_data_valid   [i]          ),
            .i_inter_cache_data_READY (w_cache_data_ready   [i]          ),
            .i_inter_cache_data       (w_cache_data         [OTHER_CACHE]),
            .i_inter_cache_data_VALID (w_cache_data_valid   [OTHER_CACHE]),
            .o_inter_cache_data_READY (w_cache_data_ready   [OTHER_CACHE]),
            .o_mem_req                (w_arb_req            [i]          ),
            .o_mem_req_VALID          (w_arb_req_valid      [i]          ),
            .i_mem_req_READY          (w_arb_req_ready      [i]          ),
            .i_mem_resp               (w_arb_rsp            [i]          ),
            .o_mem_resp_READY         (w_arb_rsp_ready      [i]          ),
            .i_mem_resp_VALID         (w_arb_rsp_valid      [i]          )
        );
    end

    directory #(
        .NUM_DIR_ENTRIES (DIR_ENTRIES)
    ) u_directory (
        .i_clk                  (i_clk                            ),
        .i_rst_n                (i_rst_n                          ),
        .i_cpu_req              (i_cpu_req                        ),
        .i_cpu_req_VALID        (i_cpu_req_valid                  ),
        .i_cpu_req_READY        (w_cpu_req_ready                  ),
        .o_cpu_req_allow        (w_cpu_req_allow                  ),
        .i_cpu_rsp              (o_cpu_rsp                        ),
        .i_cpu_rsp_VALID        (o_cpu_rsp_valid                  ),
        .i_cpu_rsp_READY        (i_cpu_rsp_ready                  ),
        .i_lock_req             (w_lock_req                       ),
        .i_lock_req_VALID       (w_lock_req_valid                 ),
        .o_lock_req_READY       (w_lock_req_ready                 ),
        .o_lock_rsp             (w_lock_rsp                       ),
        .o_lock_rsp_VALID       (w_lock_rsp_valid                 ),
        .i_lock_rsp_READY       (w_lock_rsp_ready                 ),
        .i_cache_req            (w_coh_req                        ),
        .i_cache_req_VALID      (w_coh_req_valid                  ),
        .o_cache_req_READY      (w_coh_req_ready                  ),
        .o_cache_rsp            (w_coh_rsp                        ),
        .o_cache_rsp_VALID      (w_coh_rsp_valid                  ),
        .i_cache_rsp_READY      (w_coh_rsp_ready                  ),
        .o_cache_snp            (w_coh_snp                        ),
        .o_cache_snp_VALID      (w_coh_snp_valid                  ),
        .i_cache_snp_READY      (w_coh_snp_ready                  ),
        .i_cache_ack            (w_coh_ack                        ),
        .i_cache_ack_VALID      (w_coh_ack_valid                  ),
        .o_cache_ack_READY      (w_coh_ack_ready                  ),
        .o_lock_mem_req         (w_arb_req         [LOCK_MEM_PORT]),
        .o_lock_mem_req_VALID   (w_arb_req_valid   [LOCK_MEM_PORT]),
        .i_lock_mem_req_READY   (w_arb_req_ready   [LOCK_MEM_PORT]),
        .i_lock_mem_rsp         (w_arb_rsp         [LOCK_MEM_PORT]),
        .i_lock_mem_rsp_VALID   (w_arb_rsp_valid   [LOCK_MEM_PORT]),
        .o_lock_mem_rsp_READY   (w_arb_rsp_ready   [LOCK_MEM_PORT])
    );

    memory_arbiter #(
        .NUM_PORTS (NUM_MEM_PORTS)
    ) u_memory_arbiter (
        .i_clk           (i_clk          ),
        .i_rst_n         (i_rst_n        ),
        .i_arb_req       (w_arb_req      ),
        .i_arb_req_valid (w_arb_req_valid),
        .o_arb_req_ready (w_arb_req_ready),
        .o_arb_rsp       (w_arb_rsp      ),
        .o_arb_rsp_valid (w_arb_rsp_valid),
        .i_arb_rsp_ready (w_arb_rsp_ready),
        .o_mem_req       (w_mem_req      ),
        .o_mem_req_valid (w_mem_req_valid),
        .i_mem_req_ready (w_mem_req_ready),
        .i_mem_rsp       (w_mem_rsp      ),
        .i_mem_rsp_valid (w_mem_rsp_valid),
        .o_mem_rsp_ready (w_mem_rsp_ready)
    );

    main_memory #(
        .NUM_LINES (MEM_LINES),
        .DELAY     (MEM_DELAY)
    ) u_main_memory (
        .i_clk           (i_clk          ),
        .i_rst_n         (i_rst_n        ),
        .i_mem_req       (w_mem_req      ),
        .i_mem_req_valid (w_mem_req_valid),
        .o_mem_req_ready (w_mem_req_ready),
        .o_mem_rsp       (w_mem_rsp      ),
        .o_mem_rsp_valid (w_mem_rsp_valid),
        .i_mem_rsp_ready (w_mem_rsp_ready)
    );
endmodule: mem_subsystem_top
