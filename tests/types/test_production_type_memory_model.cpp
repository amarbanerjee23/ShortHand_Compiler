#include "type_system/ProductionTypeSystem.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

using shorthand::types::Field;
using shorthand::types::OwnershipSnapshot;
using shorthand::types::OwnershipState;
using shorthand::types::OwnershipTracker;
using shorthand::types::TypeDescriptor;
using shorthand::types::TypeKind;

namespace {

int failures = 0;

void expect(bool condition, const std::string &message) {
    if (condition) return;
    std::cerr << "FAIL " << message << '\n';
    ++failures;
}

void typeContracts() {
    std::string diagnostic;
    std::size_t bytes = 0;
    const TypeDescriptor ints = TypeDescriptor::array(TypeKind::Int32, 8);
    expect(ints.validate(diagnostic), "valid fixed int32 array");
    expect(ints.storageBytes(bytes, diagnostic) && bytes == 32, "fixed array storage size");
    expect(ints.canonicalName() == "array<int32,8>", "canonical array name");
    expect(!TypeDescriptor::array(TypeKind::Int32, 0).validate(diagnostic), "zero array extent rejected");
    expect(diagnostic.find("SHD3010") != std::string::npos, "invalid type has stable diagnostic");

    const TypeDescriptor slice = TypeDescriptor::slice(TypeKind::Float64);
    expect(slice.validate(diagnostic), "valid float64 slice");
    expect(slice.canonicalName() == "slice<float64>", "canonical slice name");

    const TypeDescriptor record = TypeDescriptor::record(
        "InferenceEvidence", {{"latency_ms", TypeKind::Float64}, {"success", TypeKind::Bool}});
    expect(record.validate(diagnostic), "valid record");
    expect(!TypeDescriptor::record("Bad", {{"x", TypeKind::Int32}, {"x", TypeKind::Bool}})
                .validate(diagnostic),
           "duplicate record field rejected");

    const TypeDescriptor enumeration =
        TypeDescriptor::enumeration("ExecutionState", {"success", "not_executed", "failed"});
    expect(enumeration.validate(diagnostic), "valid enum");
    expect(!TypeDescriptor::enumeration("Bad", {"same", "same"}).validate(diagnostic),
           "duplicate enum variant rejected");
    expect(!shorthand::types::isAssignable(
               record,
               TypeDescriptor::record(
                   "InferenceEvidence", {{"latency_ms", TypeKind::Int32}, {"success", TypeKind::Bool}})),
           "same record name with a different field schema is not assignable");
    expect(!shorthand::types::isAssignable(
               enumeration,
               TypeDescriptor::enumeration("ExecutionState", {"success", "failed"})),
           "same enum name with a different variant schema is not assignable");

    expect(TypeDescriptor::option(TypeKind::String).validate(diagnostic), "valid option payload");
    expect(TypeDescriptor::result(TypeKind::Float64, TypeKind::String).validate(diagnostic),
           "valid result payloads");
    expect(!TypeDescriptor::option(TypeKind::Void).validate(diagnostic), "void option rejected");
    expect(TypeDescriptor::scalar(TypeKind::String).storageBytes(bytes, diagnostic) &&
               bytes == sizeof(void *),
           "immutable string handle storage matches current lowering");

    expect(shorthand::types::isAssignable(ints, TypeDescriptor::array(TypeKind::Int32, 8)),
           "identical arrays assignable");
    expect(!shorthand::types::isAssignable(ints, TypeDescriptor::array(TypeKind::Int32, 9)),
           "different extents not assignable");
    expect(!shorthand::types::isAssignable(TypeDescriptor::scalar(TypeKind::Int32),
                                           TypeDescriptor::scalar(TypeKind::Float64)),
           "implicit numeric narrowing rejected");

    expect(shorthand::types::checkedSliceRange(10, 3, 7, diagnostic), "slice ending at owner extent");
    expect(!shorthand::types::checkedSliceRange(10, 4, 7, diagnostic), "out of bounds slice rejected");
    expect(diagnostic.find("SHD7002") != std::string::npos, "slice bounds diagnostic stable");

    std::int32_t converted = 0;
    expect(shorthand::types::checkedFloatToInt(42.75, converted, diagnostic) && converted == 42,
           "checked float to int truncates toward zero");
    expect(!shorthand::types::checkedFloatToInt(std::numeric_limits<double>::infinity(),
                                                converted,
                                                diagnostic),
           "infinite float conversion rejected");
    expect(diagnostic.find("SHD3012") != std::string::npos, "conversion diagnostic stable");

    expect(!shorthand::types::checkedArrayBytes(
               std::numeric_limits<std::size_t>::max(), 2, bytes, diagnostic),
           "array byte overflow rejected");
}

void ownershipContracts() {
    std::string diagnostic;
    OwnershipTracker tracker;
    expect(tracker.declareValue("payload", diagnostic), "declare owner");
    expect(!tracker.read("payload", diagnostic), "uninitialized read rejected");
    expect(tracker.initialize("payload", diagnostic), "initialize owner");
    expect(tracker.read("payload", diagnostic), "owned read accepted");
    expect(tracker.borrowShared("payload", diagnostic), "first shared borrow");
    expect(tracker.borrowShared("payload", diagnostic), "second shared borrow");
    expect(!tracker.borrowMutable("payload", diagnostic), "mutable borrow conflicts with shared borrow");
    expect(!tracker.assign("payload", diagnostic), "assignment conflicts with shared borrow");
    expect(tracker.releaseShared("payload", diagnostic), "release first shared borrow");
    expect(tracker.releaseShared("payload", diagnostic), "release second shared borrow");
    expect(tracker.borrowMutable("payload", diagnostic), "exclusive mutable borrow");
    expect(!tracker.read("payload", diagnostic), "owner read blocked during mutable borrow");
    expect(tracker.releaseMutable("payload", diagnostic), "release mutable borrow");
    expect(tracker.moveValue("payload", diagnostic), "move owned value");
    expect(!tracker.read("payload", diagnostic), "use after move rejected");
    expect(diagnostic.find("SHD3016") != std::string::npos, "ownership diagnostic stable");
    expect(tracker.assign("payload", diagnostic), "assignment reinitializes moved value");

    OwnershipSnapshot snapshot;
    expect(tracker.snapshot("payload", snapshot) && snapshot.state == OwnershipState::Owned,
           "final ownership state is owned");
    expect(!tracker.declareValue("payload", diagnostic), "duplicate ownership declaration rejected");
}

}  // namespace

int main() {
    typeContracts();
    ownershipContracts();
    if (failures != 0) return 1;
    std::cout << "PASS production type descriptors conversions bounds and ownership state machine\n";
    return 0;
}
