`define PAGE_SIZE 20

`define REQUEST_SIZE 81
`define INDEX_SIZE 32 // 32
`define VAR_SIZE 10

`define VAR_COUNT (2**`VAR_SIZE) 

`define VAR_END (`VAR_COUNT-1)

`define NODE_SIZE 96

`define SIMPLE_NODE 64
`define SIMPLE_NODE_DEF [`SIMPLE_NODE-1:0]

`define HASH_SIZE 8192
`define HASH_BITS (12 + 1)

`define HASH_TABLE_HASH(e,t,var) (e + t + var) % `HASH_SIZE // (e * 4256249 + t + var) % `HASH_SIZE //

`define VAR_BYTE_SIZE 2

`define NODE_BYTE_SIZE 12

`define BDD_NEXT {12'hfff,16'hffff,3'h1,1'b1}

`define SIMPLE_CACHE_ENTRY_BIT_SIZE 2
`define SIMPLE_CACHE_ENTRY_SIZE 4

`define CACHE_BIT_SIZE 18
`define CACHE_SIZE (2**`CACHE_BIT_SIZE)

`define CACHE_HASH_DEF [`CACHE_BIT_SIZE-1:0]
`define SIMPLE_CACHE_DEF [`SIMPLE_CACHE_ENTRY_BIT_SIZE-1:0]

`define CACHE_HASH(f,g,tag,quantify) ((f + g + tag) ^ quantify) //& (`CACHE_SIZE-1) The number of bits should limit this 
`define SIMPLE_HASH(f,g,tag,quantify) ((^f) * 2 + (^g) + tag + quantify) //& (`SIMPLE_CACHE_ENTRY_SIZE-1))

// Var value if index is constant
`define CONSTANT_VAR 12'hfff

`define CONTEXT_PTR_SIZE 3

`define VAR_TAG_SIZE 4
`define VAR_TAG_DATA (`VAR_SIZE - `VAR_TAG_SIZE)

`define BDD_ZERO 30'h3fffffff
`define BDD_ONE  30'h3ffffffe

`define BDD_INSERT_INTO_CACHE_BIT 2

`define TYPE_SIZE 2
`define BDD_AND 0
`define BDD_EXIST 1
`define BDD_ABSTRACT 2
`define BDD_PERMUTE 3

`define STATE_SIZE 2
`define BDD_APPLY 0
`define BDD_RETURN_0 1
`define BDD_RETURN_1 2
`define BDD_REDUCE 3

`define BDD_STORE_INTO_CACHE 0

// The pos of the last bit
`define TYPE_END (`TYPE_SIZE-1)
`define NODE_END (`NODE_SIZE-1)
`define PAGE_END (`PAGE_SIZE-1)

// Definitions to declare a data type easier
`define REQUEST_DEF [`REQUEST_END:0]
`define VAR_DEF [`VAR_SIZE-1:0]
`define TYPE_DEF [`TYPE_END:0]
`define NODE_DEF [`NODE_END:0]
`define PAGE_DEF [`PAGE_END:0]

