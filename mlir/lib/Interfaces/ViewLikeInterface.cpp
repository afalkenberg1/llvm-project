//===- ViewLikeInterface.cpp - View-like operations in MLIR ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "mlir/Interfaces/ViewLikeInterface.h"

using namespace mlir;

//===----------------------------------------------------------------------===//
// ViewLike Interfaces
//===----------------------------------------------------------------------===//

/// Include the definitions of the loop-like interfaces.
#include "mlir/Interfaces/ViewLikeInterface.cpp.inc"

LogicalResult mlir::verifyListOfOperandsOrIntegers(Operation *op,
                                                   StringRef name,
                                                   unsigned numElements,
                                                   ArrayRef<int64_t> staticVals,
                                                   ValueRange values) {
  // Check static and dynamic offsets/sizes/strides does not overflow type.
  if (staticVals.size() != numElements)
    return op->emitError("expected ") << numElements << " " << name
                                      << " values, got " << staticVals.size();
  unsigned expectedNumDynamicEntries =
      llvm::count_if(staticVals, ShapedType::isDynamic);
  if (values.size() != expectedNumDynamicEntries)
    return op->emitError("expected ")
           << expectedNumDynamicEntries << " dynamic " << name << " values";
  return success();
}

SliceBoundsVerificationResult mlir::verifyInBoundsSlice(
    ArrayRef<int64_t> shape, ArrayRef<int64_t> staticOffsets,
    ArrayRef<int64_t> staticSizes, ArrayRef<int64_t> staticStrides,
    bool generateErrorMessage) {
  SliceBoundsVerificationResult result;
  result.isValid = true;
  for (int64_t i = 0, e = shape.size(); i < e; ++i) {
    // Nothing to verify for dynamic source dims.
    if (ShapedType::isDynamic(shape[i]))
      continue;
    // Nothing to verify if the offset is dynamic.
    if (ShapedType::isDynamic(staticOffsets[i]))
      continue;
    if (staticOffsets[i] >= shape[i]) {
      result.errorMessage =
          std::string("offset ") + std::to_string(i) +
          " is out-of-bounds: " + std::to_string(staticOffsets[i]) +
          " >= " + std::to_string(shape[i]);
      result.isValid = false;
      return result;
    }
    if (ShapedType::isDynamic(staticSizes[i]) ||
        ShapedType::isDynamic(staticStrides[i]))
      continue;
    int64_t lastPos =
        staticOffsets[i] + (staticSizes[i] - 1) * staticStrides[i];
    if (lastPos >= shape[i]) {
      result.errorMessage = std::string("slice along dimension ") +
                            std::to_string(i) +
                            " runs out-of-bounds: " + std::to_string(lastPos) +
                            " >= " + std::to_string(shape[i]);
      result.isValid = false;
      return result;
    }
  }
  return result;
}

SliceBoundsVerificationResult mlir::verifyInBoundsSlice(
    ArrayRef<int64_t> shape, ArrayRef<OpFoldResult> mixedOffsets,
    ArrayRef<OpFoldResult> mixedSizes, ArrayRef<OpFoldResult> mixedStrides,
    bool generateErrorMessage) {
  auto getStaticValues = [](ArrayRef<OpFoldResult> ofrs) {
    SmallVector<int64_t> staticValues;
    for (OpFoldResult ofr : ofrs) {
      if (auto attr = dyn_cast<Attribute>(ofr)) {
        staticValues.push_back(cast<IntegerAttr>(attr).getInt());
      } else {
        staticValues.push_back(ShapedType::kDynamic);
      }
    }
    return staticValues;
  };
  return verifyInBoundsSlice(
      shape, getStaticValues(mixedOffsets), getStaticValues(mixedSizes),
      getStaticValues(mixedStrides), generateErrorMessage);
}

