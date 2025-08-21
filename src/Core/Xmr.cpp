#include <Core/Xmr.h>

#include <System/File.h>
#include <System/Debug.h>
#include <Core/WideString.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <filesystem>
#include <string>

namespace Vortex {
namespace {

using namespace Vortex;

// ================================================================================================
// Basic String.

class xstring {
   public:
    ~xstring() { free(data); }
    explicit xstring(int n) : cap(n) {
        if (cap < 4) cap = 4;
        data = static_cast<char*>(malloc(cap));
    }
    void append(char c) {
        if (size == cap) {
            cap *= 2;
            data = static_cast<char*>(realloc(data, cap));
        }
        data[size++] = c;
    }
    void append(const char* str, int n) {
        int newSize = size + n;
        if (newSize > cap) {
            cap *= 2;
            if (newSize > cap) cap = newSize;
            data = static_cast<char*>(realloc(data, cap));
        }
        memcpy(data + size, str, n);
        size = newSize;
    }
    void append(const char* str) { append(str, strlen(str)); }
    void append(long v) {
        char buf[32];
        int n = sprintf_s(buf, 32, "%i", v);
        if (n < 0) n = 32;
        append(buf, n);
    }
    void append(double v) {
        char buf[32];
        int n = sprintf_s(buf, 32, "%f", v);
        if (n < 0) n = 32;
        if (memchr(buf, '.', n)) {
            while (n > 0 && buf[n - 1] == '0') --n;
            if (n > 0 && buf[n - 1] == '.') --n;
        }
        append(buf, n);
    }
    char* data = nullptr;
    int size = 0, cap;
};

// ================================================================================================
// Utility functions.

static const char* NO_ERROR = "no error";

static const char* EMPTY_ERROR_STRING = "";

// Valid chars: space, horizontal tab, carriage return, line feed.
static bool IsWhiteSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// Valid chars: all characters except '\0', '{', '}', ';', ',', '\n', '='.
static bool IsNameChar(char c) {
    static const uint32_t b[4] = {0xFFFFFBFE, 0xD7FFEFFF, 0xFFFFFFFF,
                                  0xD7FFFFFF};
    return (static_cast<unsigned char>(c) < 128)
               ? ((b[c >> 5] & (1 << (c & 31))) != 0)
               : true;
}

static bool IsNumber(const char* str) {
    if (*str == '-' || *str == '+') ++str;
    while (*str >= '0' && *str <= '9') ++str;
    if (*str == '.') ++str;
    while (*str >= '0' && *str <= '9') ++str;
    return *str == 0;
}

static const char* SkipSpace(const char* p) {
    while (true) {
        if (IsWhiteSpace(*p)) {
            ++p;  // Skip whitespace.
        } else if (p[0] == '/' && p[1] == '/') {
            while (*p && *p != '\n') ++p;  // Skip comments.
        } else
            break;
    }
    return p;
}

static const char* SkipSpaceExceptNewline(const char* p) {
    while (true) {
        if (*p == ' ' || *p == '\t' || *p == '\r') {
            ++p;  // Skip whitespace.
        } else if (p[0] == '/' && p[1] == '/') {
            while (*p && *p != '\n') ++p;  // Skip comments.
        } else
            break;
    }
    return p;
}

static bool StrEq(const char* a, const char* b) { return !strcmp(a, b); }

static char* CopyStr(const xstring* str) {
    char* dst = static_cast<char*>(malloc(str->size + 1));
    memcpy(dst, str->data, str->size);
    dst[str->size] = 0;
    return dst;
}

static void SetError(XmrDoc* doc, const xstring* str) {
    if (doc->lastError != NO_ERROR) free(const_cast<char*>(doc->lastError));
    doc->lastError = str ? CopyStr(str) : NO_ERROR;
}

static XmrAttrib* NewAttrib(const char* str, int strsize, const int* vals,
                            int numVals) {
    // Allocate the attribute object and strings in a single memory block.
    int basesize = sizeof(XmrAttrib) + sizeof(char*) * numVals;
    char* data = static_cast<char*>(malloc(basesize + strsize));

    // Copy the attribute data.
    XmrAttrib* out = reinterpret_cast<XmrAttrib*>(data);
    out->name = data + basesize;
    out->nextPtr = nullptr;
    out->numValues = numVals;
    out->values = reinterpret_cast<char**>(data + sizeof(XmrAttrib));

    memcpy(out->name, str, strsize);
    for (int i = 0; i < numVals; ++i) {
        out->values[i] = out->name + vals[i];
    }

    return out;
}

static XmrAttrib* NewAttrib(const char* name, const char* vals, int num) {
    int nameLen = strlen(name) + 1, valLen = 0;

    // Determine the total length.
    const char* pval = vals;
    for (int i = 0; i < num; ++i) {
        int len = strlen(pval) + 1;
        valLen += len;
        pval += len;
    }

    // Allocate the attribute object and strings in a single memory block.
    int basesize = sizeof(XmrAttrib) + sizeof(char*) * num;
    char* data = static_cast<char*>(malloc(basesize + nameLen + valLen));

    // Copy the attribute data.
    XmrAttrib* out = reinterpret_cast<XmrAttrib*>(data);
    out->nextPtr = nullptr;
    out->numValues = num;
    out->values = reinterpret_cast<char**>(data + sizeof(XmrAttrib));
    out->name = data + basesize;

    char* ptr = out->name;
    memcpy(ptr, name, nameLen);
    ptr += nameLen;
    pval = vals;
    for (int i = 0; i < num; ++i) {
        out->values[i] = ptr;
        int len = strlen(pval) + 1;
        memcpy(ptr, pval, len);
        ptr += len;
        pval += len;
    }

    return out;
}

static XmrAttrib* AddAttrib(XmrAttrib** head, const char* name,
                            const char* vals, int num) {
    while (*head) head = &((*head)->nextPtr);
    *head = NewAttrib(name, vals, num);
    return *head;
}

// ================================================================================================
// XmrReader

class XmrReader {
   public:
    XmrReader();
    ~XmrReader();

