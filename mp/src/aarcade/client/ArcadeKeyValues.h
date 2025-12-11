#ifndef ARCADE_KEYVALUES_H
#define ARCADE_KEYVALUES_H

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// Added for KeyValues string table fix
struct InstanceRecord
{
	int legacy = 0;
	std::string mapId;
	std::string title;
	std::string file;
	std::string workshopIds;
	std::string mountIds;
	std::string autoplayId;
	std::string style;
};

class ArcadeKeyValues {
public:
	enum ValueType {
		TYPE_NONE = 0,
		TYPE_STRING = 1,
		TYPE_INT = 2,
		TYPE_FLOAT = 3,
		TYPE_SUBSECTION = 4
	};

private:
	std::string name;
	std::string stringValue;
	int intValue;
	float floatValue;
	std::vector<std::pair<std::string, std::unique_ptr<ArcadeKeyValues>>> children;
	ArcadeKeyValues* parent;
	int childIndex; // Index of this node in parent's children vector
	ValueType valueType;

	// Helper methods for parsing / serialization
	static std::vector<std::uint8_t> hexToBytes(const std::string& hex);
	static std::pair<std::string, std::size_t> readString(const std::vector<std::uint8_t>& bytes, std::size_t offset);
	static std::unique_ptr<ArcadeKeyValues> parseRecursive(const std::vector<std::uint8_t>& bytes,
		std::size_t& position,
		const std::string& rootName);
	void serializeRecursive(std::vector<std::uint8_t>& buffer) const;

public:
	// Constructors / destructor
	explicit ArcadeKeyValues(const std::string& keyName = "");
	~ArcadeKeyValues();

	// Static factory method to parse from hex data
	static std::unique_ptr<ArcadeKeyValues> ParseFromHex(const std::string& hexData);

	// Core accessor methods (Valve-style API)
	const char* GetName() const;

	const char* GetString(const char* keyName = nullptr,
		const char* defaultValue = "") const;

	int GetInt(const char* keyName = nullptr,
		int defaultValue = 0) const;

	float GetFloat(const char* keyName = nullptr,
		float defaultValue = 0.0f) const;

	bool GetBool(const char* keyName = nullptr,
		bool defaultValue = false) const;

	// Subsection access
	ArcadeKeyValues* FindKey(const char* keyName,
		bool createIfNotFound = false);

	const ArcadeKeyValues* FindKey(const char* keyName) const;

	// Iteration support
	ArcadeKeyValues* GetFirstSubKey() const;
	ArcadeKeyValues* GetNextKey() const;

	// Value setting methods
	void SetString(const char* keyName, const char* value);
	void SetInt(const char* keyName, int value);
	void SetFloat(const char* keyName, float value);
	void SetBool(const char* keyName, bool value);

	// Remove a child key by name
	bool RemoveKey(const char* keyName);

	// Utility methods
	bool IsEmpty() const;
	int GetChildCount() const;
	ValueType GetValueType() const;
	void Clear();

	// Debug output
	void PrintToConsole(int depth = 0) const;

	// Get the children vector for iteration (needed for JS conversion)
	const std::vector<std::pair<std::string, std::unique_ptr<ArcadeKeyValues>>>&
		GetChildren() const;

	// Serialize to binary format
	std::vector<std::uint8_t> SerializeToBinary() const;

	// Convert to hex string
	std::string SerializeToHex() const;
};

#endif // ARCADE_KEYVALUES_H
