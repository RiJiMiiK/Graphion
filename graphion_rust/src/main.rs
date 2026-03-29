use std::env;
use std::hint::black_box;
use std::time::Instant;

#[derive(Clone, Copy)]
struct VmInsn {
    op: u8,
    a: u8,
    b: u8,
    imm: i32,
}

const OP_MOV_IMM: u8 = 2;
const OP_ADD: u8 = 3;
const OP_MOV: u8 = 4;
const OP_LOAD_CONST: u8 = 5;
const OP_LOAD_GLOBAL: u8 = 6;
const OP_STORE_GLOBAL: u8 = 7;
const OP_HALT: u8 = 1;
const OP_FRONTIER_FILTER_LT_IMM: u8 = 34;
const OP_FRONTIER_MAP_ADD_IMM: u8 = 35;
const OP_FRONTIER_REDUCE_SUM: u8 = 36;
const OP_FRONTIER_SWAP: u8 = 37;
const OP_BFS_LEVELS: u8 = 16;
const OP_INCIDENT_COUNT: u8 = 17;
const OP_HYPEREDGE_SIZE: u8 = 18;
const OP_INCIDENT_SUM: u8 = 19;
const OP_BFS_LEVEL_COUNT: u8 = 21;
const OP_BFS_ORDER: u8 = 22;
const OP_NEIGHBOR_WEIGHT_SUM: u8 = 42;
const OP_NEIGHBOR_ATTR_SUM: u8 = 43;

struct CsrGraph<'a> {
    offsets: &'a [u32],
    neighbors: &'a [u32],
}

fn recommend_frontier_mode(node_count: usize, edge_count: usize, frontier_len: usize, frontier_neighbor_work: usize) -> &'static str {
    if node_count == 0 {
        return "sparse";
    }
    if frontier_len * 100 >= node_count * 20 {
        return "dense";
    }
    if edge_count > 0 && frontier_neighbor_work * 100 >= edge_count * 35 {
        return "dense";
    }
    "sparse"
}

struct HyperGraph<'a> {
    node_offsets: &'a [u32],
    node_hyperedges: &'a [u32],
    hyperedge_offsets: &'a [u32],
}

