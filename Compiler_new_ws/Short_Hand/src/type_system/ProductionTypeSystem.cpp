#include "ProductionTypeSystem.h"

#include <cmath>
#include <limits>
#include <set>
#include <utility>

namespace shorthand::types {
namespace {

bool fail(std::string &diagnostic, const std::string &message) {
    diagnostic = message;
    return false;
}

bool validPayload(TypeKind kind) {
    return kind != TypeKind::Void && kind != TypeKind::Array && kind != TypeKind::Slice &&
           kind != TypeKind::Record && kind != TypeKind::Enum && kind != TypeKind::Option &&
           kind != TypeKind::Result;
}

std::size_t scalarBytes(TypeKind kind) {
    switch (kind) {
        case TypeKind::Bool: return 1;
        case TypeKind::Int32: return 4;
        case TypeKind::Float64: return 8;
        case TypeKind::String: return sizeof(void *);
        default: return 0;
    }
}

OwnershipSnapshot *findValue(std::map<std::string, OwnershipSnapshot> &values,
                             const std::string &name,
                             std::string &diagnostic) {
    const auto found = values.find(name);
    if (found == values.end()) {
        diagnostic = "SHD3016 ownership operation references undeclared value `" + name + "`";
        return nullptr;
    }
    return &found->second;
}

}  // namespace

const char *typeKindName(TypeKind kind) {
    switch (kind) {
        case TypeKind::Void: return "void";
        case TypeKind::Bool: return "bool";
        case TypeKind::Int32: return "int32";
        case TypeKind::Float64: return "float64";
        case TypeKind::String: return "string";
        case TypeKind::Array: return "array";
        case TypeKind::Slice: return "slice";
        case TypeKind::Record: return "record";
        case TypeKind::Enum: return "enum";
        case TypeKind::Option: return "option";
        case TypeKind::Result: return "result";
    }
    return "unknown";
}

const char *ownershipStateName(OwnershipState state) {
    switch (state) {
        case OwnershipState::Uninitialized: return "uninitialized";
        case OwnershipState::Owned: return "owned";
        case OwnershipState::SharedBorrowed: return "shared_borrowed";
        case OwnershipState::MutablyBorrowed: return "mutably_borrowed";
        case OwnershipState::Moved: return "moved";
    }
    return "unknown";
}

TypeDescriptor TypeDescriptor::scalar(TypeKind value) {
    TypeDescriptor descriptor;
    descriptor.kind = value;
    return descriptor;
}

TypeDescriptor TypeDescriptor::array(TypeKind value, std::size_t value_extent) {
    TypeDescriptor descriptor;
    descriptor.kind = TypeKind::Array;
    descriptor.element = value;
    descriptor.extent = value_extent;
    return descriptor;
}

TypeDescriptor TypeDescriptor::slice(TypeKind value) {
    TypeDescriptor descriptor;
    descriptor.kind = TypeKind::Slice;
    descriptor.element = value;
    return descriptor;
}

TypeDescriptor TypeDescriptor::record(std::string value_name, std::vector<Field> value_fields) {
    TypeDescriptor descriptor;
    descriptor.kind = TypeKind::Record;
    descriptor.name = std::move(value_name);
    descriptor.fields = std::move(value_fields);
    return descriptor;
}

TypeDescriptor TypeDescriptor::enumeration(std::string value_name,
                                           std::vector<std::string> value_variants) {
    TypeDescriptor descriptor;
    descriptor.kind = TypeKind::Enum;
    descriptor.name = std::move(value_name);
    descriptor.variants = std::move(value_variants);
    return descriptor;
}

TypeDescriptor TypeDescriptor::option(TypeKind payload) {
    TypeDescriptor descriptor;
    descriptor.kind = TypeKind::Option;
    descriptor.element = payload;
    return descriptor;
}

TypeDescriptor TypeDescriptor::result(TypeKind value_ok, TypeKind value_error) {
    TypeDescriptor descriptor;
    descriptor.kind = TypeKind::Result;
    descriptor.ok = value_ok;
    descriptor.error = value_error;
    return descriptor;
}

bool isScalar(TypeKind kind) {
    return kind == TypeKind::Bool || kind == TypeKind::Int32 ||
           kind == TypeKind::Float64 || kind == TypeKind::String;
}

bool isNumeric(TypeKind kind) {
    return kind == TypeKind::Int32 || kind == TypeKind::Float64;
}

bool TypeDescriptor::validate(std::string &diagnostic) const {
    diagnostic.clear();
    if (isScalar(kind) || kind == TypeKind::Void) return true;
    if (kind == TypeKind::Array) {
        if (!validPayload(element)) return fail(diagnostic, "SHD3010 array element type is not a scalar value");
        if (extent == 0) return fail(diagnostic, "SHD3010 array extent must be greater than zero");
        std::size_t ignored = 0;
        return checkedArrayBytes(extent, scalarBytes(element), ignored, diagnostic);
    }
    if (kind == TypeKind::Slice) {
        if (!validPayload(element)) return fail(diagnostic, "SHD3010 slice element type is not a scalar value");
        return true;
    }
    if (kind == TypeKind::Record) {
        if (name.empty()) return fail(diagnostic, "SHD3010 record requires a stable name");
        if (fields.empty()) return fail(diagnostic, "SHD3010 record requires at least one field");
        std::set<std::string> names;
        for (const Field &field : fields) {
            if (field.name.empty() || !names.insert(field.name).second)
                return fail(diagnostic, "SHD3010 record field names must be non-empty and unique");
            if (!validPayload(field.kind))
                return fail(diagnostic, "SHD3010 record field must use a scalar value type");
        }
        return true;
    }
    if (kind == TypeKind::Enum) {
        if (name.empty()) return fail(diagnostic, "SHD3010 enum requires a stable name");
        if (variants.empty()) return fail(diagnostic, "SHD3010 enum requires at least one variant");
        std::set<std::string> names;
        for (const std::string &variant : variants) {
            if (variant.empty() || !names.insert(variant).second)
                return fail(diagnostic, "SHD3010 enum variants must be non-empty and unique");
        }
        return true;
    }
    if (kind == TypeKind::Option) {
        if (!validPayload(element)) return fail(diagnostic, "SHD3010 option payload must be a scalar value type");
        return true;
    }
    if (kind == TypeKind::Result) {
        if (!validPayload(ok) || !validPayload(error))
            return fail(diagnostic, "SHD3010 result payloads must be scalar value types");
        return true;
    }
    return fail(diagnostic, "SHD3010 unknown production type descriptor");
}

bool checkedArrayBytes(std::size_t extent,
                       std::size_t element_bytes,
                       std::size_t &bytes,
                       std::string &diagnostic) {
    bytes = 0;
    if (extent == 0 || element_bytes == 0)
        return fail(diagnostic, "SHD3011 array storage requires non-zero extent and element size");
    if (extent > std::numeric_limits<std::size_t>::max() / element_bytes)
        return fail(diagnostic, "SHD3011 array storage size overflows size_t");
    bytes = extent * element_bytes;
    diagnostic.clear();
    return true;
}

bool TypeDescriptor::storageBytes(std::size_t &bytes, std::string &diagnostic) const {
    bytes = 0;
    if (!validate(diagnostic)) return false;
    if (kind == TypeKind::Void) return true;
    if (isScalar(kind)) {
        bytes = scalarBytes(kind);
        return true;
    }
    if (kind == TypeKind::Array) return checkedArrayBytes(extent, scalarBytes(element), bytes, diagnostic);
    if (kind == TypeKind::Slice || kind == TypeKind::String) {
        bytes = sizeof(void *) + sizeof(std::size_t);
        return true;
    }
    if (kind == TypeKind::Enum) {
        bytes = sizeof(std::uint32_t);
        return true;
    }
    if (kind == TypeKind::Option) {
        bytes = 1 + scalarBytes(element);
        return true;
    }
    if (kind == TypeKind::Result) {
        bytes = 1 + (scalarBytes(ok) > scalarBytes(error) ? scalarBytes(ok) : scalarBytes(error));
        return true;
    }
    if (kind == TypeKind::Record) {
        for (const Field &field : fields) {
            const std::size_t field_bytes = scalarBytes(field.kind);
            if (bytes > std::numeric_limits<std::size_t>::max() - field_bytes)
                return fail(diagnostic, "SHD3011 record storage size overflows size_t");
            bytes += field_bytes;
        }
        return true;
    }
    return fail(diagnostic, "SHD3011 storage layout is unavailable");
}

std::string TypeDescriptor::canonicalName() const {
    if (isScalar(kind) || kind == TypeKind::Void) return typeKindName(kind);
    if (kind == TypeKind::Array)
        return "array<" + std::string(typeKindName(element)) + "," + std::to_string(extent) + ">";
    if (kind == TypeKind::Slice) return "slice<" + std::string(typeKindName(element)) + ">";
    if (kind == TypeKind::Record || kind == TypeKind::Enum)
        return std::string(typeKindName(kind)) + "<" + name + ">";
    if (kind == TypeKind::Option) return "option<" + std::string(typeKindName(element)) + ">";
    if (kind == TypeKind::Result)
        return "result<" + std::string(typeKindName(ok)) + "," + typeKindName(error) + ">";
    return "unknown";
}

bool isAssignable(const TypeDescriptor &target, const TypeDescriptor &source) {
    std::string diagnostic;
    if (!target.validate(diagnostic) || !source.validate(diagnostic)) return false;
    if (target.kind != source.kind) return false;
    if (isScalar(target.kind) || target.kind == TypeKind::Void) return true;
    if (target.kind == TypeKind::Array)
        return target.element == source.element && target.extent == source.extent;
    if (target.kind == TypeKind::Slice) return target.element == source.element;
    if (target.kind == TypeKind::Record) {
        if (target.name.empty() || target.name != source.name ||
            target.fields.size() != source.fields.size()) return false;
        for (std::size_t i = 0; i < target.fields.size(); ++i) {
            if (target.fields[i].name != source.fields[i].name ||
                target.fields[i].kind != source.fields[i].kind) return false;
        }
        return true;
    }
    if (target.kind == TypeKind::Enum)
        return !target.name.empty() && target.name == source.name &&
               target.variants == source.variants;
    if (target.kind == TypeKind::Option) return target.element == source.element;
    if (target.kind == TypeKind::Result) return target.ok == source.ok && target.error == source.error;
    return false;
}

bool checkedFloatToInt(double value, std::int32_t &result, std::string &diagnostic) {
    if (!std::isfinite(value)) return fail(diagnostic, "SHD3012 cannot convert NaN or infinity to int32");
    if (value < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
        value > static_cast<double>(std::numeric_limits<std::int32_t>::max()))
        return fail(diagnostic, "SHD3012 float64 value is outside int32 range");
    result = static_cast<std::int32_t>(value);
    diagnostic.clear();
    return true;
}

bool checkedSliceRange(std::size_t owner_extent,
                       std::size_t offset,
                       std::size_t length,
                       std::string &diagnostic) {
    if (offset > owner_extent || length > owner_extent - offset)
        return fail(diagnostic, "SHD7002 slice range is outside owner bounds");
    diagnostic.clear();
    return true;
}

bool OwnershipTracker::declareValue(const std::string &name, std::string &diagnostic) {
    if (name.empty()) return fail(diagnostic, "SHD3016 ownership value name must not be empty");
    if (!values_.emplace(name, OwnershipSnapshot{}).second)
        return fail(diagnostic, "SHD3016 ownership value is already declared: `" + name + "`");
    diagnostic.clear();
    return true;
}

bool OwnershipTracker::initialize(const std::string &name, std::string &diagnostic) {
    OwnershipSnapshot *value = findValue(values_, name, diagnostic);
    if (value == nullptr) return false;
    if (value->state != OwnershipState::Uninitialized && value->state != OwnershipState::Moved)
        return fail(diagnostic, "SHD3016 initialize requires uninitialized or moved state");
    value->state = OwnershipState::Owned;
    value->shared_borrows = 0;
    diagnostic.clear();
    return true;
}

bool OwnershipTracker::read(const std::string &name, std::string &diagnostic) const {
    const auto found = values_.find(name);
    if (found == values_.end()) return fail(diagnostic, "SHD3016 read references undeclared value `" + name + "`");
    if (found->second.state == OwnershipState::Uninitialized)
        return fail(diagnostic, "SHD3016 read of uninitialized value `" + name + "`");
    if (found->second.state == OwnershipState::Moved)
        return fail(diagnostic, "SHD3016 use after move of value `" + name + "`");
    if (found->second.state == OwnershipState::MutablyBorrowed)
        return fail(diagnostic, "SHD3016 owner read is forbidden during a mutable borrow of `" + name + "`");
    diagnostic.clear();
    return true;
}

bool OwnershipTracker::moveValue(const std::string &name, std::string &diagnostic) {
    OwnershipSnapshot *value = findValue(values_, name, diagnostic);
    if (value == nullptr) return false;
    if (value->state != OwnershipState::Owned)
        return fail(diagnostic, "SHD3016 move requires an owned value with no active borrow");
    value->state = OwnershipState::Moved;
    diagnostic.clear();
    return true;
}

bool OwnershipTracker::assign(const std::string &name, std::string &diagnostic) {
    OwnershipSnapshot *value = findValue(values_, name, diagnostic);
    if (value == nullptr) return false;
    if (value->state == OwnershipState::SharedBorrowed || value->state == OwnershipState::MutablyBorrowed)
        return fail(diagnostic, "SHD3016 assignment is forbidden while value is borrowed");
    value->state = OwnershipState::Owned;
    value->shared_borrows = 0;
    diagnostic.clear();
    return true;
}

bool OwnershipTracker::borrowShared(const std::string &name, std::string &diagnostic) {
    OwnershipSnapshot *value = findValue(values_, name, diagnostic);
    if (value == nullptr) return false;
    if (value->state != OwnershipState::Owned && value->state != OwnershipState::SharedBorrowed)
        return fail(diagnostic, "SHD3016 shared borrow requires an initialized, non-moved owner");
    value->state = OwnershipState::SharedBorrowed;
    ++value->shared_borrows;
    diagnostic.clear();
    return true;
}

bool OwnershipTracker::releaseShared(const std::string &name, std::string &diagnostic) {
    OwnershipSnapshot *value = findValue(values_, name, diagnostic);
    if (value == nullptr) return false;
    if (value->state != OwnershipState::SharedBorrowed || value->shared_borrows == 0)
        return fail(diagnostic, "SHD3016 releaseShared requires an active shared borrow");
    --value->shared_borrows;
    if (value->shared_borrows == 0) value->state = OwnershipState::Owned;
    diagnostic.clear();
    return true;
}

bool OwnershipTracker::borrowMutable(const std::string &name, std::string &diagnostic) {
    OwnershipSnapshot *value = findValue(values_, name, diagnostic);
    if (value == nullptr) return false;
    if (value->state != OwnershipState::Owned)
        return fail(diagnostic, "SHD3016 mutable borrow requires exclusive ownership");
    value->state = OwnershipState::MutablyBorrowed;
    diagnostic.clear();
    return true;
}

bool OwnershipTracker::releaseMutable(const std::string &name, std::string &diagnostic) {
    OwnershipSnapshot *value = findValue(values_, name, diagnostic);
    if (value == nullptr) return false;
    if (value->state != OwnershipState::MutablyBorrowed)
        return fail(diagnostic, "SHD3016 releaseMutable requires an active mutable borrow");
    value->state = OwnershipState::Owned;
    diagnostic.clear();
    return true;
}

bool OwnershipTracker::snapshot(const std::string &name, OwnershipSnapshot &value) const {
    const auto found = values_.find(name);
    if (found == values_.end()) return false;
    value = found->second;
    return true;
}

}  // namespace shorthand::types
