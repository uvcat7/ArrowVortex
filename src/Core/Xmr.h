#pragma once

#include <Core/Core.h>
#include <filesystem>
namespace fs = std::filesystem;

namespace Vortex {

/// For-loop macro to iterate over node attributes.
/// it: name of iterator, node: pointer to root node.
#define ForXmrAttribs(it, node)                                   \
    for (XmrAttrib* it = (node) ? (node)->attrib() : nullptr; it; \
         it = it->next())

/// For-loop macro to iterate over node attributes with the given name.
/// it: name of iterator, node: pointer to root node, name: name of attributes.
#define ForXmrAttribsNamed(it, node, name)                            \
    for (XmrAttrib* it = (node) ? (node)->attrib(name) : nullptr; it; \
         it = it->next(name))

/// For-loop macro to iterate over node children.
/// it: name of iterator, node: pointer to root node.
#define ForXmrNodes(it, node) \
    for (XmrNode* it = (node) ? (node)->child() : nullptr; it; it = it->next())

/// For-loop macro to iterate over node children with the given name.
/// it: name of iterator, node: pointer to root node, name: name of children.
#define ForXmrNodesNamed(it, node, name)                           \
    for (XmrNode* it = (node) ? (node)->child(name) : nullptr; it; \
         it = it->next(name))

/// Enumeration of results that can be returned by load/save functions.
enum XmrResult {
    XMR_SUCCESS = 0,              ///< The XMR was loaded/saved succesfully.
    XMR_COULD_NOT_OPEN_FILE,      ///< The file could not be opened for
                                  ///< reading/writing.
    XMR_EXPECTED_NODE_OR_ATTRIB,  ///< A '{' or '=' was expected, but did not
                                  ///< occur.
    XMR_NODE_WITHOUT_NAME,        ///< A node is declared without name.
    XMR_ATTRIB_WITHOUT_NAME,      ///< An attribute is declared without name.
    XMR_RUNAWAY_STRING,  ///< A single/double quoted String is never terminated.
    XMR_RUNAWAY_NODE,    ///< A node is never terminated.
};

// Quote settings, used when saving XMR documents.
enum XmrQuoteSetting {
    XMR_QUOTE_ALWAYS,     ///< Strings are always quoted.
    XMR_QUOTE_WHEN_TEXT,  ///< Strings that are not formatted as numbers are
                          ///< quoted.
    XMR_QUOTE_WHEN_NECESSARY,  ///< Strings that have invalid name characters
                               ///< are quoted.
};

// Save settings, used when saving XMR documents.
struct XmrSaveSettings {
    XmrSaveSettings();
    const char* headerComment;    ///< Default is nullptr.
    bool useTabsInsteadOfSpaces;  ///< Default is false.
    XmrQuoteSetting quoteNodes;   ///< Default is XMR_QUOTE_WHEN_NECESSARY.
    XmrQuoteSetting quoteNames;   ///< Default is XMR_QUOTE_WHEN_NECESSARY.
    XmrQuoteSetting quoteValues;  ///< Default is XMR_QUOTE_WHEN_NECESSARY.
    int spacesPerIndent;          ///< Default is 2.
};

/// XmrAttrib contains the name and value of an XMR attribute.
struct XmrAttrib {
    /// Returns the next attribute, or null if there is none.
    XmrAttrib* next() const;

    /// Returns the next attribute matching name, or null if there is none.
    XmrAttrib* next(const char* name) const;

    /// The name of the attribute.
    char* name;

    /// The values of the attribute.
    char** values;

    /// The number of values stored in the attribute.
    int numValues;

    /// Link to the next attribute of the node.
    XmrAttrib* nextPtr;
};

/// XmrNode contains attributes and child notes of an XMR document.
struct XmrNode {
    /// Remove and destroy all attributes/child nodes.
    void clear();

    /// Returns the first attribute of the node, or null if there is none.
    XmrAttrib* attrib() const;

    /// Returns the first attribute matching 'name', or null if there is none.
    XmrAttrib* attrib(const char* name) const;

    /// Returns the first child, or null if there is none.
    XmrNode* child() const;

    /// Returns the first child matching 'name', or null if there is none.
    XmrNode* child(const char* name) const;

    /// Returns the next sibling, or null if there is none.
    XmrNode* next() const;

    /// Returns the next sibling matching 'name', or null if there is none.
    XmrNode* next(const char* name) const;

    /// Adds a child to the node.
    XmrNode* addChild(const char* name);

    /// Adds an attribute to the node.
    /// @{
    XmrAttrib* addAttrib(const char* name, const char** values, int numValues);
    XmrAttrib* addAttrib(const char* name, const char* value);
    XmrAttrib* addAttrib(const char* name, const double* values, int numValues);
    XmrAttrib* addAttrib(const char* name, double value);
    XmrAttrib* addAttrib(const char* name, const long* values, int numValues);
    XmrAttrib* addAttrib(const char* name, long value);
    XmrAttrib* addAttrib(const char* name, const bool* values, int numValues);
    XmrAttrib* addAttrib(const char* name, bool value);
    /// @}

    /// Removes and destroys a child node.
    void removeChild(XmrNode* node);

    /// Removes and destroys a single attribute.
    void removeAttrib(XmrAttrib* attrib);

    /// Returns the number of values of an attribute.
    int getNumValues(const char* name) const;

    /// Reads an attribute string and returns it.
    /// If the attribute is not found, the 'alt' argument is returned.
    const char* get(const char* name, const char* alt = nullptr) const;

    /// Reads an attribute value and writes it to 'v'.
    /// If the attribute is not found, false is returned and 'v' is unchanged.
    /// @{
    bool get(const char* name, int* v) const;
    bool get(const char* name, bool* v) const;
    bool get(const char* name, float* v) const;
    bool get(const char* name, double* v) const;
    /// @}

    /// Reads an attribute value and returns it.
    /// If the attribute is not found, the 'alt' argument is returned.
    /// @{
    int get(const char* name, int alt) const;
    bool get(const char* name, bool alt) const;
    float get(const char* name, float alt) const;
    double get(const char* name, double alt) const;
    /// @}

    /// Reads a list of 'n' attribute values and writes them to 'v'.
    /// Returns the number of values written to 'v'.
    /// @{
    int get(const char* name, int* v, int n) const;
    int get(const char* name, bool* v, int n) const;
    int get(const char* name, float* v, int n) const;
    int get(const char* name, double* v, int n) const;
    int get(const char* name, char** v, int n) const;  // signature not found?
    /// @}

    /// The name of the node.
    const char* name;

    /// The first attribute of the node.
    XmrAttrib* attribPtr;

    /// The first child of the node.
    XmrNode* childPtr;

    /// The next sibling of the node.
    XmrNode* nextPtr;
};

/// XmrDoc is used to load XMR documents.
struct XmrDoc : public XmrNode {
    ~XmrDoc();

    /// Constructs an empty document.
    XmrDoc();

    /// Loads an XMR document from an XMR file.
    XmrResult loadFile(fs::path path);

    /// Loads an XMR document from a string of text.
    XmrResult loadString(const char* str);

    /// Writes the XMR document to an XML file.
    XmrResult saveFile(const char* path, XmrSaveSettings settings);

    /// Writes the XMR document to a string and returns it.
    std::string saveString(XmrSaveSettings settings);

    /// Description of the last occured error.
    const char* lastError;
};

};  // namespace Vortex
