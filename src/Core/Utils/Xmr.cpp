#include <Precomp.h>

#include <Core/Utils/Xmr.h>

#include <Core/Utils/String.h>
#include <Core/Utils/Ascii.h>

#include <Core/System/File.h>
#include <Core/System/Debug.h>

namespace AV {

using namespace std;

// =====================================================================================================================
// Utility functions.

template <uint64_t offset, char... c>
constexpr uint64_t CharBitmask() { return ((1ull << (uint64_t(c) - offset)) | ...); }

static const char* NoErrorString = "no error";
static const char* EndOfBufferString = "";

static constexpr uint64_t NameCharMaskLo = ~CharBitmask<0, '\0', '\n', ';', '='>();
static constexpr uint64_t NameCharMaskHi = ~CharBitmask<64, '{', '}'>();

// Matches all characters except '\0','\n', ';', '=', '{', '}'.
static constexpr bool IsNameChar(char c)
{
	if (c < 64) return (NameCharMaskLo >> c) & 1;
	if (c < 128) return (NameCharMaskHi >> (c - 64)) & 1;
	return true;
}

// Matches strings of digits with an optional sign and decimal point.
static bool IsBasicNumber(const char* str)
{
	if (*str == '-' || *str == '+') ++str;
	while (*str >= '0' && *str <= '9') ++str;
	if (*str == '.') ++str;
	while (*str >= '0' && *str <= '9') ++str;
	return *str == 0;
}

// Skips space characters and comments.
static const char* SkipSpace(const char* p)
{
	while (true)
	{
		if (Ascii::isSpace(*p))
		{
			++p; // Skip whitespace.
		}
		else if (p[0] == '/' && p[1] == '/')
		{
			while (*p && *p != '\n') ++p; // Skip comments.
		}
		else break;
	}
	return p;
}

// Skips space characters (except newlines) and comments.
static const char* SkipSpaceExceptNewline(const char* p)
{
	while (true)
	{
		if (*p == ' ' || *p == '\t' || *p == '\r')
		{
			++p; // Skip whitespace.
		}
		else if (p[0] == '/' && p[1] == '/')
		{
			while (*p && *p != '\n') ++p; // Skip comments.
		}
		else break;
	}
	return p;
}

// =====================================================================================================================
// Iterators

XmrNodeIter& XmrNodeIter::operator++()
{
	n = n->next(name);
	return *this;
}

// =====================================================================================================================
// XmrReader

class XmrReader
{
public:
	XmrReader();
	~XmrReader();

	XmrResult load(const char* buffer, XmrDoc& doc);

