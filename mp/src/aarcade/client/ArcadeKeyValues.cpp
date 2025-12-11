#include "cbase.h"

#include "ArcadeKeyValues.h"

#include <cstdio>
#include <cstring>
#include <iostream>


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ---------------- Constructors / Destructor ----------------

ArcadeKeyValues::ArcadeKeyValues(const std::string& keyName)
    : name(keyName),
      intValue(0),
      floatValue(0.0f),
      parent(nullptr),
      childIndex(-1),
      valueType(TYPE_NONE) {
}

ArcadeKeyValues::~ArcadeKeyValues() = default;

// ---------------- Static Factory ----------------

std::unique_ptr<ArcadeKeyValues> ArcadeKeyValues::ParseFromHex(const std::string& hexData) {
    auto bytes = hexToBytes(hexData);
    std::size_t position = 0;
    return parseRecursive(bytes, position, "root");
}

// ---------------- Core Accessors ----------------

const char* ArcadeKeyValues::GetName() const {
    return name.c_str();
}

const char* ArcadeKeyValues::GetString(const char* keyName,
                                       const char* defaultValue) const {
    if (keyName == nullptr) {
        return (valueType == TYPE_STRING) ? stringValue.c_str() : defaultValue;
    }

    for (const auto& pair : children) {
        if (pair.first == keyName && pair.second->valueType == TYPE_STRING) {
            return pair.second->stringValue.c_str();
        }
    }
    return defaultValue;
}

int ArcadeKeyValues::GetInt(const char* keyName, int defaultValue) const {
    if (keyName == nullptr) {
        return (valueType == TYPE_INT) ? intValue : defaultValue;
    }

    for (const auto& pair : children) {
        if (pair.first == keyName) {
            if (pair.second->valueType == TYPE_INT) {
                return pair.second->intValue;
            } else if (pair.second->valueType == TYPE_STRING) {
                // Try to convert string to int
                try {
                    return std::stoi(pair.second->stringValue);
                } catch (...) {
                    return defaultValue;
                }
            }
            break;
        }
    }
    return defaultValue;
}

float ArcadeKeyValues::GetFloat(const char* keyName, float defaultValue) const {
    if (keyName == nullptr) {
        return (valueType == TYPE_FLOAT) ? floatValue : defaultValue;
    }

    for (const auto& pair : children) {
        if (pair.first == keyName) {
            if (pair.second->valueType == TYPE_FLOAT) {
                return pair.second->floatValue;
            } else if (pair.second->valueType == TYPE_STRING) {
                // Try to convert string to float
                try {
                    return std::stof(pair.second->stringValue);
                } catch (...) {
                    return defaultValue;
                }
            } else if (pair.second->valueType == TYPE_INT) {
                return static_cast<float>(pair.second->intValue);
            }
            break;
        }
    }
    return defaultValue;
}

bool ArcadeKeyValues::GetBool(const char* keyName, bool defaultValue) const {
    if (keyName == nullptr) {
        if (valueType == TYPE_INT) {
            return intValue != 0;
        } else if (valueType == TYPE_STRING) {
            return stringValue == "1" || stringValue == "true" || stringValue == "True";
        }
        return defaultValue;
    }

    for (const auto& pair : children) {
        if (pair.first == keyName) {
            if (pair.second->valueType == TYPE_INT) {
                return pair.second->intValue != 0;
            } else if (pair.second->valueType == TYPE_STRING) {
                const std::string& val = pair.second->stringValue;
                return val == "1" || val == "true" || val == "True";
            }
            break;
        }
    }
    return defaultValue;
}

// ---------------- Subsection Access ----------------

