use std::collections::VecDeque;
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
const OP_HALT: u8 = 1;
const OP_BFS_LEVELS: u8 = 16;
const OP_INCIDENT_COUNT: u8 = 17;
const OP_HYPEREDGE_SIZE: u8 = 18;
const OP_NEIGHBOR_WEIGHT_SUM: u8 = 21;
const OP_NEIGHBOR_ATTR_SUM: u8 = 22;

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
        VmInsn { op: OP_MOV_IMM, a: 0, b: 0, imm: 1 },
        VmInsn { op: OP_MOV_IMM, a: 1, b: 0, imm: 2 },
        VmInsn { op: OP_ADD, a: 0, b: 1, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 2, b: 0, imm: 10 },
        VmInsn { op: OP_ADD, a: 0, b: 2, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 3, b: 0, imm: 4 },
        VmInsn { op: OP_ADD, a: 0, b: 3, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 4, b: 0, imm: 5 },
        VmInsn { op: OP_ADD, a: 0, b: 4, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 5, b: 0, imm: 20 },
        VmInsn { op: OP_ADD, a: 0, b: 5, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 6, b: 0, imm: 1 },
        VmInsn { op: OP_ADD, a: 0, b: 6, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 7, b: 0, imm: 8 },
        VmInsn { op: OP_ADD, a: 0, b: 7, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 8, b: 0, imm: 100 },
        VmInsn { op: OP_ADD, a: 0, b: 8, imm: 0 },
        VmInsn { op: OP_HALT, a: 0, b: 0, imm: 0 },
    ];

    let start = Instant::now();
    let mut checksum: u64 = 0;
    for _ in 0..iterations {
        let mut regs = [0_i64; 16];
        let mut pc = 0usize;
        loop {
            let insn = program[pc];
            pc += 1;
            match insn.op {
                OP_MOV_IMM => regs[insn.a as usize] = i64::from(insn.imm),
                OP_ADD => regs[insn.a as usize] += regs[insn.b as usize],
                OP_HALT => break,
                _ => panic!("invalid opcode"),
            }
        }
        checksum = checksum.wrapping_add(regs[0] as u64);
    }
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let mips = (iterations as f64 * program.len() as f64 / secs) / 1_000_000.0;
    let ns_per_instruction = (secs * 1_000_000_000.0) / (iterations as f64 * program.len() as f64);
    println!(
        "{{\"benchmark\":\"vm_dispatch\",\"iterations\":{},\"instructions_per_iteration\":{},\"seconds\":{:.6},\"mips\":{:.3},\"ns_per_instruction\":{:.3},\"checksum\":{}}}",
        iterations,
        program.len(),
        secs,
        mips,
        ns_per_instruction,
        checksum
    );
}

