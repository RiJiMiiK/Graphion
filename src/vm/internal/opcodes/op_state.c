/* SPDX-License-Identifier: MIT */

#include "vm/internal/opcodes/op_state.h"

#include <string.h>

#include "vm/internal/opcodes/op_scalar.h"
#include "vm/internal/core/value.h"

int op_nop(graphion_vm *vm, const graphion_insn *in) {
  (void)vm;
  (void)in;
  return 0;
}

int op_halt(graphion_vm *vm, const graphion_insn *in) {
  (void)in;
  vm->halted = true;
  return 0;
}

int op_mov_imm(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_MOV_IMM_REG;
  }
  vm_free_owned_reg_string(vm, in->a);
  vm_reg_set_int(vm, in->a, (int64_t)in->imm);
  return 0;
}

int op_jump(graphion_vm *vm, const graphion_insn *in) {
  (void)in;
  if (in->imm < 0 || (size_t)in->imm >= vm->program_len) {
    return GVM_ERR_INVALID_ARG;
  }
  vm->pc = (size_t)in->imm;
  return GVM_OK;
}

int op_jump_if_true(graphion_vm *vm, const graphion_insn *in) {
  int bool_value;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_boolean(&vm->regs[in->a], &bool_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (bool_value != 0) {
    if (in->imm < 0 || (size_t)in->imm >= vm->program_len) {
      return GVM_ERR_INVALID_ARG;
    }
    vm->pc = (size_t)in->imm;
  }
  return GVM_OK;
}

int op_jump_if_false(graphion_vm *vm, const graphion_insn *in) {
  int bool_value;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_boolean(&vm->regs[in->a], &bool_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (bool_value == 0) {
    if (in->imm < 0 || (size_t)in->imm >= vm->program_len) {
      return GVM_ERR_INVALID_ARG;
    }
    vm->pc = (size_t)in->imm;
  }
  return GVM_OK;
}

int op_mov(graphion_vm *vm, const graphion_insn *in) {
  graphion_vm_value cloned;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->b].kind == GVM_VALUE_STRING && vm->regs[in->b].as.string_value != NULL) {
    return vm_reg_set_string_copy(vm, in->a, vm->regs[in->b].as.string_value);
  }
  if (vm->regs[in->b].kind == GVM_VALUE_LIST || vm->regs[in->b].kind == GVM_VALUE_DICT ||
      vm->regs[in->b].kind == GVM_VALUE_TUPLE || vm->regs[in->b].kind == GVM_VALUE_SET ||
      vm->regs[in->b].kind == GVM_VALUE_GRAPH_REF || vm->regs[in->b].kind == GVM_VALUE_HYPERGRAPH_REF ||
      vm->regs[in->b].kind == GVM_VALUE_STRUCT_TYPE || vm->regs[in->b].kind == GVM_VALUE_STRUCT) {
    int rc = vm_value_clone(&cloned, &vm->regs[in->b]);
    if (rc != GVM_OK) {
      return rc;
    }
    vm_free_owned_reg_string(vm, in->a);
    vm->regs[in->a] = cloned;
    return GVM_OK;
  }
  vm_free_owned_reg_string(vm, in->a);
  vm_value_copy(&vm->regs[in->a], &vm->regs[in->b]);
  return 0;
}

int op_load_const(graphion_vm *vm, const graphion_insn *in) {
  graphion_vm_value cloned;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->const_pool == NULL) {
    return GVM_ERR_CONST_UNBOUND;
  }
  if (in->imm < 0 || (size_t)in->imm >= vm->const_count) {
    return GVM_ERR_INVALID_CONST_INDEX;
  }
  if (vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_STRING &&
      vm->const_pool[(size_t)in->imm].as.string_value != NULL) {
    return vm_reg_set_string_copy(vm, in->a, vm->const_pool[(size_t)in->imm].as.string_value);
  }
  if (vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_LIST ||
      vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_DICT ||
      vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_TUPLE ||
      vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_SET ||
      vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_GRAPH_REF ||
      vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_HYPERGRAPH_REF ||
      vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_STRUCT_TYPE ||
      vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_STRUCT) {
    int rc = vm_value_clone(&cloned, &vm->const_pool[(size_t)in->imm]);
    if (rc != GVM_OK) {
      return rc;
    }
    vm_free_owned_reg_string(vm, in->a);
    vm->regs[in->a] = cloned;
    return GVM_OK;
  }
  vm_free_owned_reg_string(vm, in->a);
  vm_value_copy(&vm->regs[in->a], &vm->const_pool[(size_t)in->imm]);
  return 0;
}