ArcadeKeyValues* ArcadeKeyValues::FindKey(const char* keyName,
                                          bool createIfNotFound) {
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (children[i].first == keyName) {
            return children[i].second.get();
        }
    }

    if (createIfNotFound) {
        auto newKey = std::make_unique<ArcadeKeyValues>(keyName);
        newKey->parent = this;
        newKey->childIndex = static_cast<int>(children.size());
        newKey->valueType = TYPE_SUBSECTION;
        ArcadeKeyValues* ptr = newKey.get();
        children.push_back({ keyName, std::move(newKey) });
        return ptr;
    }

    return nullptr;
}

const ArcadeKeyValues* ArcadeKeyValues::FindKey(const char* keyName) const {
    for (const auto& pair : children) {
        if (pair.first == keyName) {
            return pair.second.get();
        }
    }
    return nullptr;
}

// ---------------- Iteration Support ----------------

ArcadeKeyValues* ArcadeKeyValues::GetFirstSubKey() const {
    if (children.empty()) return nullptr;
    return children[0].second.get();
}

ArcadeKeyValues* ArcadeKeyValues::GetNextKey() const {
    if (!parent || childIndex < 0) return nullptr;

    const auto& parentChildren = parent->children;
    int nextIndex = childIndex + 1;
    if (nextIndex < static_cast<int>(parentChildren.size())) {
        return parentChildren[nextIndex].second.get();
    }
    return nullptr;
}

// ---------------- Value Setting ----------------

void ArcadeKeyValues::SetString(const char* keyName, const char* value) {
    if (keyName == nullptr) {
        stringValue = value;
        valueType = TYPE_STRING;
    } else {
        auto key = FindKey(keyName, true);
        key->stringValue = value;
        key->valueType = TYPE_STRING;
    }
}

void ArcadeKeyValues::SetInt(const char* keyName, int value) {
    if (keyName == nullptr) {
        intValue = value;
        valueType = TYPE_INT;
    } else {
        auto key = FindKey(keyName, true);
        key->intValue = value;
        key->valueType = TYPE_INT;
    }
}

void ArcadeKeyValues::SetFloat(const char* keyName, float value) {
    if (keyName == nullptr) {
        floatValue = value;
        valueType = TYPE_FLOAT;
    } else {
        auto key = FindKey(keyName, true);
        key->floatValue = value;
        key->valueType = TYPE_FLOAT;
    }
}

void ArcadeKeyValues::SetBool(const char* keyName, bool value) {
    SetInt(keyName, value ? 1 : 0);
}

// ---------------- Remove Key ----------------

bool ArcadeKeyValues::RemoveKey(const char* keyName) {
    if (keyName == nullptr) {
        return false;
    }

    for (std::size_t i = 0; i < children.size(); ++i) {
        if (children[i].first == keyName) {
            children.erase(children.begin() + static_cast<long>(i));
            // Update childIndex for all subsequent children
            for (std::size_t j = i; j < children.size(); ++j) {
                children[j].second->childIndex = static_cast<int>(j);
            }
            return true;
        }
    }
    return false;
}

// ---------------- Utility Methods ----------------

bool ArcadeKeyValues::IsEmpty() const {
    return valueType == TYPE_NONE && children.empty();
}

int ArcadeKeyValues::GetChildCount() const {
    return static_cast<int>(children.size());
}

ArcadeKeyValues::ValueType ArcadeKeyValues::GetValueType() const {
    return valueType;
}

void ArcadeKeyValues::Clear() {
    children.clear();
    stringValue.clear();
    intValue = 0;
    floatValue = 0.0f;
    valueType = TYPE_NONE;
}

// ---------------- Debug Output ----------------

void ArcadeKeyValues::PrintToConsole(int depth) const {
    std::string indent(depth * 2, ' ');

    std::cout << indent << "\"" << name << "\"";

    switch (valueType) {
    case TYPE_STRING:
        std::cout << " \"" << stringValue << "\"" << std::endl;
        break;
    case TYPE_INT:
        std::cout << " " << intValue << std::endl;
        break;
    case TYPE_FLOAT:
        std::cout << " " << floatValue << std::endl;
        break;
    case TYPE_SUBSECTION:
    case TYPE_NONE:
        std::cout << std::endl << indent << "{" << std::endl;
        for (const auto& pair : children) {
            const std::unique_ptr<ArcadeKeyValues>& child = pair.second;
            child->PrintToConsole(depth + 1);
        }
        std::cout << indent << "}" << std::endl;
        break;
    }
}