	const char* error(const char* p, XmrResult code, char c);
	const char* error(const char* p, XmrResult code, const char* description);
	const char* parse(const char* p, XmrNode* n);
	const char* parseString(string& out, const char* p);

private:
	XmrResult myResult;
	string myBuffer;
	string myErrorStr;
	const char* myErrorPos;
};

XmrReader::XmrReader()
	: myResult(XmrResult::Success)
	, myErrorPos(nullptr)
	, myBuffer()
{
	myBuffer.reserve(128);
}

XmrReader::~XmrReader()
{
}

XmrResult XmrReader::load(const char* buffer, XmrDoc& doc)
{
	// Parse all attributes/nodes in the document root.
	const char* p = SkipSpace(buffer);
	while (*p) p = parse(p, &doc);

	// If an error occured, report it.
	if (myResult != XmrResult::Success)
	{
		// Find the error line and position.
		int line = 1, pos = 1;
		for (p = buffer; *p && p < myErrorPos; ++p)
		{
			if (*p == '\n')
				++line, pos = 1; // newline character.
			else if ((uchar)(*p) < 128)
				++pos; // single-byte character.
			else if (((uchar)(*p) & 192) == 192)
				++pos; // first byte of multi-byte character.
		}

		// Construct the error string.
		if (*p)
		{
			string location = "line ";
			String::appendInt(location, line);
			location.append(", pos ");
			String::appendInt(location, pos);
			location.append(": ");
			myErrorStr.insert(0, location);
		}

		doc.lastError = myErrorStr;
	}

	return myResult;
}

const char* XmrReader::error(const char* p, XmrResult code, char c)
{
	if (myResult == XmrResult::Success) // Only store the first error.
	{
		myErrorStr = format("invalid char '{}'", c);
		myErrorPos = p;
		myResult = code;
	}
	return EndOfBufferString;
}

const char* XmrReader::error(const char* p, XmrResult code, const char* description)
{
	if (myResult == XmrResult::Success) // Only store the first error.
	{
		myErrorStr = description;
		myErrorPos = p;
		myResult = code;
	}
	return EndOfBufferString;
}

const char* XmrReader::parseString(string& out, const char* p)
{
	// Skip leading whitespace.
	p = SkipSpaceExceptNewline(p);

	// Check if the string is enclosed in double quotes.
	if (*p == '"')
	{
		bool escapeNextChar = false;
		const char* q = p + 1;
		while (*q && (*q != *p || escapeNextChar))
		{
			char c = *q++;
			if (escapeNextChar) // Escape sequence, second char.
			{
				switch (c)
				{
					case 'n': c = '\n'; break;
					case 'r': c = '\r'; break;
					case 't': c = '\t'; break;
				}
				escapeNextChar = false;
			}
			else if (c == '\\') // Escape sequence, first char.
			{
				escapeNextChar = true;
				continue;
			}
			out.push_back(c);
		}
		if (*q != *p) return error(p, XmrResult::RunawayString, "runaway string");
		p = q + 1; // skip end qoutes.
	}
	// Check if the first character is a name character.
	else if (IsNameChar(*p))
	{
		const char* q = p + 1;
		while (*q && IsNameChar(*q)) ++q;
		while (p != q && Ascii::isSpace(q[-1])) --q; // Remove trailing whitespace.
		out.append(p, q - p);
		p = q;
	}

	return p;
}

const char* XmrReader::parse(const char* p, XmrNode* pn)
{
	auto child = pn->addChild();

	// Read the name of the node.
	auto nameEnd = parseString(child->name, p);
	p = SkipSpaceExceptNewline(nameEnd);
	if (child->name.empty())
		return error(p, XmrResult::MissingName, "node without name");

	// Check if the node has a value.
	if (*p == '=')
		p = SkipSpaceExceptNewline(parseString(child->value, p + 1)); // Parse value.

	// Check if the node has children.
	if (*p == '{')
	{
		auto q = SkipSpace(p + 1); // Skip '{'.
		while (*q && *q != '}') q = parse(q, child); // Parse child content.
		if (*q == 0) return error(p, XmrResult::RunawayNode, "runaway node");
		p = q + 1; // Skip '}'.
	}

	if (*p == ';') ++p;
	return SkipSpace(p);
}

// =====================================================================================================================
// XmrWriter

static void WriteString(const XmrSaveSettings& settings, XmrQuoteSetting qs, string& out, stringref str)
{
	const char* s = str.data();
	char escStr[6] = {'\n', '\r', '\t', '\\', '"', 0};
	char repStr[6] = {'n', 'r', 't', '\\', '"', 0};
	bool useQuotes = true;
	if (qs == XmrQuoteSetting::NonNumbers)
	{
		useQuotes = !IsBasicNumber(s);
	}
	else if (qs == XmrQuoteSetting::WhenNecessary)
	{
		const char* p = s;
		while (*p && IsNameChar(*p)) ++p;
		useQuotes = (p > s) && (*p || Ascii::isSpace(*s) || Ascii::isSpace(p[-1]));
	}
	if (useQuotes)
	{
		out.push_back('"');
		for (; *s; ++s)
		{
			char* esc = strchr(escStr, *s);
			if (esc)
			{
				out.push_back('\\');
				out.push_back(repStr[esc - escStr]);
			}
			else out.push_back(*s);
		}
		out.push_back('"');
	}
	else
	{
		out.append(s);
	}
}

static void WriteNode(const XmrSaveSettings& settings, string& out, XmrNode* node, int indentLevel)
{
	char indentChr = settings.useTabsInsteadOfSpaces ? '\t' : ' ';
	string indent = string(indentLevel, indentChr);

	// Write all child nodes.
	for (auto n : node->children())
	{
		out.append(indent);
		WriteString(settings, settings.quoteNodes, out, n->name);
		if (n->value.size())
		{
			out.append(" = ");
			WriteString(settings, settings.quoteValues, out, n->value);
		}
		auto child = n->firstChild.get();
		if (child)
		{
			out.append(" {\n");
			WriteNode(settings, out, n, indentLevel + settings.spacesPerIndent);
			out.append(indent);
			out.append("}");
		}
		out.push_back('\n');
	}
}

static void WriteComment(string& out, const char* str)
{
	while (*str)
	{
		const char* begin = str;
		while (*str && *str != '\n') ++str;
		if (*str == '\n') ++str;
		const char* end = str;
		if (end != begin)
		{
			out.append("// ");
			out.append(begin, end - begin);
		}
	}
	out.append("\n\n");
}

// =====================================================================================================================
// XmrSaveSettings

XmrSaveSettings::XmrSaveSettings()
	: useTabsInsteadOfSpaces(false)
	, quoteNodes(XmrQuoteSetting::WhenNecessary)
	, quoteNames(XmrQuoteSetting::WhenNecessary)
	, quoteValues(XmrQuoteSetting::WhenNecessary)
	, spacesPerIndent(2)
{
}

// =====================================================================================================================
// XmrNode

XmrNode* XmrNode::addChild()
{
	auto n = &firstChild;
	while (*n) n = &(*n)->nextSibling;
	n->reset(new XmrNode);
	return n->get();
}

XmrNode* XmrNode::addChild(const char* name)
{
	auto n = addChild();
	n->name = name;
	return n;
}

XmrNode* XmrNode::addChild(const char* name, std::string value)
{
	auto n = addChild(name);
	n->value.swap(value);
	return n;
}

XmrNode* XmrNode::getOrAddChild(const char* name)
{
	XmrNode* n = child(name);
	if (!n) n = addChild(name);
	return n;
}

XmrNode* XmrNode::child() const
{
	return firstChild.get();
}

XmrNode* XmrNode::child(const char* name) const
{
	for (auto n = firstChild.get(); n; n = n->nextSibling.get()) {
		if (name == nullptr || n->name == name) return n;
	}
	return nullptr;
}

XmrNodeRange XmrNode::children() const
{
	return { firstChild.get() };
}

XmrNodeRange XmrNode::children(const char* name) const
{
	return { child(name), name };
}

XmrNode* XmrNode::next() const
{
	return nextSibling.get();
}

XmrNode* XmrNode::next(const char* name) const
{
	for (auto n = nextSibling.get(); n; n = n->nextSibling.get()) {
		if (name == nullptr || n->name == name) return n;
	}
	return nullptr;
}

const char* XmrNode::get(const char* name, const char* alt) const
{
	auto n = child(name);
	return n ? n->value.data() : alt;
}

// =====================================================================================================================
// Amount retrieval functions.

bool XmrNode::get(const char* name, int* v) const
{
	auto n = child(name);
	return n ? String::readInt(n->value, *v) : false;
}

bool XmrNode::get(const char* name, bool* v) const
{
	auto n = child(name);
	return n ? String::readBool(n->value, *v) : false;
}

bool XmrNode::get(const char* name, float* v) const
{
	auto n = child(name);
	return n ? String::readFloat(n->value, *v) : false;
}

bool XmrNode::get(const char* name, double* v) const
{
	auto n = child(name);
	return n ? String::readDouble(n->value, *v) : false;
}

int XmrNode::get(const char* name, int alt) const
{
	auto n = child(name);
	return n ? String::toInt(n->value, alt) : alt;
}

bool XmrNode::get(const char* name, bool alt) const
{
	auto n = child(name);
	return n ? String::toBool(n->value, alt) : alt;
}

float XmrNode::get(const char* name, float alt) const
{
	auto n = child(name);
	return n ? String::toFloat(n->value, alt) : alt;
}

double XmrNode::get(const char* name, double alt) const
{
	auto n = child(name);
	return n ? String::toDouble(n->value, alt) : alt;
}

// =====================================================================================================================
// XmrDoc

XmrDoc::XmrDoc()
	: lastError(NoErrorString)
{
	name = "root";
}

XmrResult XmrDoc::loadFile(const FilePath& path)
{
	firstChild.reset();
	lastError = NoErrorString;

	// Open the XMR file.
	FileReader file;
	if (!file.open(path))
	{
		lastError = "could not open file";
		return XmrResult::CouldNotOpenFile;
	}

	// Read the file contents into a buffer.
	size_t size = file.size();
	char* buffer = new char[size + 1];
	size = file.read(buffer, 1, size);
	buffer[size] = 0;
	file.close();

	// Parse the file as a string of text.
	XmrResult result = loadString(buffer);
	delete[] buffer;
	return result;
}

XmrResult XmrDoc::loadString(const char* str)
{
	firstChild.reset();
	lastError = NoErrorString;

	// Skip BOM characters if present.
	const uchar bom[] = {0xEF, 0xBB, 0xBF};
	if (memcmp(str, bom, 3) == 0) str += 3;

	// Parse the document.
	XmrReader reader;
	return reader.load(str, *this);
}

XmrResult XmrDoc::saveFile(const FilePath& path, XmrSaveSettings settings)
{
	lastError = NoErrorString;

	// Open the output file.
	FileWriter file;
	if (!file.open(path))
	{
		lastError = "could not open file";
		return XmrResult::CouldNotOpenFile;
	}

	// Write the string to the output file.
	string str = saveString(settings);
	file.write(str.data(), 1, str.length());

	return XmrResult::Success;
}

string XmrDoc::saveString(XmrSaveSettings settings)
{
	lastError = NoErrorString;

	// Insert the header comment.
	string str;
	if (settings.headerComment.length() > 0)
		WriteComment(str, settings.headerComment.data());

	// Write the node tree to the string.
	WriteNode(settings, str, this, 0);

	// Remove the last newline, it's not necessary.
	if (str.length() > 0 && str.back() == '\n') str.pop_back();

	return str;
}

} // namespace AV