int op_load_global(graphion_vm *vm, const graphion_insn *in) {
  graphion_vm_value cloned;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->globals == NULL) {
    return GVM_ERR_GLOBALS_UNBOUND;
  }
  if (in->imm < 0 || (size_t)in->imm >= vm->global_count) {
    return GVM_ERR_INVALID_GLOBAL_INDEX;
  }
  if (vm->globals[(size_t)in->imm].kind == GVM_VALUE_STRING &&
      vm->globals[(size_t)in->imm].as.string_value != NULL) {
    return vm_reg_set_string_copy(vm, in->a, vm->globals[(size_t)in->imm].as.string_value);
  }
  if (vm->globals[(size_t)in->imm].kind == GVM_VALUE_LIST ||
      vm->globals[(size_t)in->imm].kind == GVM_VALUE_DICT ||
      vm->globals[(size_t)in->imm].kind == GVM_VALUE_TUPLE ||
      vm->globals[(size_t)in->imm].kind == GVM_VALUE_SET ||
      vm->globals[(size_t)in->imm].kind == GVM_VALUE_GRAPH_REF ||
      vm->globals[(size_t)in->imm].kind == GVM_VALUE_HYPERGRAPH_REF ||
      vm->globals[(size_t)in->imm].kind == GVM_VALUE_STRUCT_TYPE ||
      vm->globals[(size_t)in->imm].kind == GVM_VALUE_STRUCT) {
    int rc = vm_value_clone(&cloned, &vm->globals[(size_t)in->imm]);
    if (rc != GVM_OK) {
      return rc;
    }
    vm_free_owned_reg_string(vm, in->a);
    vm->regs[in->a] = cloned;
    return GVM_OK;
  }
  vm_free_owned_reg_string(vm, in->a);
  vm_value_copy(&vm->regs[in->a], &vm->globals[(size_t)in->imm]);
  return 0;
}

int op_store_global(graphion_vm *vm, const graphion_insn *in) {
  graphion_vm_value cloned;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->globals == NULL) {
    return GVM_ERR_GLOBALS_UNBOUND;
  }
  if (in->imm < 0 || (size_t)in->imm >= vm->global_count) {
    return GVM_ERR_INVALID_GLOBAL_INDEX;
  }
  if (vm->regs[in->a].kind == GVM_VALUE_STRING && vm->regs[in->a].as.string_value != NULL) {
    return vm_global_set_string_copy(vm, (size_t)in->imm, vm->regs[in->a].as.string_value);
  }
  if (vm->regs[in->a].kind == GVM_VALUE_LIST || vm->regs[in->a].kind == GVM_VALUE_DICT ||
      vm->regs[in->a].kind == GVM_VALUE_TUPLE || vm->regs[in->a].kind == GVM_VALUE_SET ||
      vm->regs[in->a].kind == GVM_VALUE_GRAPH_REF || vm->regs[in->a].kind == GVM_VALUE_HYPERGRAPH_REF ||
      vm->regs[in->a].kind == GVM_VALUE_STRUCT_TYPE || vm->regs[in->a].kind == GVM_VALUE_STRUCT) {
    int rc = vm_value_clone(&cloned, &vm->regs[in->a]);
    if (rc != GVM_OK) {
      return rc;
    }
    vm_release_global_value(vm, (size_t)in->imm);
    vm->globals[(size_t)in->imm] = cloned;
    return GVM_OK;
  }
  vm_release_global_value(vm, (size_t)in->imm);
  vm_value_copy(&vm->globals[(size_t)in->imm], &vm->regs[in->a]);
  return 0;
}

int op_store_const_global(graphion_vm *vm, const graphion_insn *in) {
  graphion_vm_value cloned;
  if (vm->const_pool == NULL) {
    return GVM_ERR_CONST_UNBOUND;
  }
  if (vm->globals == NULL) {
    return GVM_ERR_GLOBALS_UNBOUND;
  }
  if (in->imm < 0 || (size_t)in->imm >= vm->const_count) {
    return GVM_ERR_INVALID_CONST_INDEX;
  }
  if ((size_t)in->b >= vm->global_count) {
    return GVM_ERR_INVALID_GLOBAL_INDEX;
  }
  if (vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_STRING &&
      vm->const_pool[(size_t)in->imm].as.string_value != NULL) {
    return vm_global_set_string_copy(vm, (size_t)in->b, vm->const_pool[(size_t)in->imm].as.string_value);
  }
  if (vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_LIST ||
      vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_DICT ||
      vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_TUPLE ||
      vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_SET ||
      vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_GRAPH_REF ||
      vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_HYPERGRAPH_REF ||
      vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_STRUCT_TYPE ||
      vm->const_pool[(size_t)in->imm].kind == GVM_VALUE_STRUCT) {
    int rc = vm_value_clone(&cloned, &vm->const_pool[(size_t)in->imm]);
    if (rc != GVM_OK) {
      return rc;
    }
    vm_release_global_value(vm, in->b);
    vm->globals[in->b] = cloned;
    return GVM_OK;
  }
  vm_release_global_value(vm, in->b);
  vm_value_copy(&vm->globals[in->b], &vm->const_pool[(size_t)in->imm]);
  return 0;
}