// ---------------- Children Access ----------------

const std::vector<std::pair<std::string, std::unique_ptr<ArcadeKeyValues>>>&
ArcadeKeyValues::GetChildren() const {
    return children;
}

// ---------------- Serialization ----------------

std::vector<std::uint8_t> ArcadeKeyValues::SerializeToBinary() const {
    std::vector<std::uint8_t> result;
    serializeRecursive(result);
    // Add end-of-root object marker so the format matches what parseRecursive expects
    result.push_back(0x08);
    return result;
}

std::string ArcadeKeyValues::SerializeToHex() const {
    std::vector<std::uint8_t> binary = SerializeToBinary();
    std::string hexString;
    hexString.reserve(binary.size() * 2);

    for (std::uint8_t byte : binary) {
        char hexByte[3];
        //std::snprintf(hexByte, sizeof(hexByte), "%02x", byte);	// Because we're in Source...
		Q_snprintf(hexByte, sizeof(hexByte), "%02x", byte);
        hexString += hexByte;
    }

    return hexString;
}

// ---------------- Private Helpers (Parsing) ----------------

std::vector<std::uint8_t> ArcadeKeyValues::hexToBytes(const std::string& hex) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(hex.length() / 2);

    for (std::size_t i = 0; i < hex.length(); i += 2) {
        std::uint8_t byte =
            static_cast<std::uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

std::pair<std::string, std::size_t> ArcadeKeyValues::readString(
    const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    std::string result;
    std::size_t pos = offset;

    while (pos < bytes.size() && bytes[pos] != 0) {
        result += static_cast<char>(bytes[pos]);
        pos++;
    }

    return { result, pos + 1 };
}

std::unique_ptr<ArcadeKeyValues> ArcadeKeyValues::parseRecursive(
    const std::vector<std::uint8_t>& bytes,
    std::size_t& position,
    const std::string& rootName) {

    auto kv = std::make_unique<ArcadeKeyValues>(rootName);
    kv->valueType = TYPE_SUBSECTION;

    while (position < bytes.size()) {
        // Get type byte
        std::uint8_t typeByte = bytes[position++];

        // End of object marker
        if (typeByte == 0x08) {
            break;
        }

        // Read key name
        std::pair<std::string, std::size_t> keyResult = readString(bytes, position);
        std::string keyName = keyResult.first;
        std::size_t newPos = keyResult.second;
        position = newPos;

        // Empty key means end of object
        if (keyName.empty()) {
            break;
        }

        // Create child key
        auto child = std::make_unique<ArcadeKeyValues>(keyName);
        child->parent = kv.get();

        // Parse value based on type
        if (typeByte == 0x00) { // Nested object
            child = parseRecursive(bytes, position, keyName);
            child->parent = kv.get();
        } else if (typeByte == 0x01) { // String
            std::pair<std::string, std::size_t> valueResult = readString(bytes, position);
            std::string value = valueResult.first;
            std::size_t valuePos = valueResult.second;
            position = valuePos;
            child->stringValue = value;
            child->valueType = TYPE_STRING;
        } else if (typeByte == 0x02) { // Int32 (little-endian)
            if (position + 4 <= bytes.size()) {
                std::int32_t value =
                    static_cast<std::int32_t>(bytes[position]) |
                    (static_cast<std::int32_t>(bytes[position + 1]) << 8) |
                    (static_cast<std::int32_t>(bytes[position + 2]) << 16) |
                    (static_cast<std::int32_t>(bytes[position + 3]) << 24);
                child->intValue = value;
                child->valueType = TYPE_INT;
                position += 4;
            } else {
                break;
            }
        } else if (typeByte == 0x03) { // Float32 (little-endian)
            if (position + 4 <= bytes.size()) {
                // Read 4 bytes as little-endian and interpret as float
                std::uint32_t intBits =
                    static_cast<std::uint32_t>(bytes[position]) |
                    (static_cast<std::uint32_t>(bytes[position + 1]) << 8) |
                    (static_cast<std::uint32_t>(bytes[position + 2]) << 16) |
                    (static_cast<std::uint32_t>(bytes[position + 3]) << 24);
                float value;
                std::memcpy(&value, &intBits, sizeof(float));
                child->floatValue = value;
                child->valueType = TYPE_FLOAT;
                position += 4;
            } else {
                break;
            }
        } else {
            std::cerr << "Unknown type byte: 0x"
                      << std::hex << static_cast<int>(typeByte) << std::endl;
            break;
        }

        // Set childIndex before adding to parent's children vector
        child->childIndex = static_cast<int>(kv->children.size());
        kv->children.push_back({ keyName, std::move(child) });
    }

    return kv;
}

// ---------------- Private Helper (Serialization) ----------------

void ArcadeKeyValues::serializeRecursive(std::vector<std::uint8_t>& buffer) const {
    // Serialize all children in order
    for (const auto& pair : children) {
        const std::string& childName = pair.first;
        const std::unique_ptr<ArcadeKeyValues>& child = pair.second;

        // Determine if this is a subsection (has children OR explicitly marked as subsection)
        bool isSubsection =
            (child->valueType == TYPE_SUBSECTION || child->GetChildCount() > 0);

        // Skip empty strings to avoid cluttering the binary data
        if (child->valueType == TYPE_STRING && child->stringValue.empty()) {
            continue;
        }

        // Skip empty subsections (no children and TYPE_SUBSECTION or TYPE_NONE)
        if (isSubsection && child->GetChildCount() == 0) {
            continue;
        }

        // Write type byte based on actual value type
        if (isSubsection) {
            buffer.push_back(0x00); // Nested object
        } else if (child->valueType == TYPE_STRING) {
            buffer.push_back(0x01); // String
        } else if (child->valueType == TYPE_INT) {
            buffer.push_back(0x02); // Int32
        } else if (child->valueType == TYPE_FLOAT) {
            buffer.push_back(0x03); // Float32
        } else {
            continue; // Skip TYPE_NONE or unknown types
        }

        // Write key name
        for (char c : childName) {
            buffer.push_back(static_cast<std::uint8_t>(c));
        }
        buffer.push_back(0x00); // Null terminator

        // Write value based on type
        if (isSubsection) {
            // Recursively serialize nested object
            child->serializeRecursive(buffer);
            buffer.push_back(0x08); // End of object marker
        } else if (child->valueType == TYPE_STRING) {
            // Write string value
            for (char c : child->stringValue) {
                buffer.push_back(static_cast<std::uint8_t>(c));
            }
            buffer.push_back(0x00); // Null terminator
        } else if (child->valueType == TYPE_INT) {
            // Write int32 value (little-endian)
            std::int32_t value = child->intValue;
            buffer.push_back(static_cast<std::uint8_t>(value & 0xFF));
            buffer.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
            buffer.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
            buffer.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFF));
        } else if (child->valueType == TYPE_FLOAT) {
            // Write float32 value (little-endian)
            float value = child->floatValue;
            std::uint32_t intBits;
            std::memcpy(&intBits, &value, sizeof(float));
            buffer.push_back(static_cast<std::uint8_t>(intBits & 0xFF));
            buffer.push_back(static_cast<std::uint8_t>((intBits >> 8) & 0xFF));
            buffer.push_back(static_cast<std::uint8_t>((intBits >> 16) & 0xFF));
            buffer.push_back(static_cast<std::uint8_t>((intBits >> 24) & 0xFF));
        }
    }
}
