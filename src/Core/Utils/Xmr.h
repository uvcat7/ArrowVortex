#pragma once

#include <Core/System/File.h>

namespace AV {

struct XmrNode;

// Enumeration of results that can be returned by load/save functions.
enum class XmrResult
{
	Success = 0,       // The XMR was loaded/saved succesfully.
	CouldNotOpenFile,  // The file could not be opened for reading/writing.
	MissingName,       // A node is declared without name.
	RunawayString,     // A single/double quoted string is never terminated.
	RunawayNode,       // A node is never terminated.
};

// Quote settings, used when saving XMR documents.
enum class XmrQuoteSetting
{
	Always,        // All strings are quoted.
	NonNumbers,    // All strings that are not formatted as numbers are quoted.
	WhenNecessary, // Only strings that have invalid name characters are quoted.
};

// Save settings, used when saving XMR documents.
struct XmrSaveSettings
{
	XmrSaveSettings();
	std::string headerComment;   // Inserted at the document start. Default is empty string.
	bool useTabsInsteadOfSpaces; // Default is false.
	XmrQuoteSetting quoteNodes;  // Default is XmrQuoteSetting::WhenNecessary.
	XmrQuoteSetting quoteNames;  // Default is XmrQuoteSetting::WhenNecessary.
	XmrQuoteSetting quoteValues; // Default is XmrQuoteSetting::WhenNecessary.
	int spacesPerIndent;         // Default is 2.
};

// Iterates through all siblings of a node.
struct XmrNodeIter
{
	XmrNode* n = nullptr;
	const char* name = nullptr;
	XmrNodeIter& operator++();
	XmrNode* operator*() const { return n; }
	bool operator!=(const XmrNodeIter& i) const { return n != i.n; }
};

// Helper for iterating nodes.
struct XmrNodeRange
{
	XmrNode* n = nullptr;
	const char* name = nullptr;
	XmrNodeIter begin() { return {n, name}; }
	XmrNodeIter end() { return {}; }
};

// XmrNode node. Can contain a value, any number of children, or neither.
struct XmrNode
{
	// Returns the first child, or null if there is none.
	XmrNode* child() const;

	// Returns the first child matching <name>, or null if there is none.
	XmrNode* child(const char* name) const;

	// Returns all children.
	XmrNodeRange children() const;

	// Returns all children with the given name.
	XmrNodeRange children(const char* name) const;

	// Returns the next sibling, or null if there is none.
	XmrNode* next() const;

	// Returns the next sibling matching <name>, or null if there is none.
	XmrNode* next(const char* name) const;

	// Adds a child to the node.
	XmrNode* addChild();

	// Adds a child to the node.
	XmrNode* addChild(const char* name);

	// Adds a child to the node with the given value.
	XmrNode* addChild(const char* name, std::string value);

	// Returns the first child matching <name>, or adds it if there is none.
	XmrNode* getOrAddChild(const char* name);

	// Reads a child value and returns it.
	// If the child is not found, <alt> is returned.
	const char* get(const char* name, const char* alt = nullptr) const;

	// Reads a child value and writes it to <v>.
	// If the child is not found, false is returned and <v> is unchanged.
	bool get(const char* name, int* v) const;
	bool get(const char* name, bool* v) const;
	bool get(const char* name, float* v) const;
	bool get(const char* name, double* v) const;

	// Reads a child value and returns it.
	// If the child is not found, <alt> is returned.
	int get(const char* name, int alt) const;
	bool get(const char* name, bool alt) const;
	float get(const char* name, float alt) const;
	double get(const char* name, double alt) const;

	std::string name;
	std::string value;
	unique_ptr<XmrNode> firstChild;
	unique_ptr<XmrNode> nextSibling;
};

// XmrDoc is used to load XMR documents.
struct XmrDoc : public XmrNode
{
	// Constructs an empty document.
	XmrDoc();

	// Loads an XMR document from an XMR file.
	XmrResult loadFile(const FilePath& path);

	// Loads an XMR document from a string of text.
	XmrResult loadString(const char* str);

	// Writes the XMR document to an XML file.
	XmrResult saveFile(const FilePath& path, XmrSaveSettings settings);

	// Writes the XMR document to a string and returns it.
	std::string saveString(XmrSaveSettings settings);

	// Description of the last occured error.
	std::string lastError;
};

} // namespace AV