int op_copy_global(graphion_vm *vm, const graphion_insn *in) {
  graphion_vm_value cloned;
  if (vm->globals == NULL) {
    return GVM_ERR_GLOBALS_UNBOUND;
  }
  if (in->imm < 0 || (size_t)in->imm >= vm->global_count || (size_t)in->b >= vm->global_count) {
    return GVM_ERR_INVALID_GLOBAL_INDEX;
  }
  if (vm->globals[(size_t)in->imm].kind == GVM_VALUE_STRING &&
      vm->globals[(size_t)in->imm].as.string_value != NULL) {
    return vm_global_set_string_copy(vm, (size_t)in->b, vm->globals[(size_t)in->imm].as.string_value);
  }
  if (vm->globals[(size_t)in->imm].kind == GVM_VALUE_LIST ||
      vm->globals[(size_t)in->imm].kind == GVM_VALUE_DICT ||
      vm->globals[(size_t)in->imm].kind == GVM_VALUE_TUPLE ||
      vm->globals[(size_t)in->imm].kind == GVM_VALUE_SET ||
      vm->globals[(size_t)in->imm].kind == GVM_VALUE_GRAPH_REF ||
      vm->globals[(size_t)in->imm].kind == GVM_VALUE_HYPERGRAPH_REF ||
      vm->globals[(size_t)in->imm].kind == GVM_VALUE_STRUCT_TYPE ||
      vm->globals[(size_t)in->imm].kind == GVM_VALUE_STRUCT) {
    int rc = vm_value_clone(&cloned, &vm->globals[(size_t)in->imm]);
    if (rc != GVM_OK) {
      return rc;
    }
    vm_release_global_value(vm, in->b);
    vm->globals[in->b] = cloned;
    return GVM_OK;
  }
  vm_release_global_value(vm, in->b);
  vm_value_copy(&vm->globals[in->b], &vm->globals[(size_t)in->imm]);
  return 0;
}

int op_list_new(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  return vm_reg_set_empty_list(vm, in->a);
}

int op_list_append(graphion_vm *vm, const graphion_insn *in) {
  return vm_list_append_reg(vm, in->a, in->b);
}

int op_list_get(graphion_vm *vm, const graphion_insn *in) {
  return vm_list_get_element(vm, in->a, in->b);
}

int op_dict_new(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  return vm_reg_set_empty_dict(vm, in->a);
}

int op_dict_set(graphion_vm *vm, const graphion_insn *in) {
  if (vm->const_pool == NULL || in->imm < 0 || (size_t)in->imm >= vm->const_count) {
    return GVM_ERR_INVALID_CONST_INDEX;
  }
  if (vm->const_pool[(size_t)in->imm].kind != GVM_VALUE_STRING) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  return vm_dict_set_reg(vm,
                         in->a,
                         vm->const_pool[(size_t)in->imm].as.string_value != NULL
                             ? vm->const_pool[(size_t)in->imm].as.string_value
                             : "",
                         in->b);
}

int op_dict_get(graphion_vm *vm, const graphion_insn *in) {
  return vm_dict_get_element(vm, in->a, in->b);
}

int op_dict_set_key(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b) || in->imm < 0 || in->imm > 15) {
    return GVM_ERR_INVALID_REG;
  }
  return vm_dict_set_element(vm, in->a, in->b, (uint8_t)in->imm);
}

int op_tuple_new(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  return vm_reg_set_empty_tuple(vm, in->a);
}

int op_tuple_append(graphion_vm *vm, const graphion_insn *in) {
  return vm_tuple_append_reg(vm, in->a, in->b);
}

int op_set_new(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  return vm_reg_set_empty_set(vm, in->a);
}

int op_set_add(graphion_vm *vm, const graphion_insn *in) {
  return vm_set_add_reg(vm, in->a, in->b);
}

int op_set_contains(graphion_vm *vm, const graphion_insn *in) {
  return vm_collection_contains_reg(vm, in->a, in->b);
}

int op_graph_new(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (in->imm < 0) {
    return GVM_ERR_INVALID_ARG;
  }
  return vm_reg_set_graph_node_count(vm, in->a, (size_t)in->imm);
}

int op_hypergraph_new(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (in->imm != 0) {
    return GVM_ERR_INVALID_ARG;
  }
  return vm_reg_set_empty_hypergraph(vm, in->a);
}

int op_struct_new(graphion_vm *vm, const graphion_insn *in) {
  graphion_vm_value instance;
  int rc;

  if (vm == NULL || !is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  memset(&instance, 0, sizeof(instance));
  instance.kind = GVM_VALUE_NONE;
  rc = vm_value_instantiate_struct(&instance, &vm->regs[in->a], &vm->regs[in->b]);
  if (rc != GVM_OK) {
    return rc;
  }
  vm_free_owned_reg_string(vm, in->a);
  vm->regs[in->a] = instance;
  return GVM_OK;
}