#[derive(Clone, Copy)]
enum VmValue {
    None,
    Int(i64),
    Float(f64),
    Bool(bool),
    String(&'static str),
}

fn value_as_int(value: VmValue) -> i64 {
    match value {
        VmValue::Int(number) => number,
        _ => panic!("type mismatch"),
    }
}

fn text_len(value: VmValue) -> usize {
    match value {
        VmValue::None => 5,
        VmValue::Int(number) => format!("{number}\n").len(),
        VmValue::Float(number) => format!("{number}\n").len(),
        VmValue::Bool(true) => 5,
        VmValue::Bool(false) => 6,
        VmValue::String(text) => text.len() + 1,
    }
}

#[inline(never)]
fn hypergraph_incident_sum(hg: &HyperGraph<'_>, node: usize) -> u64 {
    let b = hg.node_offsets[node] as usize;
    let e = hg.node_offsets[node + 1] as usize;
    let mut total = 0_u64;
    for &h in &hg.node_hyperedges[b..e] {
        total = total.wrapping_add(u64::from(h));
    }
    total
}

#[inline(never)]
fn hypergraph_hyperedge_node_sum(
    hg: &HyperGraph<'_>,
    hyperedge_nodes: &[u32],
    hyperedge: usize,
) -> u64 {
    let b = hg.hyperedge_offsets[hyperedge] as usize;
    let e = hg.hyperedge_offsets[hyperedge + 1] as usize;
    let mut total = 0_u64;
    for &node in &hyperedge_nodes[b..e] {
        total = total.wrapping_add(u64::from(node));
    }
    total
}

fn vm_dispatch(iterations: u64) {
    let program = [
        VmInsn { op: OP_LOAD_CONST, a: 0, b: 0, imm: 0 },
        VmInsn { op: OP_STORE_GLOBAL, a: 0, b: 0, imm: 0 },
        VmInsn { op: OP_LOAD_GLOBAL, a: 1, b: 0, imm: 0 },
        VmInsn { op: OP_LOAD_CONST, a: 2, b: 0, imm: 1 },
        VmInsn { op: OP_ADD, a: 1, b: 2, imm: 0 },
        VmInsn { op: OP_STORE_GLOBAL, a: 1, b: 0, imm: 0 },
        VmInsn { op: OP_LOAD_GLOBAL, a: 3, b: 0, imm: 0 },
        VmInsn { op: OP_MOV, a: 4, b: 3, imm: 0 },
        VmInsn { op: OP_LOAD_CONST, a: 5, b: 0, imm: 2 },
        VmInsn { op: OP_ADD, a: 4, b: 5, imm: 0 },
        VmInsn { op: OP_STORE_GLOBAL, a: 4, b: 0, imm: 0 },
        VmInsn { op: OP_LOAD_CONST, a: 6, b: 0, imm: 3 },
        VmInsn { op: OP_STORE_GLOBAL, a: 6, b: 0, imm: 1 },
        VmInsn { op: OP_LOAD_GLOBAL, a: 7, b: 0, imm: 0 },
        VmInsn { op: OP_MOV, a: 8, b: 7, imm: 0 },
        VmInsn { op: OP_HALT, a: 0, b: 0, imm: 0 },
    ];
    let const_pool = [
        VmValue::Int(1),
        VmValue::Int(2),
        VmValue::Int(10),
        VmValue::String("graphion"),
    ];

    let start = Instant::now();
    let mut checksum: u64 = 0;
    for _ in 0..iterations {
        let mut regs = [VmValue::Int(0); 16];
        let mut globals = [VmValue::Int(0), VmValue::None];
        let mut pc = 0usize;
        loop {
            let insn = program[pc];
            pc += 1;
            match insn.op {
                OP_MOV_IMM => regs[insn.a as usize] = VmValue::Int(i64::from(insn.imm)),
                OP_ADD => {
                    let lhs = value_as_int(regs[insn.a as usize]);
                    let rhs = value_as_int(regs[insn.b as usize]);
                    regs[insn.a as usize] = VmValue::Int(lhs.wrapping_add(rhs));
                }
                OP_MOV => regs[insn.a as usize] = regs[insn.b as usize],
                OP_LOAD_CONST => regs[insn.a as usize] = const_pool[insn.imm as usize],
                OP_LOAD_GLOBAL => regs[insn.a as usize] = globals[insn.imm as usize],
                OP_STORE_GLOBAL => globals[insn.imm as usize] = regs[insn.a as usize],
                OP_HALT => break,
                _ => panic!("invalid opcode"),
            }
        }
        if let VmValue::String(text) = globals[1] {
            black_box(text);
        } else {
            panic!("global string lost");
        }
        checksum = checksum.wrapping_add(value_as_int(regs[8]) as u64);
    }
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let mips = (iterations as f64 * program.len() as f64 / secs) / 1_000_000.0;
    let ns_per_instruction = (secs * 1_000_000_000.0) / (iterations as f64 * program.len() as f64);
    let ns_per_iteration = (secs * 1_000_000_000.0) / iterations as f64;
    println!(
        "{{\"benchmark\":\"vm_dispatch\",\"iterations\":{},\"instructions_per_iteration\":{},\"seconds\":{:.6},\"mips\":{:.3},\"ns_per_instruction\":{:.3},\"ns_per_iteration\":{:.3},\"typed_value_ops_per_iteration\":15,\"checksum\":{}}}",
        iterations,
        program.len(),
        secs,
        mips,
        ns_per_instruction,
        ns_per_iteration,
        checksum
    );
}

fn scalar_values_print(iterations: u64) {
    let program = [
        VmInsn { op: OP_LOAD_CONST, a: 0, b: 0, imm: 0 },
        VmInsn { op: OP_STORE_GLOBAL, a: 0, b: 0, imm: 0 },
        VmInsn { op: OP_LOAD_CONST, a: 1, b: 0, imm: 1 },
        VmInsn { op: OP_STORE_GLOBAL, a: 1, b: 0, imm: 1 },
        VmInsn { op: OP_LOAD_CONST, a: 2, b: 0, imm: 2 },
        VmInsn { op: OP_STORE_GLOBAL, a: 2, b: 0, imm: 2 },
        VmInsn { op: OP_LOAD_CONST, a: 3, b: 0, imm: 3 },
        VmInsn { op: OP_STORE_GLOBAL, a: 3, b: 0, imm: 3 },
        VmInsn { op: OP_LOAD_GLOBAL, a: 4, b: 0, imm: 0 },
        VmInsn { op: OP_STORE_GLOBAL, a: 4, b: 0, imm: 4 },
        VmInsn { op: OP_HALT, a: 0, b: 0, imm: 0 },
    ];
    let const_pool = [
        VmValue::Int(42),
        VmValue::Float(3.5),
        VmValue::String("graphion"),
        VmValue::Bool(true),
    ];

    let start = Instant::now();
    let mut checksum: u64 = 0;
    for _ in 0..iterations {
        let mut regs = [VmValue::Int(0); 16];
        let mut globals = [VmValue::None, VmValue::None, VmValue::None, VmValue::None, VmValue::None];
        let mut pc = 0usize;
        loop {
            let insn = program[pc];
            pc += 1;
            match insn.op {
                OP_LOAD_CONST => regs[insn.a as usize] = const_pool[insn.imm as usize],
                OP_STORE_GLOBAL => globals[insn.imm as usize] = regs[insn.a as usize],
                OP_LOAD_GLOBAL => regs[insn.a as usize] = globals[insn.imm as usize],
                OP_HALT => break,
                _ => panic!("invalid opcode"),
            }
        }
        checksum = checksum
            .wrapping_add(text_len(VmValue::Int(7)) as u64)
            .wrapping_add(text_len(VmValue::String("raw")) as u64)
            .wrapping_add(text_len(VmValue::Bool(false)) as u64)
            .wrapping_add(text_len(globals[0]) as u64)
            .wrapping_add(text_len(globals[1]) as u64)
            .wrapping_add(text_len(globals[2]) as u64)
            .wrapping_add(text_len(globals[3]) as u64)
            .wrapping_add(text_len(globals[4]) as u64);
    }
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let ns_per_iteration = (secs * 1_000_000_000.0) / iterations as f64;
    let mops = (iterations as f64 * 13.0 / secs) / 1_000_000.0;
    println!(
        "{{\"benchmark\":\"scalar_values_print\",\"iterations\":{},\"source_ops_per_iteration\":13,\"print_ops_per_iteration\":8,\"seconds\":{:.6},\"mops\":{:.3},\"ns_per_iteration\":{:.3},\"checksum\":{}}}",
        iterations,
        secs,
        mops,
        ns_per_iteration,
        checksum
    );
}

fn frontier_primitives(iterations: u64) {
    const FRONTIER_INPUT_LEN: usize = 64;
    const FRONTIER_CAPACITY: usize = 64;
    const FRONTIER_ITEMS_PER_ITERATION: usize = 128;
    const PROGRAM: [VmInsn; 6] = [
        VmInsn { op: OP_FRONTIER_FILTER_LT_IMM, a: 0, b: 0, imm: 32 },
        VmInsn { op: OP_FRONTIER_SWAP, a: 1, b: 0, imm: 0 },
        VmInsn { op: OP_FRONTIER_MAP_ADD_IMM, a: 2, b: 0, imm: 3 },
        VmInsn { op: OP_FRONTIER_SWAP, a: 3, b: 0, imm: 0 },
        VmInsn { op: OP_FRONTIER_REDUCE_SUM, a: 4, b: 0, imm: 0 },
        VmInsn { op: OP_HALT, a: 0, b: 0, imm: 0 },
    ];

    let start = Instant::now();
    let mut checksum: u64 = 0;
    let mut input = [0_u32; FRONTIER_INPUT_LEN];
    let mut output = [0_u32; FRONTIER_CAPACITY];
    for _ in 0..iterations {
        for (idx, slot) in input.iter_mut().enumerate() {
            *slot = idx as u32;
        }
        let mut regs = [VmValue::Int(0); 16];
        let mut input_is_a = true;
        let mut frontier_input_len = FRONTIER_INPUT_LEN;
        let mut frontier_output_len = 0usize;
        let mut pc = 0usize;
        loop {
            let insn = PROGRAM[pc];
            pc += 1;
            match insn.op {
                OP_FRONTIER_FILTER_LT_IMM => {
                    let threshold = insn.imm as i64;
                    frontier_output_len = 0;
                    if input_is_a {
                        for &value in &input[..frontier_input_len] {
                            if (value as i64) < threshold {
                                output[frontier_output_len] = value;
                                frontier_output_len += 1;
                            }
                        }
                    } else {
                        for &value in &output[..frontier_input_len] {
                            if (value as i64) < threshold {
                                input[frontier_output_len] = value;
                                frontier_output_len += 1;
                            }
                        }
                    }
                    regs[insn.a as usize] = VmValue::Int(frontier_output_len as i64);
                }
                OP_FRONTIER_MAP_ADD_IMM => {
                    let delta = insn.imm as i64;
                    frontier_output_len = frontier_input_len;
                    if input_is_a {
                        for (idx, &value) in input[..frontier_input_len].iter().enumerate() {
                            output[idx] = ((value as i64) + delta) as u32;
                        }
                    } else {
                        for (idx, &value) in output[..frontier_input_len].iter().enumerate() {
                            input[idx] = ((value as i64) + delta) as u32;
                        }
                    }
                    regs[insn.a as usize] = VmValue::Int(frontier_output_len as i64);
                }
                OP_FRONTIER_REDUCE_SUM => {
                    let src = if input_is_a { &input[..frontier_input_len] } else { &output[..frontier_input_len] };
                    let mut sum = 0_u64;
                    for &value in src {
                        sum = sum.wrapping_add(u64::from(value));
                    }
                    regs[insn.a as usize] = VmValue::Int(sum as i64);
                }
                OP_FRONTIER_SWAP => {
                    input_is_a = !input_is_a;
                    frontier_input_len = frontier_output_len;
                    frontier_output_len = 0;
                    regs[insn.a as usize] = VmValue::Int(frontier_input_len as i64);
                }
                OP_HALT => break,
                _ => panic!("invalid frontier opcode"),
            }
        }
        checksum = checksum.wrapping_add(value_as_int(regs[4]) as u64);
        if input_is_a {
            black_box(&input[..frontier_input_len]);
        } else {
            black_box(&output[..frontier_input_len]);
        }
    }
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let instructions_per_iteration = PROGRAM.len();
    let mips = (iterations as f64 * instructions_per_iteration as f64 / secs) / 1_000_000.0;
    let ns_per_instruction =
        (secs * 1_000_000_000.0) / (iterations as f64 * instructions_per_iteration as f64);
    let ns_per_frontier_item =
        (secs * 1_000_000_000.0) / (iterations as f64 * FRONTIER_ITEMS_PER_ITERATION as f64);
    println!(
        "{{\"benchmark\":\"frontier_primitives\",\"iterations\":{},\"instructions_per_iteration\":{},\"frontier_items_per_iteration\":{},\"seconds\":{:.6},\"mips\":{:.3},\"ns_per_instruction\":{:.3},\"ns_per_frontier_item\":{:.3},\"checksum\":{}}}",
        iterations,
        instructions_per_iteration,
        FRONTIER_ITEMS_PER_ITERATION,
        secs,
        mips,
        ns_per_instruction,
        ns_per_frontier_item,
        checksum
    );
}

fn bfs_levels(graph: &CsrGraph<'_>, source: usize, levels: &mut [i32], queue: &mut [u32]) {
    levels.fill(-1);
    levels[source] = 0;
    let mut head = 0usize;
    let mut tail = 0usize;
    queue[tail] = source as u32;
    tail += 1;
    while head < tail {
        let u = queue[head] as usize;
        head += 1;
        let next_level = levels[u] + 1;
        let begin = graph.offsets[u] as usize;
        let end = graph.offsets[u + 1] as usize;
        for &v in &graph.neighbors[begin..end] {
            let v = v as usize;
            if levels[v] == -1 {
                levels[v] = next_level;
                queue[tail] = v as u32;
                tail += 1;
            }
        }
    }
}

#[inline(never)]
fn sum_frontier_neighbors(graph: &CsrGraph<'_>, frontier: &[usize]) -> u64 {
    let offsets = black_box(graph.offsets);
    let neighbors = black_box(graph.neighbors);
    let frontier = black_box(frontier);
    let mut checksum = 0_u64;

    for &node in frontier {
        let begin = offsets[node] as usize;
        let end = offsets[node + 1] as usize;
        for &neighbor in &neighbors[begin..end] {
            checksum = checksum.wrapping_add(u64::from(neighbor));
        }
    }

    checksum
}

#[inline(never)]
fn sum_weighted_node_weights(offsets: &[u32], weights: &[i64], node: usize) -> i64 {
    let begin = offsets[node] as usize;
    let end = offsets[node + 1] as usize;
    let mut total = 0_i64;
    for &weight in &weights[begin..end] {
        total = total.wrapping_add(weight);
    }
    total
}

#[inline(never)]
fn sum_weighted_node_attrs(offsets: &[u32], edge_attrs: &[i64], node: usize) -> i64 {
    let begin = offsets[node] as usize;
    let end = offsets[node + 1] as usize;
    let mut total = 0_i64;
    for &attr in &edge_attrs[begin..end] {
        total = total.wrapping_add(attr);
    }
    total
}

#[inline(never)]
fn sum_hypergraph_memberships(hg: &HyperGraph<'_>, hyperedge_nodes: &[u32]) -> u64 {
    let node_offsets = black_box(hg.node_offsets);
    let node_hyperedges = black_box(hg.node_hyperedges);
    let hyperedge_offsets = black_box(hg.hyperedge_offsets);
    let hyperedge_nodes = black_box(hyperedge_nodes);
    let mut checksum = 0_u64;

    for node in 0..(node_offsets.len() - 1) {
        let begin = node_offsets[node] as usize;
        let end = node_offsets[node + 1] as usize;
        for &hyperedge in &node_hyperedges[begin..end] {
            checksum = checksum.wrapping_add(u64::from(hyperedge));
        }
    }
    for hyperedge in 0..(hyperedge_offsets.len() - 1) {
        let begin = hyperedge_offsets[hyperedge] as usize;
        let end = hyperedge_offsets[hyperedge + 1] as usize;
        for &node in &hyperedge_nodes[begin..end] {
            checksum = checksum.wrapping_add(u64::from(node));
        }
    }

    checksum
}

fn bench_bfs(iterations: u64) {
    let graph = CsrGraph {
        offsets: &[0, 2, 4, 6, 9, 12, 14, 17, 19],
        neighbors: &[1, 2, 3, 4, 4, 5, 0, 6, 7, 1, 5, 7, 6, 7, 0, 2, 3, 1, 4],
    };
    let mut levels = [0_i32; 8];
    let mut queue = [0_u32; 8];

    let start = Instant::now();
    let mut checksum: u64 = 0;
    for _ in 0..iterations {
        bfs_levels(&graph, 0, &mut levels, &mut queue);
        checksum = checksum.wrapping_add(levels[7] as u64);
    }
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let edges_per_iter = graph.neighbors.len();
    let mteps = (iterations as f64 * edges_per_iter as f64 / secs) / 1_000_000.0;
    let ns_per_edge = (secs * 1_000_000_000.0) / (iterations as f64 * edges_per_iter as f64);
    println!(
        "{{\"benchmark\":\"bfs_levels\",\"iterations\":{},\"edges_per_iteration\":{},\"seconds\":{:.6},\"mteps\":{:.3},\"ns_per_edge\":{:.3},\"checksum\":{}}}",
        iterations, edges_per_iter, secs, mteps, ns_per_edge, checksum
    );
}

fn bench_neighbors(iterations: u64) {
    let offsets = vec![0, 2, 4, 6, 9, 12, 14, 17, 19];
    let neighbors = vec![1, 2, 3, 4, 4, 5, 0, 6, 7, 1, 5, 7, 6, 7, 0, 2, 3, 1, 4];
    let base_frontier: [usize; 3] = [0, 3, 6];
    let mut frontier: Vec<usize> = Vec::with_capacity(base_frontier.len() * 32);
    for _ in 0..32 {
        frontier.extend_from_slice(&base_frontier);
    }
    let graph = CsrGraph {
        offsets: &offsets,
        neighbors: &neighbors,
    };
    let frontier_neighbor_work: usize = frontier
        .iter()
        .map(|&node| (graph.offsets[node + 1] - graph.offsets[node]) as usize)
        .sum();
    let frontier_mode = recommend_frontier_mode(
        graph.offsets.len() - 1,
        graph.neighbors.len(),
        frontier.len(),
        frontier_neighbor_work,
    );

    let start = Instant::now();
    let mut checksum: u64 = 0;
    for _ in 0..iterations {
        checksum = checksum.wrapping_add(sum_frontier_neighbors(&graph, &frontier));
    }
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let neighbors_per_iteration = frontier_neighbor_work;
    let mteps = (iterations as f64 * neighbors_per_iteration as f64 / secs) / 1_000_000.0;
    let ns_per_neighbor =
        (secs * 1_000_000_000.0) / (iterations as f64 * neighbors_per_iteration as f64);
    println!(
        "{{\"benchmark\":\"neighbor_iteration\",\"iterations\":{},\"frontier_len\":{},\"neighbors_per_iteration\":{},\"frontier_neighbor_work\":{},\"recommended_frontier_mode\":\"{}\",\"seconds\":{:.6},\"mteps\":{:.3},\"ns_per_neighbor\":{:.3},\"checksum\":{}}}",
        iterations,
        frontier.len(),
        neighbors_per_iteration,
        neighbors_per_iteration,
        frontier_mode,
        secs,
        mteps,
        ns_per_neighbor,
        checksum
    );
}

fn bench_weighted_neighbor_sums(iterations: u64) {
    const INNER_REPEATS: usize = 4;
    const NODE_COUNT: usize = 8;
    const EDGES_PER_NODE: usize = 1024;
    const EDGE_COUNT: usize = NODE_COUNT * EDGES_PER_NODE;

    let offsets: Vec<u32> = (0..=NODE_COUNT)
        .map(|node| (node * EDGES_PER_NODE) as u32)
        .collect();
    let mut weights = Vec::with_capacity(EDGE_COUNT);
    let mut edge_attrs = Vec::with_capacity(EDGE_COUNT);
    for idx in 0..EDGE_COUNT {
        weights.push((idx + 1) as i64);
        edge_attrs.push((100 + idx) as i64);
    }
    let edge_data_items_per_iteration = 8192usize;
    let program = [
        VmInsn { op: OP_MOV_IMM, a: 0, b: 0, imm: 0 },
        VmInsn { op: OP_NEIGHBOR_WEIGHT_SUM, a: 0, b: 1, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 2, b: 0, imm: 2 },
        VmInsn { op: OP_NEIGHBOR_ATTR_SUM, a: 2, b: 3, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 4, b: 0, imm: 4 },
        VmInsn { op: OP_NEIGHBOR_WEIGHT_SUM, a: 4, b: 5, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 6, b: 0, imm: 6 },
        VmInsn { op: OP_NEIGHBOR_ATTR_SUM, a: 6, b: 7, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 8, b: 0, imm: 1 },
        VmInsn { op: OP_NEIGHBOR_WEIGHT_SUM, a: 8, b: 9, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 10, b: 0, imm: 3 },
        VmInsn { op: OP_NEIGHBOR_ATTR_SUM, a: 10, b: 11, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 12, b: 0, imm: 5 },
        VmInsn { op: OP_NEIGHBOR_WEIGHT_SUM, a: 12, b: 13, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 14, b: 0, imm: 7 },
        VmInsn { op: OP_NEIGHBOR_ATTR_SUM, a: 14, b: 15, imm: 0 },
        VmInsn { op: OP_HALT, a: 0, b: 0, imm: 0 },
    ];

    let start = Instant::now();
    let mut checksum: u64 = 0;
    for _ in 0..iterations {
        for _ in 0..INNER_REPEATS {
            let mut regs = [0_i64; 16];
            let mut pc = 0usize;
            loop {
                let insn = program[pc];
                pc += 1;
                match insn.op {
                    OP_MOV_IMM => regs[insn.a as usize] = i64::from(insn.imm),
                    OP_NEIGHBOR_WEIGHT_SUM => {
                        let node = regs[insn.a as usize] as usize;
                        regs[insn.b as usize] = sum_weighted_node_weights(&offsets, &weights, node);
                    }
                    OP_NEIGHBOR_ATTR_SUM => {
                        let node = regs[insn.a as usize] as usize;
                        regs[insn.b as usize] = sum_weighted_node_attrs(&offsets, &edge_attrs, node);
                    }
                    OP_HALT => break,
                    _ => panic!("invalid opcode"),
                }
            }
            checksum = checksum.wrapping_add(
                (regs[1] + regs[3] + regs[5] + regs[7] + regs[9] + regs[11] + regs[13] + regs[15])
                    as u64,
            );
        }
    }
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let instruction_count = program.len() * INNER_REPEATS;
    let edge_data_items_per_iteration = edge_data_items_per_iteration * INNER_REPEATS;
    let mteps = (iterations as f64 * edge_data_items_per_iteration as f64 / secs) / 1_000_000.0;
    let ns_per_instruction =
        (secs * 1_000_000_000.0) / (iterations as f64 * instruction_count as f64);
    let ns_per_edge_data =
        (secs * 1_000_000_000.0) / (iterations as f64 * edge_data_items_per_iteration as f64);
    println!(
        "{{\"benchmark\":\"weighted_neighbor_sums\",\"iterations\":{},\"instructions_per_iteration\":{},\"edge_data_items_per_iteration\":{},\"seconds\":{:.6},\"mteps\":{:.3},\"ns_per_instruction\":{:.3},\"ns_per_edge_data\":{:.3},\"checksum\":{}}}",
        iterations,
        instruction_count,
        edge_data_items_per_iteration,
        secs,
        mteps,
        ns_per_instruction,
        ns_per_edge_data,
        checksum
    );
}

fn bench_hypergraph(iterations: u64) {
    const INNER_REPEATS: usize = 8;
    let hg = HyperGraph {
        node_offsets: &[0, 2, 5, 8, 10, 12],
        node_hyperedges: &[0, 1, 0, 2, 3, 1, 2, 3, 2, 3, 0, 1],
        hyperedge_offsets: &[0, 3, 6, 9, 12],
    };
    let mut checksum: u64 = 0;

    let start = Instant::now();
    for _ in 0..iterations {
        for _ in 0..INNER_REPEATS {
            let values = black_box(hg.node_hyperedges);
            let mut i = 0usize;
            while i + 4 <= values.len() {
                checksum = checksum
                    .wrapping_add(u64::from(values[i]))
                    .wrapping_add(u64::from(values[i + 1]))
                    .wrapping_add(u64::from(values[i + 2]))
                    .wrapping_add(u64::from(values[i + 3]));
                i += 4;
            }
            while i < values.len() {
                checksum = checksum.wrapping_add(u64::from(values[i]));
                i += 1;
            }
        }
    }
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let incidence = hg.node_hyperedges.len() * INNER_REPEATS;
    let mips = (iterations as f64 * incidence as f64 / secs) / 1_000_000.0;
    let ns_per_incidence = (secs * 1_000_000_000.0) / (iterations as f64 * incidence as f64);
    println!(
        "{{\"benchmark\":\"hypergraph_incidence\",\"iterations\":{},\"incidence_per_iteration\":{},\"seconds\":{:.6},\"mips\":{:.3},\"ns_per_incidence\":{:.3},\"checksum\":{}}}",
        iterations, incidence, secs, mips, ns_per_incidence, checksum
    );
}

fn bench_hypergraph_traversal(iterations: u64) {
    let node_offsets = vec![0, 2, 5, 8, 10, 12];
    let node_hyperedges = vec![0, 1, 0, 2, 3, 1, 2, 3, 2, 3, 0, 1];
    let hyperedge_offsets = vec![0, 3, 6, 9, 12];
    let hyperedge_nodes = vec![0, 1, 4, 0, 2, 4, 1, 2, 3, 1, 2, 3];
    let hg = HyperGraph {
        node_offsets: &node_offsets,
        node_hyperedges: &node_hyperedges,
        hyperedge_offsets: &hyperedge_offsets,
    };
    let memberships_per_iteration = hg.node_hyperedges.len() + hyperedge_nodes.len();
    let mut checksum: u64 = 0;

    let start = Instant::now();
    for _ in 0..iterations {
        checksum = checksum.wrapping_add(sum_hypergraph_memberships(&hg, &hyperedge_nodes));
    }
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let mteps = (iterations as f64 * memberships_per_iteration as f64 / secs) / 1_000_000.0;
    let ns_per_membership =
        (secs * 1_000_000_000.0) / (iterations as f64 * memberships_per_iteration as f64);
    println!(
        "{{\"benchmark\":\"hypergraph_traversal\",\"iterations\":{},\"memberships_per_iteration\":{},\"seconds\":{:.6},\"mteps\":{:.3},\"ns_per_membership\":{:.3},\"checksum\":{}}}",
        iterations, memberships_per_iteration, secs, mteps, ns_per_membership, checksum
    );
}

fn bench_hypergraph_incident_sum(iterations: u64) {
    let hg = HyperGraph {
        node_offsets: &[0, 2, 5, 8, 10, 12],
        node_hyperedges: &[0, 1, 0, 2, 3, 1, 2, 3, 2, 3, 0, 1],
        hyperedge_offsets: &[0, 3, 6, 9, 12],
    };
    let mut node_workload: Vec<usize> = Vec::with_capacity((hg.node_offsets.len() - 1) * 16);
    for _ in 0..16 {
        for node in 0..(hg.node_offsets.len() - 1) {
            node_workload.push(node);
        }
    }
    let mut checksum: u64 = 0;

    let start = Instant::now();
    for _ in 0..iterations {
        for &node in &node_workload {
            checksum = checksum.wrapping_add(hypergraph_incident_sum(&hg, node));
        }
    }
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let calls_per_iter = node_workload.len();
    let mips = (iterations as f64 * calls_per_iter as f64 / secs) / 1_000_000.0;
    let ns_per_call = (secs * 1_000_000_000.0) / (iterations as f64 * calls_per_iter as f64);
    println!(
        "{{\"benchmark\":\"hypergraph_incident_sum\",\"iterations\":{},\"calls_per_iteration\":{},\"seconds\":{:.6},\"mips\":{:.3},\"ns_per_call\":{:.3},\"checksum\":{}}}",
        iterations, calls_per_iter, secs, mips, ns_per_call, checksum
    );
}

fn bench_hypergraph_hyperedge_node_sum(iterations: u64) {
    let hg = HyperGraph {
        node_offsets: &[0, 2, 5, 8, 10, 12],
        node_hyperedges: &[0, 1, 0, 2, 3, 1, 2, 3, 2, 3, 0, 1],
        hyperedge_offsets: &[0, 3, 6, 9, 12],
    };
    let hyperedge_nodes: [u32; 12] = [0, 1, 4, 0, 2, 4, 1, 2, 3, 1, 2, 3];
    let mut hyperedge_workload: Vec<usize> = Vec::with_capacity((hg.hyperedge_offsets.len() - 1) * 16);
    for _ in 0..16 {
        for h in 0..(hg.hyperedge_offsets.len() - 1) {
            hyperedge_workload.push(h);
        }
    }
    let mut checksum: u64 = 0;

    let start = Instant::now();
    for _ in 0..iterations {
        for &h in &hyperedge_workload {
            checksum = checksum.wrapping_add(hypergraph_hyperedge_node_sum(&hg, &hyperedge_nodes, h));
        }
    }
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let calls_per_iter = hyperedge_workload.len();
    let mips = (iterations as f64 * calls_per_iter as f64 / secs) / 1_000_000.0;
    let ns_per_call = (secs * 1_000_000_000.0) / (iterations as f64 * calls_per_iter as f64);
    println!(
        "{{\"benchmark\":\"hypergraph_hyperedge_node_sum\",\"iterations\":{},\"calls_per_iteration\":{},\"seconds\":{:.6},\"mips\":{:.3},\"ns_per_call\":{:.3},\"checksum\":{}}}",
        iterations, calls_per_iter, secs, mips, ns_per_call, checksum
    );
}

fn run_vm_graph_ops(iterations: u64) {
    let csr = CsrGraph {
        offsets: &[0, 2, 3, 5, 6],
        neighbors: &[1, 2, 3, 0, 3, 1],
    };
    let hg = HyperGraph {
        node_offsets: &[0, 1, 3, 5, 7],
        node_hyperedges: &[0, 0, 1, 0, 2, 1, 2],
        hyperedge_offsets: &[0, 3, 5, 7],
    };
    let program = [
        VmInsn { op: OP_MOV_IMM, a: 0, b: 0, imm: 0 },
        VmInsn { op: OP_BFS_LEVEL_COUNT, a: 0, b: 1, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 2, b: 0, imm: 0 },
        VmInsn { op: OP_BFS_ORDER, a: 2, b: 3, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 4, b: 0, imm: 1 },
        VmInsn { op: OP_INCIDENT_COUNT, a: 4, b: 5, imm: 0 },
        VmInsn { op: OP_INCIDENT_SUM, a: 4, b: 6, imm: 0 },
        VmInsn { op: OP_ADD, a: 7, b: 1, imm: 0 },
        VmInsn { op: OP_ADD, a: 7, b: 3, imm: 0 },
        VmInsn { op: OP_ADD, a: 7, b: 5, imm: 0 },
        VmInsn { op: OP_ADD, a: 7, b: 6, imm: 0 },
        VmInsn { op: OP_HALT, a: 0, b: 0, imm: 0 },
    ];
    let mut levels = [0_i32; 4];
    let mut queue = [0_u32; 4];
    let mut frontier = [0_u32; 4];

    let start = Instant::now();
    let mut checksum: u64 = 0;
    for _ in 0..iterations {
        let mut regs = [VmValue::Int(0); 16];
        let mut frontier_output_len = 0usize;
        let mut pc = 0usize;
        loop {
            let insn = program[pc];
            pc += 1;
            match insn.op {
                OP_MOV_IMM => regs[insn.a as usize] = VmValue::Int(i64::from(insn.imm)),
                OP_ADD => {
                    let lhs = value_as_int(regs[insn.a as usize]);
                    let rhs = value_as_int(regs[insn.b as usize]);
                    regs[insn.a as usize] = VmValue::Int(lhs.wrapping_add(rhs));
                }
                OP_BFS_LEVELS => {
                    let source = value_as_int(regs[insn.a as usize]) as usize;
                    bfs_levels(&csr, source, &mut levels, &mut queue);
                    regs[insn.b as usize] =
                        VmValue::Int(levels.iter().filter(|x| **x >= 0).count() as i64);
                }
                OP_BFS_LEVEL_COUNT => {
                    let source = value_as_int(regs[insn.a as usize]) as usize;
                    bfs_levels(&csr, source, &mut levels, &mut queue);
                    regs[insn.b as usize] = VmValue::Int(
                        (levels.iter().copied().max().unwrap_or(-1) + 1).max(0) as i64,
                    );
                }
                OP_BFS_ORDER => {
                    let source = value_as_int(regs[insn.a as usize]) as usize;
                    bfs_levels(&csr, source, &mut levels, &mut queue);
                    frontier_output_len = levels.iter().filter(|x| **x >= 0).count();
                    for idx in 0..frontier_output_len {
                        frontier[idx] = queue[idx];
                    }
                    black_box(&frontier[..frontier_output_len]);
                    regs[insn.b as usize] = VmValue::Int(frontier_output_len as i64);
                }
                OP_INCIDENT_COUNT => {
                    let node = value_as_int(regs[insn.a as usize]) as usize;
                    let c = hg.node_offsets[node + 1] - hg.node_offsets[node];
                    regs[insn.b as usize] = VmValue::Int(i64::from(c));
                }
                OP_INCIDENT_SUM => {
                    let node = value_as_int(regs[insn.a as usize]) as usize;
                    regs[insn.b as usize] = VmValue::Int(hypergraph_incident_sum(&hg, node) as i64);
                }
                OP_HYPEREDGE_SIZE => {
                    let h = value_as_int(regs[insn.a as usize]) as usize;
                    let c = hg.hyperedge_offsets[h + 1] - hg.hyperedge_offsets[h];
                    regs[insn.b as usize] = VmValue::Int(i64::from(c));
                }
                OP_HALT => break,
                _ => panic!("invalid opcode"),
            }
        }
        checksum = checksum.wrapping_add(value_as_int(regs[7]) as u64);
    }
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let mips = (iterations as f64 * program.len() as f64 / secs) / 1_000_000.0;
    let ns_per_instruction = (secs * 1_000_000_000.0) / (iterations as f64 * program.len() as f64);
    println!(
        "{{\"benchmark\":\"vm_graph_ops\",\"iterations\":{},\"instructions_per_iteration\":{},\"seconds\":{:.6},\"mips\":{:.3},\"ns_per_instruction\":{:.3},\"checksum\":{}}}",
        iterations,
        program.len(),
        secs,
        mips,
        ns_per_instruction,
        checksum
    );
}

fn parse_iterations(args: &[String], default_value: u64) -> u64 {
    if args.len() >= 3 {
        args[2].parse::<u64>().unwrap_or(default_value)
    } else {
        default_value
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("usage: graphion_rust <frontier_primitives|vm_dispatch|scalar_values_print|bfs_levels|neighbor_iteration|weighted_neighbor_sums|hypergraph_incidence|hypergraph_traversal|hypergraph_incident_sum|hypergraph_hyperedge_node_sum|vm_graph_ops> [iterations]");
        std::process::exit(2);
    }

    match args[1].as_str() {
        "frontier_primitives" => frontier_primitives(parse_iterations(&args, 10_000_000)),
        "vm_dispatch" => vm_dispatch(parse_iterations(&args, 5_000_000)),
        "scalar_values_print" => scalar_values_print(parse_iterations(&args, 100_000)),
        "bfs_levels" => bench_bfs(parse_iterations(&args, 5_000_000)),
        "neighbor_iteration" => bench_neighbors(parse_iterations(&args, 10_000_000)),
        "weighted_neighbor_sums" => bench_weighted_neighbor_sums(parse_iterations(&args, 300_000)),
        "hypergraph_incidence" => bench_hypergraph(parse_iterations(&args, 10_000_000)),
        "hypergraph_traversal" => bench_hypergraph_traversal(parse_iterations(&args, 10_000_000)),
        "hypergraph_incident_sum" => bench_hypergraph_incident_sum(parse_iterations(&args, 10_000_000)),
        "hypergraph_hyperedge_node_sum" => {
            bench_hypergraph_hyperedge_node_sum(parse_iterations(&args, 10_000_000))
        }
        "vm_graph_ops" => run_vm_graph_ops(parse_iterations(&args, 10_000_000)),
        _ => {
            eprintln!("unknown benchmark '{}'", args[1]);
            std::process::exit(2);
        }
    }
}
