/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_RUNTIME_INTERPRETER_BUILTIN_CATALOG_H
#define GRAPHION_RUNTIME_INTERPRETER_BUILTIN_CATALOG_H

#define GRAPHION_UNARY_DIRECT_BUILTINS(X)      \
  X("abs", GVM_OP_ABS)                         \
  X("sqrt", GVM_OP_SQRT)                       \
  X("expm1", GVM_OP_EXPM1)                     \
  X("exp2", GVM_OP_EXP2)                       \
  X("log1p", GVM_OP_LOG1P)                     \
  X("erf", GVM_OP_ERF)                         \
  X("erfc", GVM_OP_ERFC)                       \
  X("gamma", GVM_OP_GAMMA)                     \
  X("lgamma", GVM_OP_LGAMMA)                   \
  X("exp", GVM_OP_EXP)                         \
  X("ln", GVM_OP_LN)                           \
  X("floor", GVM_OP_FLOOR)                     \
  X("ceil", GVM_OP_CEIL)                       \
  X("round", GVM_OP_ROUND)                     \
  X("trunc", GVM_OP_TRUNC)                     \
  X("fract", GVM_OP_FRACT)                     \
  X("sign", GVM_OP_SIGN)                       \
  X("cbrt", GVM_OP_CBRT)                       \
  X("sin", GVM_OP_SIN)                         \
  X("csc", GVM_OP_CSC)                         \
  X("sec", GVM_OP_SEC)                         \
  X("cot", GVM_OP_COT)                         \
  X("acsc", GVM_OP_ACSC)                       \
  X("asec", GVM_OP_ASEC)                       \
  X("acot", GVM_OP_ACOT)                       \
  X("sech", GVM_OP_SECH)                       \
  X("csch", GVM_OP_CSCH)                       \
  X("coth", GVM_OP_COTH)                       \
  X("sinh", GVM_OP_SINH)                       \
  X("asinh", GVM_OP_ASINH)                     \
  X("acosh", GVM_OP_ACOSH)                     \
  X("cosh", GVM_OP_COSH)                       \
  X("tanh", GVM_OP_TANH)                       \
  X("atanh", GVM_OP_ATANH)                     \
  X("cos", GVM_OP_COS)                         \
  X("tan", GVM_OP_TAN)                         \
  X("asin", GVM_OP_ASIN)                       \
  X("acos", GVM_OP_ACOS)                       \
  X("atan", GVM_OP_ATAN)                       \
  X("degrees", GVM_OP_DEGREES)                 \
  X("radians", GVM_OP_RADIANS)                 \
  X("isnan", GVM_OP_ISNAN)                     \
  X("isinf", GVM_OP_ISINF)                     \
  X("isfinite", GVM_OP_ISFINITE)               \
  X("rint", GVM_OP_RINT)                       \
  X("len", GVM_OP_LEN)                         \
  X("node_count", GVM_OP_GRAPH_NODE_COUNT)     \
  X("edge_count", GVM_OP_GRAPH_EDGE_COUNT)     \
  X("is_directed", GVM_OP_GRAPH_IS_DIRECTED) \
  X("is_weighted", GVM_OP_GRAPH_IS_WEIGHTED) \
  X("orientation", GVM_OP_GRAPH_ORIENTATION)

#define GRAPHION_BINARY_DIRECT_BUILTINS(X) \
  X("min", GVM_OP_MIN)                     \
  X("max", GVM_OP_MAX)                     \
  X("atan2", GVM_OP_ATAN2)                 \
  X("hypot", GVM_OP_HYPOT)                 \
  X("copysign", GVM_OP_COPYSIGN)           \
  X("fdim", GVM_OP_FDIM)                   \
  X("remainder", GVM_OP_REMAINDER)         \
  X("log", GVM_OP_LOG)

#define GRAPHION_TERNARY_DIRECT_BUILTINS(X) \
  X("clamp", GVM_OP_CLAMP)                  \
  X("fma", GVM_OP_FMA)

#define GRAPHION_SPECIAL_BUILTIN_NAMES(X) \
  X("log10")                              \
  X("log2")

#endif
