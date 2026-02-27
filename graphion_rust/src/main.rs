use std::time::Instant;
use std::arch::x86_64::*;

// ---------------- STRUCTS ----------------
#[derive(Clone, Copy)]
struct Instruction {
    opcode: u8,
    operand: u32,
}

#[derive(Clone, Copy)]
struct Edge {
    target_id: u32,
    weight: u32,
}

struct Node {
    id: u32,
    edges: Vec<Edge>,
}

struct HyperGraph {
    nodes: Vec<Node>,
}

// ---------------- HOT PATH SIMD ----------------
fn traverse_neighbors_simd(node: &Node, repetitions: usize) -> u64 {
    let start = Instant::now();
    let mut acc_sum: u64 = 0;

    let edge_count = node.edges.len();
    let mut i = 0;

    unsafe {
        while i + 8 <= edge_count {
            let mut target_arr = [0u32; 8];
            let mut weight_arr = [0u32; 8];
            for j in 0..8 {
                target_arr[j] = node.edges[i + j].target_id;
                weight_arr[j] = node.edges[i + j].weight;
            }

            let targets = _mm256_loadu_si256(target_arr.as_ptr() as *const __m256i);
            let weights = _mm256_loadu_si256(weight_arr.as_ptr() as *const __m256i);

            // Exemple simple de calcul SIMD : somme des poids
            let sum = _mm256_sad_epu8(weights, _mm256_setzero_si256());
            acc_sum += _mm256_extract_epi64(sum, 0) as u64;
            acc_sum += _mm256_extract_epi64(sum, 1) as u64;
            acc_sum += _mm256_extract_epi64(sum, 2) as u64;
            acc_sum += _mm256_extract_epi64(sum, 3) as u64;

            i += 8;
        }

        // Traiter les éventuels restes
        while i < edge_count {
            acc_sum += node.edges[i].weight as u64;
            i += 1;
        }
    }

    let elapsed = start.elapsed();
    println!(
        "Hot path SIMD traversal for node {} with {} edges repeated {} times took {:.6} seconds, accumulated sum={}",
        node.id,
        node.edges.len(),
        repetitions,
        elapsed.as_secs_f64(),
        acc_sum
    );

    acc_sum
}

// ---------------- INTERPRETEUR ----------------
fn run_program(program: &[Instruction]) {
    println!("Running program with {} instructions...", program.len());
    for instr in program {
        println!("Opcode: {}, Operand: {}", instr.opcode, instr.operand);
    }
}

// ---------------- MAIN ----------------
fn main() {
    println!("Graphion SIMD HyperGraph ready!");

    // Exemple de programme
    let program = vec![
        Instruction { opcode: 1, operand: 100 },
        Instruction { opcode: 2, operand: 200 },
    ];

    run_program(&program);

    // Exemple de graph
    let mut node = Node {
        id: 0,
        edges: Vec::new(),
    };
    for i in 1..=100_000 {
        node.edges.push(Edge {
            target_id: i,
            weight: i % 100 + 1,
        });
    }

    println!(
        "Traversing neighbors via hot path ({} edges, {} repetitions)...",
        node.edges.len(),
        1000
    );

    traverse_neighbors_simd(&node, 1000);
}