// Define the data layout of a request 
`define F_LOWER_END 	0
`define F_UPPER_END 	(`F_LOWER_END + `INDEX_SIZE - 1)
`define G_LOWER_END 	(`F_UPPER_END + 1)
`define G_UPPER_END 	(`G_LOWER_END + `INDEX_SIZE - 1)
`define TYPE_LOWER_END 	(`G_UPPER_END + 1)
`define TYPE_UPPER_END 	(`TYPE_LOWER_END + `TYPE_SIZE - 1)
`define CACHE_END       (`TYPE_UPPER_END + 1)
`define STATE_LOWER_END (`CACHE_END + 1)
`define STATE_UPPER_END (`STATE_LOWER_END + `STATE_SIZE - 1)
`define NEGATE_END 	    (`STATE_UPPER_END + 1)
`define TOP_LOWER_END	(`NEGATE_END + 1)
`define TOP_UPPER_END 	(`TOP_LOWER_END + `VAR_SIZE - 1)
`define QUANTIFY_END    (`TOP_UPPER_END + 1)
`define REQUEST_END 	`QUANTIFY_END

// Extract from node
`define NODE_NEXT(node)  node[29:0]
`define NODE_ELSE(node)  node[61:32]
`define NODE_THEN(node) node[93:64]

// Extract from request
`define REQUEST_RESULT(req) req[`RESULT_UPPER_END:`RESULT_LOWER_END]
`define REQUEST_F(req) req[`F_UPPER_END:`F_LOWER_END]
`define REQUEST_G(req) req[`G_UPPER_END:`G_LOWER_END]
`define REQUEST_TYPE(req) req[`TYPE_UPPER_END:`TYPE_LOWER_END]
`define REQUEST_CACHE(req) req[`CACHE_END]
`define REQUEST_STATE(req) req[`STATE_UPPER_END:`STATE_LOWER_END]
`define REQUEST_NEGATE(req) req[`NEGATE_END]
`define REQUEST_TOP(req) req[`TOP_UPPER_END:`TOP_LOWER_END]
`define REQUEST_QUANTIFY(req) req[`QUANTIFY_END]

`define EQUAL_NEGATE(f,g) (f[29:1] == g[29:1] && f[0] != g[0])

// Memory access constants
`define MA_TYPE_NODE 2'b00
`define MA_TYPE_INSERT 2'b01
`define MA_TYPE_CACHE 2'b10
`define MA_TYPE_VARIABLE 2'b11

`define MA_TYPE_2_FETCH_NODE 3'b000
`define MA_TYPE_2_FETCH_SIMPLE_NODE 3'b001
`define MA_TYPE_2_GET_VAR 3'b010
`define MA_TYPE_2_FETCH_CACHE 3'b011
`define MA_TYPE_2_INSERT_NODE 3'b100
`define MA_TYPE_2_WRITE_NODE_NEXT 3'b101
`define MA_TYPE_2_WRITE_CACHE 3'b110

`define MA_CACHE_START (61*1024*1024)

// Memory Access Data structures
`define MA_REQUEST_$_TYPE_2_END 2
`define MA_REQUEST_$_BDDINDEX_NO_NEGATE_END 31
`define MA_REQUEST_$_DATA_END 127
`define MA_REQUEST_END 127

`define MA_REQUEST_$_TYPE_2(in) in[2:0]
`define MA_REQUEST_$_BDDINDEX_NO_NEGATE(in) in[31:3]
`define MA_REQUEST_$_DATA(in) in[127:32]
`define MA_REQUEST_DEF [127:0]
`define MA_REQUEST_RANGE 127:0

`define MA_INTRO_$_MA_REQUEST_END 127
`define MA_INTRO_$_ID_END 129
`define MA_INTRO_END 129

`define MA_INTRO_$_MA_REQUEST(in) in[127:0]
`define MA_INTRO_$_ID(in) in[129:128]
`define MA_INTRO_DEF [129:0]
`define MA_INTRO_RANGE 129:0

`define INDEX_$_NEGATE_END 0
`define INDEX_$_INDEX_END 9
`define INDEX_$_PAGE_END 29
`define INDEX_END 29

`define INDEX_$_NEGATE(in) in[0:0]
`define INDEX_$_INDEX(in) in[9:1]
`define INDEX_$_PAGE(in) in[29:10]
`define INDEX_DEF [29:0]
`define INDEX_RANGE 29:0

`define MA_STAGE_12_$_MA_REQUEST_END 127
`define MA_STAGE_12_$_ADDRESS_INDEX_END 141
`define MA_STAGE_12_$_ADDRESS_TAG_END 145
`define MA_STAGE_12_$_VARIABLE_END 157
`define MA_STAGE_12_$_VALID_VARIABLE_END 158
`define MA_STAGE_12_$_VALID_ADDRESS_END 159
`define MA_STAGE_12_$_NODECOUNT_END 168
`define MA_STAGE_12_END 168

`define MA_STAGE_12_$_MA_REQUEST(in) in[127:0]
`define MA_STAGE_12_$_ADDRESS_INDEX(in) in[141:128]
`define MA_STAGE_12_$_ADDRESS_TAG(in) in[145:142]
`define MA_STAGE_12_$_VARIABLE(in) in[157:146]
`define MA_STAGE_12_$_VALID_VARIABLE(in) in[158:158]
`define MA_STAGE_12_$_VALID_ADDRESS(in) in[159:159]
`define MA_STAGE_12_$_NODECOUNT(in) in[168:160]
`define MA_STAGE_12_DEF [168:0]
`define MA_STAGE_12_RANGE 168:0

`define MA_STAGE_23_$_DO_VAR_END 0
`define MA_STAGE_23_$_DO_INSERTION_END 1
`define MA_STAGE_23_$_WRITE_END 2
`define MA_STAGE_23_$_ID_END 4
`define MA_STAGE_23_$_VARIABLE_END 16
`define MA_STAGE_23_$_FINAL_ADDRESS_END 42
`define MA_STAGE_23_$_BURSTCOUNT_END 46
`define MA_STAGE_23_$_DATA_END 142
`define MA_STAGE_23_$_BDDINDEX_END 172
`define MA_STAGE_23_END 172

`define MA_STAGE_23_$_DO_VAR(in) in[0:0]
`define MA_STAGE_23_$_DO_INSERTION(in) in[1:1]
`define MA_STAGE_23_$_WRITE(in) in[2:2]
`define MA_STAGE_23_$_ID(in) in[4:3]
`define MA_STAGE_23_$_VARIABLE(in) in[16:5]
`define MA_STAGE_23_$_FINAL_ADDRESS(in) in[42:17]
`define MA_STAGE_23_$_BURSTCOUNT(in) in[46:43]
`define MA_STAGE_23_$_DATA(in) in[142:47]
`define MA_STAGE_23_$_BDDINDEX(in) in[172:143]
`define MA_STAGE_23_DEF [172:0]
`define MA_STAGE_23_RANGE 172:0

`define MA_STAGE_34_$_BDDINDEX_END 29
`define MA_STAGE_34_$_SIZE_END 33
`define MA_STAGE_34_$_ID_END 35
`define MA_STAGE_34_$_WRITE_END 36
`define MA_STAGE_34_END 36

`define MA_STAGE_34_$_BDDINDEX(in) in[29:0]
`define MA_STAGE_34_$_SIZE(in) in[33:30]
`define MA_STAGE_34_$_ID(in) in[35:34]
`define MA_STAGE_34_$_WRITE(in) in[36:36]
`define MA_STAGE_34_DEF [36:0]
`define MA_STAGE_34_RANGE 36:0

`define MA_STAGE_45_$_ID_END 1
`define MA_STAGE_45_$_DATA_END 129
`define MA_STAGE_45_END 129

`define MA_STAGE_45_$_ID(in) in[1:0]
`define MA_STAGE_45_$_DATA(in) in[129:2]
`define MA_STAGE_45_DEF [129:0]
`define MA_STAGE_45_RANGE 129:0

`define CACHE_ENTRY_$_BDDINDEX_F_END 29
`define CACHE_ENTRY_$_QUANTIFY_END 30
`define CACHE_ENTRY_$_UNUSED_1_END 31
`define CACHE_ENTRY_$_BDDINDEX_G_END 61
`define CACHE_ENTRY_$_UNUSED_2_END 63
`define CACHE_ENTRY_$_BDDINDEX_RESULT_END 93
`define CACHE_ENTRY_$_TYPE_END 95
`define CACHE_ENTRY_END 95

`define CACHE_ENTRY_$_BDDINDEX_F(in) in[29:0]
`define CACHE_ENTRY_$_QUANTIFY(in) in[30:30]
`define CACHE_ENTRY_$_UNUSED_1(in) in[31:31]
`define CACHE_ENTRY_$_BDDINDEX_G(in) in[61:32]
`define CACHE_ENTRY_$_UNUSED_2(in) in[63:62]
`define CACHE_ENTRY_$_BDDINDEX_RESULT(in) in[93:64]
`define CACHE_ENTRY_$_TYPE(in) in[95:94]
`define CACHE_ENTRY_DEF [95:0]
`define CACHE_ENTRY_RANGE 95:0