LogicalResult
mlir::detail::verifyOffsetSizeAndStrideOp(OffsetSizeAndStrideOpInterface op) {
  // A dynamic size is represented as ShapedType::kDynamic in `static_sizes`.
  // Its corresponding Value appears in `sizes`. Thus, the number of dynamic
  // dimensions in `static_sizes` must equal the rank of `sizes`.
  // The same applies to strides and offsets.
  size_t numDynamicDims =
      llvm::count_if(op.getStaticSizes(), ShapedType::isDynamic);
  if (op.getSizes().size() != numDynamicDims) {
    return op->emitError("expected the number of 'sizes' to match the number "
                         "of dynamic entries in 'static_sizes' (")
           << op.getSizes().size() << " vs " << numDynamicDims << ")";
  }
  size_t numDynamicStrides =
      llvm::count_if(op.getStaticStrides(), ShapedType::isDynamic);
  if (op.getStrides().size() != numDynamicStrides) {
    return op->emitError("expected the number of 'strides' to match the number "
                         "of dynamic entries in 'static_strides' (")
           << op.getStrides().size() << " vs " << numDynamicStrides << ")";
  }
  size_t numDynamicOffsets =
      llvm::count_if(op.getStaticOffsets(), ShapedType::isDynamic);
  if (op.getOffsets().size() != numDynamicOffsets) {
    return op->emitError("expected the number of 'offsets' to match the number "
                         "of dynamic entries in 'static_offsets' (")
           << op.getOffsets().size() << " vs " << numDynamicOffsets << ")";
  }

  std::array<unsigned, 3> maxRanks = op.getArrayAttrMaxRanks();
  // Offsets can come in 2 flavors:
  //   1. Either single entry (when maxRanks == 1).
  //   2. Or as an array whose rank must match that of the mixed sizes.
  // So that the result type is well-formed.
  if (!(op.getMixedOffsets().size() == 1 && maxRanks[0] == 1) && // NOLINT
      op.getMixedOffsets().size() != op.getMixedSizes().size())
    return op->emitError(
               "expected mixed offsets rank to match mixed sizes rank (")
           << op.getMixedOffsets().size() << " vs " << op.getMixedSizes().size()
           << ") so the rank of the result type is well-formed.";
  // Ranks of mixed sizes and strides must always match so the result type is
  // well-formed.
  if (op.getMixedSizes().size() != op.getMixedStrides().size())
    return op->emitError(
               "expected mixed sizes rank to match mixed strides rank (")
           << op.getMixedSizes().size() << " vs " << op.getMixedStrides().size()
           << ") so the rank of the result type is well-formed.";

  if (failed(verifyListOfOperandsOrIntegers(
          op, "offset", maxRanks[0], op.getStaticOffsets(), op.getOffsets())))
    return failure();
  if (failed(verifyListOfOperandsOrIntegers(
          op, "size", maxRanks[1], op.getStaticSizes(), op.getSizes())))
    return failure();
  if (failed(verifyListOfOperandsOrIntegers(
          op, "stride", maxRanks[2], op.getStaticStrides(), op.getStrides())))
    return failure();

  for (int64_t offset : op.getStaticOffsets()) {
    if (offset < 0 && ShapedType::isStatic(offset))
      return op->emitError("expected offsets to be non-negative, but got ")
             << offset;
  }
  for (int64_t size : op.getStaticSizes()) {
    if (size < 0 && ShapedType::isStatic(size))
      return op->emitError("expected sizes to be non-negative, but got ")
             << size;
  }
  return success();
}

static char getLeftDelimiter(AsmParser::Delimiter delimiter) {
  switch (delimiter) {
  case AsmParser::Delimiter::Paren:
    return '(';
  case AsmParser::Delimiter::LessGreater:
    return '<';
  case AsmParser::Delimiter::Square:
    return '[';
  case AsmParser::Delimiter::Braces:
    return '{';
  default:
    llvm_unreachable("unsupported delimiter");
  }
}

static char getRightDelimiter(AsmParser::Delimiter delimiter) {
  switch (delimiter) {
  case AsmParser::Delimiter::Paren:
    return ')';
  case AsmParser::Delimiter::LessGreater:
    return '>';
  case AsmParser::Delimiter::Square:
    return ']';
  case AsmParser::Delimiter::Braces:
    return '}';
  default:
    llvm_unreachable("unsupported delimiter");
  }
}

