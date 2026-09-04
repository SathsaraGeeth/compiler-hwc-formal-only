/*
 * directory_pkg.sv
 *
 * 2026
 */

package directory_pkg;

import coherency_pkg::*;
import core_pkg::*;
import cache_pkg::*;
import interface_pkg::*;

localparam int DIR_INDEX_W = (NUM_DIR_ENTRIES > 1) ? $clog2(NUM_DIR_ENTRIES) : 1;
localparam int DIR_TAG_W   = PHY_ADDR_W - BYTE_OFFSET_W - DIR_INDEX_W;

typedef struct packed {
    logic [DIR_TAG_W-1:0]     tag;
    logic [DIR_INDEX_W-1:0]   index;
} dir_addr_t;

function automatic dir_addr_t decode_dir_addr(input logic [PHY_ADDR_W-1:0] addr);
    dir_addr_t decoded_addr;
    decoded_addr.tag    = addr[PHY_ADDR_W-1 -: DIR_TAG_W];
    decoded_addr.index  = addr[BYTE_OFFSET_W +: DIR_INDEX_W];
    return decoded_addr;
endfunction: decode_dir_addr

typedef struct packed {
    logic                   valid;
    logic [DIR_TAG_W-1:0]   tag;         // tag of the cache line
    logic [NUM_CACHES-1:0]  sharers;     // caches with a copy of the line in SHARED state
    logic                   owner_valid; // a cache has the line in EXCLUSIVE or MODIFIED
    logic [CACHE_ID_W-1:0]  owner;       // the cache EXCLUSIVE or MODIFIED own this line
} dir_entry_t;

function automatic logic dir_entry_is_hit(
    input dir_entry_t entry,
    input dir_addr_t  addr
);
    return entry.valid && (entry.tag == addr.tag);
endfunction: dir_entry_is_hit

function automatic logic dir_entry_has_other_owner(
    input dir_entry_t                 entry,
    input logic [CACHE_ID_W-1:0]      requester
);
    return entry.owner_valid && (entry.owner != requester);
endfunction: dir_entry_has_other_owner

function automatic logic [NUM_CACHES-1:0] get_other_sharers(
    input dir_entry_t                 entry,
    input logic [CACHE_ID_W-1:0]      requester
);
    return entry.sharers & ~(NUM_CACHES'(1) << requester);
endfunction: get_other_sharers


/* bus lock */
localparam int PORT_W = (NUM_CACHES > 1) ? $clog2(NUM_CACHES) : 1;

typedef struct packed {
    logic                              cross_line;
    logic [$clog2(2*CACHE_LINE_W)-1:0] shift;
    logic [2*CACHE_LINE_W-1:0]         lines;
    logic [2*CACHE_LINE_W/8-1:0]       wstrb;
} bus_lock_data_t;

function automatic bus_lock_data_t get_bus_lock_data(
    input cpu_req_t                req,
    input logic [CACHE_LINE_W-1:0] line_lo,
    input logic [CACHE_LINE_W-1:0] line_hi,
    input logic [XLEN-1:0]         new_value
);
    logic [BYTE_OFFSET_W:0]        bytes;
    logic [BYTE_OFFSET_W:0]        offset;
    logic [2*CACHE_LINE_W-1:0]     mask;
    bus_lock_data_t                data;

    bytes           = (BYTE_OFFSET_W+1)'(1) << req.size;
    offset          = {1'b0, req.addr[BYTE_OFFSET_W-1:0]};
    data.cross_line = (offset + bytes) > (CACHE_LINE_W/8);
    data.shift      = req.addr[BYTE_OFFSET_W-1:0] * 8;
    mask            = ((2*CACHE_LINE_W)'(1) << (8 << req.size)) - 1'b1;
    data.lines      = ({line_hi, line_lo} & ~(mask << data.shift)) |
                      (((2*CACHE_LINE_W)'(new_value) << data.shift) &
                      (mask << data.shift));
    data.wstrb      = '0;

    for (int i = 0; i < 2*(CACHE_LINE_W/8); i++) begin
        if ((i >= offset) && (i < (offset + bytes))) begin
            data.wstrb[i] = 1'b1;
        end
    end
    return data;
endfunction: get_bus_lock_data

//scb
localparam int NUM_LINE_RESERVATIONS = NUM_CACHES * NUM_MSHR_ENTRY;
localparam int LINE_RESERVATION_W    = $clog2(NUM_LINE_RESERVATIONS);

typedef struct packed {
    logic                    valid;
    logic [PHY_ADDR_W-1:0]   addr;
    logic [CACHE_ID_W-1:0]   owner;
    logic [CPU_REQ_ID_W-1:0] id;
} line_reservation_t;

endpackage : directory_pkg