    XmrResult load(const char* buffer, XmrDoc& doc);

    const char* error(const char* p, XmrResult code, char c);
    const char* error(const char* p, XmrResult code, const char* description);
    const char* parse(const char* p, XmrNode* n);
    const char* parseString(const char* p);

    void addValue(int pos);

   private:
    xstring xmrErrorMessage_;
    XmrResult xmrResult_ = XMR_SUCCESS;
    const char* xmrErrorPosition_ = nullptr;
    xstring xmrTextBuffer_;
    int* xmrValueBuffer_;
    int xmrNumValues_ = 0, xmrValueCapacity_ = 16;
};

XmrReader::XmrReader()
    : xmrErrorMessage_(16),

      xmrTextBuffer_(256) {
    xmrValueBuffer_ =
        static_cast<int*>(malloc(sizeof(int) * xmrValueCapacity_));
}

XmrReader::~XmrReader() { free(xmrValueBuffer_); }

XmrResult XmrReader::load(const char* buffer, XmrDoc& doc) {
    // Parse all attributes/nodes in the document root
    const char* p = SkipSpace(buffer);
    while (*p) p = parse(p, &doc);

    // If an error occured, report it.
    if (xmrResult_ != XMR_SUCCESS) {
        // Find the error line and position.
        int line = 1, pos = 1;
        for (p = buffer; *p && p < xmrErrorPosition_; ++p) {
            if (static_cast<uint8_t>(*p) == 0xA)
                ++line, pos = 1;  // newline character.
            else if (static_cast<uint8_t>(*p) < 128)
                ++pos;  // single-byte character.
            else if ((static_cast<uint8_t>(*p) & 192) == 192)
                ++pos;  // first byte of multi-byte character.
        }

        // Construct the error xstring.
        xstring err(16);
        if (*p) {
            err.append("line ");
            err.append(static_cast<long>(line));
            err.append(", pos ");
            err.append(static_cast<long>(pos));
            err.append(": ");
        }
        err.append(xmrErrorMessage_.data, xmrErrorMessage_.size);

        SetError(&doc, &err);
    }

    return xmrResult_;
}

const char* XmrReader::error(const char* p, XmrResult code, char c) {
    if (xmrResult_ == XMR_SUCCESS)  // Only store the first error encountered.
    {
        xmrErrorMessage_.size = 0;
        xmrErrorMessage_.append("invalid char '");
        xmrErrorMessage_.append(c);
        xmrErrorMessage_.append('\'');
        xmrErrorPosition_ = p;
        xmrResult_ = code;
    }
    return EMPTY_ERROR_STRING;
}

const char* XmrReader::error(const char* p, XmrResult code,
                             const char* description) {
    if (xmrResult_ == XMR_SUCCESS)  // Only store the first error encountered.
    {
        xmrErrorMessage_.size = 0;
        xmrErrorMessage_.append(description);
        xmrErrorPosition_ = p;
        xmrResult_ = code;
    }
    return EMPTY_ERROR_STRING;
}

const char* XmrReader::parse(const char* p, XmrNode* pn) {
    // Read the name of the attribute/node.
    xmrTextBuffer_.size = 0;
    const char* nameEnd = parseString(p);
    xmrTextBuffer_.append('\0');
    p = SkipSpaceExceptNewline(nameEnd);

    // Check if we are reading an attribute.
    if (*p == '=') {
        // Check if the attribute has a name.
        if (xmrTextBuffer_.size == 1) {
            return error(p, XMR_ATTRIB_WITHOUT_NAME, "attribute without name");
        }

        // Read the first value.
        xmrNumValues_ = 0;
        addValue(xmrTextBuffer_.size);
        p = SkipSpaceExceptNewline(parseString(p + 1));
        xmrTextBuffer_.append('\0');

        // Read additional values.
        while (*p == ',') {
            addValue(xmrTextBuffer_.size);
            p = SkipSpaceExceptNewline(parseString(p + 1));
            xmrTextBuffer_.append('\0');
        }

        // Add the attribute to the node.
        auto a = &pn->attribPtr;
        while (*a) a = &((*a)->nextPtr);
        *a = NewAttrib(xmrTextBuffer_.data, xmrTextBuffer_.size,
                       xmrValueBuffer_, xmrNumValues_);

        // Skip the next character if it's a semicolon.
        if (*p == ';') ++p;
    }
    // Check if we are reading a node.
    else if (*p == '{') {
        // Check if the node has a name.
        if (xmrTextBuffer_.size == 1) {
            return error(p, XMR_NODE_WITHOUT_NAME, "node without name");
        }
        XmrNode* n = pn->addChild(xmrTextBuffer_.data);
        const char* q = SkipSpace(p + 1);  // skip '{'
        while (*q && *q != '}') q = parse(q, n);
        if (*q == 0) return error(p, XMR_RUNAWAY_NODE, "runaway node");
        p = q + 1;  // skip '}'
    } else {
        return error(nameEnd, XMR_EXPECTED_NODE_OR_ATTRIB,
                     "expected '{' or '='");
    }

    return SkipSpace(p);
}

const char* XmrReader::parseString(const char* p) {
    // Skip leading whitespace.
    p = SkipSpaceExceptNewline(p);

    // Check if the string is enclosed in double quotes.
    if (*p == '"') {
        bool escape = false;
        const char* q = p + 1;
        while (*q && (*q != *p || escape)) {
            char c = *q++;
            if (escape)  // Escape sequence, second char.
            {
                for (int i = 0; i < 3; ++i)
                    if (c == "nrt"[i]) c = "\n\r\t"[i];
                escape = false;
            } else if (c == '\\')  // Escape sequence, first char.
            {
                escape = true;
                continue;
            }
            xmrTextBuffer_.append(c);
        }
        if (*q != *p) return error(p, XMR_RUNAWAY_STRING, "runaway string");
        p = q + 1;  // skip end qoutes.
    }
    // Check if the first character is a name character.
    else if (IsNameChar(*p)) {
        const char* q = p + 1;
        while (*q && IsNameChar(*q)) ++q;
        while (p != q && IsWhiteSpace(q[-1]))
            --q;  // Remove trailing whitespace.
        xmrTextBuffer_.append(p, q - p);
        p = q;
    }

    return p;
}

void XmrReader::addValue(int pos) {
    if (xmrNumValues_ == xmrValueCapacity_) {
        xmrValueCapacity_ *= 2;
        xmrValueBuffer_ = static_cast<int*>(
            realloc(xmrValueBuffer_, sizeof(int) * xmrValueCapacity_));
    }
    xmrValueBuffer_[xmrNumValues_] = pos;
    ++xmrNumValues_;
}

// ================================================================================================
// XmrWriter

static void WriteString(const XmrSaveSettings& settings, XmrQuoteSetting qs,
                        xstring& out, const char* s) {
    char escStr[6] = {'\n', '\r', '\t', '\\', '"', 0};
    char repStr[6] = {'n', 'r', 't', '\\', '"', 0};
    bool useQuotes = true;
    if (qs == XMR_QUOTE_WHEN_TEXT) {
        useQuotes = !IsNumber(s);
    } else if (qs == XMR_QUOTE_WHEN_NECESSARY) {
        const char* p = s;
        while (*p && IsNameChar(*p)) ++p;
        useQuotes = (p > s) && (*p || IsWhiteSpace(*s) || IsWhiteSpace(p[-1]));
    }
    if (useQuotes) {
        out.append('"');
        for (; *s; ++s) {
            char* esc = strchr(escStr, *s);
            if (esc) {
                out.append('\\');
                out.append(&repStr[esc - escStr], 1);
            } else
                out.append(*s);
        }
        out.append('"');
    } else {
        out.append(s);
    }
}

static void WriteNode(const XmrSaveSettings& settings, xstring& out,
                      XmrNode* node, int indentLevel) {
    char indentChr = settings.useTabsInsteadOfSpaces ? '\t' : ' ';
    xstring indent(indentLevel);
    memset(indent.data, indentChr, indentLevel);

    // Write all attributes.
    ForXmrAttribs(a, node) {
        out.append(indent.data, indentLevel);
        WriteString(settings, settings.quoteNames, out, a->name);
        out.append(" = ");
        for (int i = 0; i < a->numValues; ++i) {
            if (i > 0) out.append(", ");
            WriteString(settings, settings.quoteValues, out, a->values[i]);
        }
        out.append('\n');
    }

    // Write all child nodes.
    ForXmrNodes(n, node) {
        out.append(indent.data, indentLevel);
        WriteString(settings, settings.quoteNodes, out, n->name);
        out.append(" {\n");
        WriteNode(settings, out, n, indentLevel + settings.spacesPerIndent);
        out.append(indent.data, indentLevel);
        out.append("}\n");
    }
}

static void WriteComment(xstring& out, const char* str) {
    while (*str) {
        const char* begin = str;
        while (*str && *str != '\n') ++str;
        if (*str == '\n') ++str;
        const char* end = str;
        if (end != begin) {
            out.append("// ");
            out.append(begin, end - begin);
        }
    }
    out.append("\n\n");
}

};  // anonymous namespace.

// =============================================================================================================
// XmrSaveSettings

XmrSaveSettings::XmrSaveSettings()
    : headerComment(nullptr),
      useTabsInsteadOfSpaces(false),
      quoteNodes(XMR_QUOTE_WHEN_NECESSARY),
      quoteNames(XMR_QUOTE_WHEN_NECESSARY),
      quoteValues(XMR_QUOTE_WHEN_NECESSARY),
      spacesPerIndent(2) {}

// =============================================================================================================
// XmrAttrib

XmrAttrib* XmrAttrib::next() const { return nextPtr; }

XmrAttrib* XmrAttrib::next(const char* name) const {
    for (XmrAttrib* a = nextPtr; a; a = a->nextPtr)
        if (StrEq(a->name, name)) return a;
    return nullptr;
}

// =============================================================================================================
// XmrNode

static XmrNode* NewNode(const char* n) {
    int alen = sizeof(XmrNode);
    int nlen = strlen(n) + 1;

    char* data = static_cast<char*>(malloc(alen + nlen));
    char* str = data + alen;
    XmrNode* out = reinterpret_cast<XmrNode*>(data);

    out->name = str;
    out->nextPtr = nullptr;
    out->attribPtr = nullptr;
    out->childPtr = nullptr;
    memcpy(str, n, nlen);

    return out;
}

void XmrNode::clear() {
    for (auto* a = attribPtr; a;) {
        auto* rem = a;
        a = a->nextPtr;
        free(rem);
    }
    for (auto* n = childPtr; n;) {
        auto* rem = n;
        n = n->nextPtr;
        rem->clear();
        free(rem);
    }
    attribPtr = nullptr;
    childPtr = nullptr;
}

XmrAttrib* XmrNode::addAttrib(const char* name, const char** values,
                              int numValues) {
    xstring str(256);
    for (int i = 0; i < numValues; ++i) {
        str.append(values[i]);
        str.append('\0');
    }
    return AddAttrib(&attribPtr, name, str.data, numValues);
}

XmrAttrib* XmrNode::addAttrib(const char* name, const char* value) {
    return AddAttrib(&attribPtr, name, value, 1);
}

XmrAttrib* XmrNode::addAttrib(const char* name, const double* values,
                              int numValues) {
    xstring str(256);
    for (int i = 0; i < numValues; ++i) {
        str.append(values[i]);
        str.append('\0');
    }
    return AddAttrib(&attribPtr, name, str.data, numValues);
}

XmrAttrib* XmrNode::addAttrib(const char* name, double value) {
    return addAttrib(name, &value, 1);
}

XmrAttrib* XmrNode::addAttrib(const char* name, const long* values,
                              int numValues) {
    xstring str(256);
    for (int i = 0; i < numValues; ++i) {
        str.append(values[i]);
        str.append('\0');
    }
    return AddAttrib(&attribPtr, name, str.data, numValues);
}

XmrAttrib* XmrNode::addAttrib(const char* name, long value) {
    return addAttrib(name, &value, 1);
}

XmrAttrib* XmrNode::addAttrib(const char* name, const bool* values,
                              int numValues) {
    xstring str(256);
    for (int i = 0; i < numValues; ++i) {
        str.append(values[i] ? "yes" : "no");
        str.append('\0');
    }
    return AddAttrib(&attribPtr, name, str.data, numValues);
}

XmrAttrib* XmrNode::addAttrib(const char* name, bool value) {
    return addAttrib(name, &value, 1);
}

XmrNode* XmrNode::addChild(const char* name) {
    auto n = &childPtr;
    while (*n) n = &((*n)->nextPtr);
    *n = NewNode(name);
    return *n;
}

void XmrNode::removeChild(XmrNode* node) {
    if (childPtr == node) {
        childPtr = node->nextPtr;
        node->clear();
        free(node);
    } else
        for (XmrNode* n = childPtr; n; n = n->nextPtr) {
            if (n->nextPtr == node) {
                n->nextPtr = node->nextPtr;
                node->clear();
                free(node);
                break;
            }
        }
}

void XmrNode::removeAttrib(XmrAttrib* attrib) {
    if (attribPtr == attrib) {
        attribPtr = attrib->nextPtr;
        free(attrib);
    } else
        for (XmrAttrib* a = attribPtr; a; a = a->nextPtr) {
            if (a->nextPtr == attrib) {
                a->nextPtr = attrib->nextPtr;
                free(attrib);
                break;
            }
        }
}

XmrAttrib* XmrNode::attrib() const { return attribPtr; }

XmrAttrib* XmrNode::attrib(const char* name) const {
    for (XmrAttrib* a = attribPtr; a; a = a->nextPtr)
        if (StrEq(a->name, name)) return a;
    return nullptr;
}

XmrNode* XmrNode::child() const { return childPtr; }

XmrNode* XmrNode::child(const char* name) const {
    for (XmrNode* n = childPtr; n; n = n->nextPtr)
        if (StrEq(n->name, name)) return n;
    return nullptr;
}

XmrNode* XmrNode::next() const { return nextPtr; }

XmrNode* XmrNode::next(const char* name) const {
    for (XmrNode* n = nextPtr; n; n = n->nextPtr)
        if (StrEq(n->name, name)) return n;
    return nullptr;
}

int XmrNode::getNumValues(const char* name) const {
    XmrAttrib* a = attrib(name);
    return a ? a->numValues : 0;
}

const char* XmrNode::get(const char* name, const char* alt) const {
    XmrAttrib* a = attrib(name);
    return (a && a->values) ? a->values[0] : alt;
}

// ================================================================================================
// String-to-type conversion functions.

static int Read(const XmrAttrib* a, bool* out, int n) {
    int read = 0;
    if (!a)
        n = 0;
    else if (n > a->numValues)
        n = a->numValues;
    for (int i = 0; i < n; ++i) {
        char* str = a->values[i];
        if (strcmp(str, "true") == 0 || strcmp(str, "yes") == 0 ||
            strcmp(str, "1") == 0) {
            out[read++] = true;
        } else if (strcmp(str, "false") == 0 || strcmp(str, "no") == 0 ||
                   strcmp(str, "0") == 0) {
            out[read++] = false;
        }
    }
    return read;
}

static int Read(const XmrAttrib* a, int* out, int n) {
    int read = 0;
    if (!a)
        n = 0;
    else if (n > a->numValues)
        n = a->numValues;
    for (int i = 0; i < n; ++i) {
        char *end, *str = a->values[i];
        int v = static_cast<int>(strtol(str, &end, 0));
        if (end != str) out[read++] = v;
    }
    return read;
}

static int Read(const XmrAttrib* a, float* out, int n) {
    int read = 0;
    if (!a)
        n = 0;
    else if (n > a->numValues)
        n = a->numValues;
    for (int i = 0; i < n; ++i) {
        char *end, *str = a->values[i];
        float v = static_cast<float>(strtod(str, &end));
        if (end != str) out[read++] = v;
    }
    return read;
}

static int Read(const XmrAttrib* a, double* out, int n) {
    int read = 0;
    if (!a)
        n = 0;
    else if (n > a->numValues)
        n = a->numValues;
    for (int i = 0; i < n; ++i) {
        char *end, *str = a->values[i];
        double v = strtod(str, &end);
        if (end != str) out[read++] = v;
    }
    return read;
}

// ================================================================================================
// Value retrieval functions.

bool XmrNode::get(const char* name, int* v) const {
    return Read(attrib(name), v, 1) == 1;
}

bool XmrNode::get(const char* name, bool* v) const {
    return Read(attrib(name), v, 1) == 1;
}

bool XmrNode::get(const char* name, float* v) const {
    return Read(attrib(name), v, 1) == 1;
}

bool XmrNode::get(const char* name, double* v) const {
    return Read(attrib(name), v, 1) == 1;
}

int XmrNode::get(const char* name, int alt) const {
    Read(attrib(name), &alt, 1);
    return alt;
}

bool XmrNode::get(const char* name, bool alt) const {
    Read(attrib(name), &alt, 1);
    return alt;
}

float XmrNode::get(const char* name, float alt) const {
    Read(attrib(name), &alt, 1);
    return alt;
}

double XmrNode::get(const char* name, double alt) const {
    Read(attrib(name), &alt, 1);
    return alt;
}

int XmrNode::get(const char* name, int* v, int n) const {
    return Read(attrib(name), v, n);
}

int XmrNode::get(const char* name, bool* v, int n) const {
    return Read(attrib(name), v, n);
}

int XmrNode::get(const char* name, float* v, int n) const {
    return Read(attrib(name), v, n);
}

int XmrNode::get(const char* name, double* v, int n) const {
    return Read(attrib(name), v, n);
}

// =============================================================================================================
// XmrDoc

XmrDoc::~XmrDoc() {
    SetError(this, nullptr);
    clear();
}

XmrDoc::XmrDoc() : lastError(NO_ERROR) {
    name = "root";
    attribPtr = nullptr;
    childPtr = nextPtr = nullptr;
}

XmrResult XmrDoc::loadFile(const char* path) {
    clear();
    SetError(this, nullptr);

    // Open the XMR file.
    std::ifstream file(Widen(path).str());
    if (file.fail()) {
        xstring err(16);
        err.append("could not open file");
        SetError(this, &err);
        return XMR_COULD_NOT_OPEN_FILE;
    }

    // Read the file contents into a buffer.
    size_t size = std::filesystem::file_size(path);
    char* buffer = static_cast<char*>(malloc(size + 1));
    file.read(buffer, size);
    size = file.gcount();
    buffer[size] = 0;
    file.close();

    // Parse the file as a string of text.
    XmrResult result = loadString(buffer);
    free(buffer);
    return result;
}

XmrResult XmrDoc::loadString(const char* str) {
    clear();
    SetError(this, nullptr);

    // Skip the BOM character if there is one.
    const uint8_t bom[] = {0xEF, 0xBB, 0xBF};
    if (memcmp(str, bom, 3) == 0) str += 3;

    // Parse the document.
    XmrReader reader;
    return reader.load(str, *this);
}

XmrResult XmrDoc::saveFile(const char* path, XmrSaveSettings settings) {
    SetError(this, nullptr);

    // Open the output file.
    std::ofstream file(path);
    if (file.fail()) {
        xstring err(16);
        err.append("could not open file");
        SetError(this, &err);
        return XMR_COULD_NOT_OPEN_FILE;
    }

    // Write the string to the output file.
    std::string str = saveString(settings);
    file.write(str.data(), str.length());

    return XMR_SUCCESS;
}

std::string XmrDoc::saveString(XmrSaveSettings settings) {
    SetError(this, nullptr);
    xstring str(1024);

    // Insert the header comment.
    if (settings.headerComment) WriteComment(str, settings.headerComment);

    // Write the node tree to the string.
    WriteNode(settings, str, this, 0);

    // Remove the last newline, it's not necessary.
    int last = str.size - 1;
    if (last >= 0 && str.data[last] == '\n') --str.size;

    return std::string(str.data, str.size);
}

};  // namespace Vortex