fn frontier_primitives(iterations: u64) {
    const FRONTIER_INPUT_LEN: usize = 64;
    const FRONTIER_ITEMS_PER_ITERATION: usize = 128;

    let start = Instant::now();
    let mut checksum: u64 = 0;
    for _ in 0..iterations {
        let input: Vec<u32> = (0..FRONTIER_INPUT_LEN as u32).collect();
        let filtered: Vec<u32> = input.iter().copied().filter(|&value| (value as i64) < 32).collect();
        let mapped: Vec<u32> = filtered.iter().map(|&value| value + 3).collect();
        let reduced: u64 = mapped.iter().map(|&value| u64::from(value)).sum();
        checksum = checksum.wrapping_add(reduced);
        black_box(&mapped);
    }
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let instructions_per_iteration = 6usize;
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

fn bfs_levels(graph: &CsrGraph<'_>, source: usize, levels: &mut [i32]) {
    levels.fill(-1);
    levels[source] = 0;
    let mut q = VecDeque::new();
    q.push_back(source as u32);
    while let Some(u) = q.pop_front() {
        let u = u as usize;
        let next_level = levels[u] + 1;
        let begin = graph.offsets[u] as usize;
        let end = graph.offsets[u + 1] as usize;
        for &v in &graph.neighbors[begin..end] {
            let v = v as usize;
            if levels[v] == -1 {
                levels[v] = next_level;
                q.push_back(v as u32);
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

    let start = Instant::now();
    let mut checksum: u64 = 0;
    for _ in 0..iterations {
        bfs_levels(&graph, 0, &mut levels);
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
    let frontier: Vec<usize> = vec![0, 3, 6];
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
    let mteps = (iterations as f64 * frontier_neighbor_work as f64 / secs) / 1_000_000.0;
    let ns_per_neighbor =
        (secs * 1_000_000_000.0) / (iterations as f64 * frontier_neighbor_work as f64);
    println!(
        "{{\"benchmark\":\"neighbor_iteration\",\"iterations\":{},\"frontier_len\":{},\"neighbors_per_iteration\":{},\"frontier_neighbor_work\":{},\"recommended_frontier_mode\":\"{}\",\"seconds\":{:.6},\"mteps\":{:.3},\"ns_per_neighbor\":{:.3},\"checksum\":{}}}",
        iterations,
        frontier.len(),
        frontier_neighbor_work,
        frontier_neighbor_work,
        frontier_mode,
        secs,
        mteps,
        ns_per_neighbor,
        checksum
    );
}

fn bench_weighted_neighbor_sums(iterations: u64) {
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
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let instruction_count = program.len();
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
    let hg = HyperGraph {
        node_offsets: &[0, 2, 5, 8, 10, 12],
        node_hyperedges: &[0, 1, 0, 2, 3, 1, 2, 3, 2, 3, 0, 1],
        hyperedge_offsets: &[0, 3, 6, 9, 12],
    };
    let mut checksum: u64 = 0;

    let start = Instant::now();
    for _ in 0..iterations {
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
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let incidence = hg.node_hyperedges.len();
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
    let mut checksum: u64 = 0;

    let start = Instant::now();
    for _ in 0..iterations {
        for node in 0..(hg.node_offsets.len() - 1) {
            checksum = checksum.wrapping_add(hypergraph_incident_sum(&hg, node));
        }
    }
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let calls_per_iter = hg.node_offsets.len() - 1;
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
    let mut checksum: u64 = 0;

    let start = Instant::now();
    for _ in 0..iterations {
        for h in 0..(hg.hyperedge_offsets.len() - 1) {
            checksum = checksum.wrapping_add(hypergraph_hyperedge_node_sum(&hg, &hyperedge_nodes, h));
        }
    }
    black_box(checksum);
    let secs = start.elapsed().as_secs_f64().max(1e-9);
    let calls_per_iter = hg.hyperedge_offsets.len() - 1;
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
        VmInsn { op: OP_BFS_LEVELS, a: 0, b: 1, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 2, b: 0, imm: 1 },
        VmInsn { op: OP_INCIDENT_COUNT, a: 2, b: 3, imm: 0 },
        VmInsn { op: OP_MOV_IMM, a: 4, b: 0, imm: 0 },
        VmInsn { op: OP_HYPEREDGE_SIZE, a: 4, b: 5, imm: 0 },
        VmInsn { op: OP_ADD, a: 6, b: 1, imm: 0 },
        VmInsn { op: OP_ADD, a: 6, b: 3, imm: 0 },
        VmInsn { op: OP_ADD, a: 6, b: 5, imm: 0 },
        VmInsn { op: OP_HALT, a: 0, b: 0, imm: 0 },
    ];
    let mut levels = [0_i32; 4];

    let start = Instant::now();
    let mut checksum: u64 = 0;
    for _ in 0..iterations {
        let mut regs = [0_i64; 16];
        let mut pc = 0usize;
        loop {
            let insn = program[pc];
            pc += 1;
            match insn.op {
                OP_MOV_IMM => regs[insn.a as usize] = i64::from(insn.imm),
                OP_ADD => regs[insn.a as usize] += regs[insn.b as usize],
                OP_BFS_LEVELS => {
                    let source = regs[insn.a as usize] as usize;
                    bfs_levels(&csr, source, &mut levels);
                    regs[insn.b as usize] = levels.iter().filter(|x| **x >= 0).count() as i64;
                }
                OP_INCIDENT_COUNT => {
                    let node = regs[insn.a as usize] as usize;
                    let c = hg.node_offsets[node + 1] - hg.node_offsets[node];
                    regs[insn.b as usize] = i64::from(c);
                }
                OP_HYPEREDGE_SIZE => {
                    let h = regs[insn.a as usize] as usize;
                    let c = hg.hyperedge_offsets[h + 1] - hg.hyperedge_offsets[h];
                    regs[insn.b as usize] = i64::from(c);
                }
                OP_HALT => break,
                _ => panic!("invalid opcode"),
            }
        }
        checksum = checksum.wrapping_add(regs[6] as u64);
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
        eprintln!("usage: graphion_rust <frontier_primitives|vm_dispatch|bfs_levels|neighbor_iteration|weighted_neighbor_sums|hypergraph_incidence|hypergraph_traversal|hypergraph_incident_sum|hypergraph_hyperedge_node_sum|vm_graph_ops> [iterations]");
        std::process::exit(2);
    }

    match args[1].as_str() {
        "frontier_primitives" => frontier_primitives(parse_iterations(&args, 300_000)),
        "vm_dispatch" => vm_dispatch(parse_iterations(&args, 500_000)),
        "bfs_levels" => bench_bfs(parse_iterations(&args, 200_000)),
        "neighbor_iteration" => bench_neighbors(parse_iterations(&args, 300_000)),
        "weighted_neighbor_sums" => bench_weighted_neighbor_sums(parse_iterations(&args, 300_000)),
        "hypergraph_incidence" => bench_hypergraph(parse_iterations(&args, 500_000)),
        "hypergraph_traversal" => bench_hypergraph_traversal(parse_iterations(&args, 300_000)),
        "hypergraph_incident_sum" => bench_hypergraph_incident_sum(parse_iterations(&args, 500_000)),
        "hypergraph_hyperedge_node_sum" => {
            bench_hypergraph_hyperedge_node_sum(parse_iterations(&args, 500_000))
        }
        "vm_graph_ops" => run_vm_graph_ops(parse_iterations(&args, 300_000)),
        _ => {
            eprintln!("unknown benchmark '{}'", args[1]);
            std::process::exit(2);
        }
    }
}