void mlir::printDynamicIndexList(OpAsmPrinter &printer, Operation *op,
                                 OperandRange values,
                                 ArrayRef<int64_t> integers,
                                 ArrayRef<bool> scalableFlags,
                                 TypeRange valueTypes,
                                 AsmParser::Delimiter delimiter) {
  char leftDelimiter = getLeftDelimiter(delimiter);
  char rightDelimiter = getRightDelimiter(delimiter);
  printer << leftDelimiter;
  if (integers.empty()) {
    printer << rightDelimiter;
    return;
  }

  unsigned dynamicValIdx = 0;
  unsigned scalableIndexIdx = 0;
  llvm::interleaveComma(integers, printer, [&](int64_t integer) {
    if (!scalableFlags.empty() && scalableFlags[scalableIndexIdx])
      printer << "[";
    if (ShapedType::isDynamic(integer)) {
      printer << values[dynamicValIdx];
      if (!valueTypes.empty())
        printer << " : " << valueTypes[dynamicValIdx];
      ++dynamicValIdx;
    } else {
      printer << integer;
    }
    if (!scalableFlags.empty() && scalableFlags[scalableIndexIdx])
      printer << "]";

    scalableIndexIdx++;
  });

  printer << rightDelimiter;
}

ParseResult mlir::parseDynamicIndexList(
    OpAsmParser &parser,
    SmallVectorImpl<OpAsmParser::UnresolvedOperand> &values,
    DenseI64ArrayAttr &integers, DenseBoolArrayAttr &scalableFlags,
    SmallVectorImpl<Type> *valueTypes, AsmParser::Delimiter delimiter) {

  SmallVector<int64_t, 4> integerVals;
  SmallVector<bool, 4> scalableVals;
  auto parseIntegerOrValue = [&]() {
    OpAsmParser::UnresolvedOperand operand;
    auto res = parser.parseOptionalOperand(operand);

    // When encountering `[`, assume that this is a scalable index.
    scalableVals.push_back(parser.parseOptionalLSquare().succeeded());

    if (res.has_value() && succeeded(res.value())) {
      values.push_back(operand);
      integerVals.push_back(ShapedType::kDynamic);
      if (valueTypes && parser.parseColonType(valueTypes->emplace_back()))
        return failure();
    } else {
      int64_t integer;
      if (failed(parser.parseInteger(integer)))
        return failure();
      integerVals.push_back(integer);
    }

    // If this is assumed to be a scalable index, verify that there's a closing
    // `]`.
    if (scalableVals.back() && parser.parseOptionalRSquare().failed())
      return failure();
    return success();
  };
  if (parser.parseCommaSeparatedList(delimiter, parseIntegerOrValue,
                                     " in dynamic index list"))
    return parser.emitError(parser.getNameLoc())
           << "expected SSA value or integer";
  integers = parser.getBuilder().getDenseI64ArrayAttr(integerVals);
  scalableFlags = parser.getBuilder().getDenseBoolArrayAttr(scalableVals);
  return success();
}

bool mlir::detail::sameOffsetsSizesAndStrides(
    OffsetSizeAndStrideOpInterface a, OffsetSizeAndStrideOpInterface b,
    llvm::function_ref<bool(OpFoldResult, OpFoldResult)> cmp) {
  if (a.getStaticOffsets().size() != b.getStaticOffsets().size())
    return false;
  if (a.getStaticSizes().size() != b.getStaticSizes().size())
    return false;
  if (a.getStaticStrides().size() != b.getStaticStrides().size())
    return false;
  for (auto it : llvm::zip(a.getMixedOffsets(), b.getMixedOffsets()))
    if (!cmp(std::get<0>(it), std::get<1>(it)))
      return false;
  for (auto it : llvm::zip(a.getMixedSizes(), b.getMixedSizes()))
    if (!cmp(std::get<0>(it), std::get<1>(it)))
      return false;
  for (auto it : llvm::zip(a.getMixedStrides(), b.getMixedStrides()))
    if (!cmp(std::get<0>(it), std::get<1>(it)))
      return false;
  return true;
}

unsigned mlir::detail::getNumDynamicEntriesUpToIdx(ArrayRef<int64_t> staticVals,
                                                   unsigned idx) {
  return std::count_if(staticVals.begin(), staticVals.begin() + idx,
                       ShapedType::isDynamic);
}
