#ifndef SHORTHAND_PRODUCTION_TYPE_SYSTEM_H
#define SHORTHAND_PRODUCTION_TYPE_SYSTEM_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace shorthand::types {

enum class TypeKind {
    Void,
    Bool,
    Int32,
    Float64,
    String,
    Array,
    Slice,
    Record,
    Enum,
    Option,
    Result
};

struct Field {
    std::string name;
    TypeKind kind = TypeKind::Void;
};

struct TypeDescriptor {
    TypeKind kind = TypeKind::Void;
    std::string name;
    TypeKind element = TypeKind::Void;
    std::size_t extent = 0;
    std::vector<Field> fields;
    std::vector<std::string> variants;
    TypeKind ok = TypeKind::Void;
    TypeKind error = TypeKind::Void;

    static TypeDescriptor scalar(TypeKind kind);
    static TypeDescriptor array(TypeKind element, std::size_t extent);
    static TypeDescriptor slice(TypeKind element);
    static TypeDescriptor record(std::string name, std::vector<Field> fields);
    static TypeDescriptor enumeration(std::string name, std::vector<std::string> variants);
    static TypeDescriptor option(TypeKind payload);
    static TypeDescriptor result(TypeKind ok, TypeKind error);

    bool validate(std::string &diagnostic) const;
    bool storageBytes(std::size_t &bytes, std::string &diagnostic) const;
    std::string canonicalName() const;
};

bool isScalar(TypeKind kind);
bool isNumeric(TypeKind kind);
bool isAssignable(const TypeDescriptor &target, const TypeDescriptor &source);
bool checkedFloatToInt(double value, std::int32_t &result, std::string &diagnostic);
bool checkedArrayBytes(std::size_t extent,
                       std::size_t element_bytes,
                       std::size_t &bytes,
                       std::string &diagnostic);
bool checkedSliceRange(std::size_t owner_extent,
                       std::size_t offset,
                       std::size_t length,
                       std::string &diagnostic);

enum class OwnershipState {
    Uninitialized,
    Owned,
    SharedBorrowed,
    MutablyBorrowed,
    Moved
};

struct OwnershipSnapshot {
    OwnershipState state = OwnershipState::Uninitialized;
    std::size_t shared_borrows = 0;
};

class OwnershipTracker {
public:
    bool declareValue(const std::string &name, std::string &diagnostic);
    bool initialize(const std::string &name, std::string &diagnostic);
    bool read(const std::string &name, std::string &diagnostic) const;
    bool moveValue(const std::string &name, std::string &diagnostic);
    bool assign(const std::string &name, std::string &diagnostic);
    bool borrowShared(const std::string &name, std::string &diagnostic);
    bool releaseShared(const std::string &name, std::string &diagnostic);
    bool borrowMutable(const std::string &name, std::string &diagnostic);
    bool releaseMutable(const std::string &name, std::string &diagnostic);
    bool snapshot(const std::string &name, OwnershipSnapshot &value) const;

private:
    std::map<std::string, OwnershipSnapshot> values_;
};

const char *typeKindName(TypeKind kind);
const char *ownershipStateName(OwnershipState state);

}  // namespace shorthand::types

#endif
