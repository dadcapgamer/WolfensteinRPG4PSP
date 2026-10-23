#include <stdexcept>
#include <cstring>

#include "CAppContainer.h"
#include "App.h"
#include "Text.h"
#include "JavaStream.h"
#include "Resource.h"
#include "MenuStrings.h"
#include "Entity.h"

// --------------------
// Localization Class
// --------------------
constexpr char* Localization::STRINGS_MENUS[4][49];

#ifdef WOLFENRPG_PSP
static const char* getPSPTextOverride(int16_t file, int16_t id) {
	if (file == Strings::FILE_CODESTRINGS) {
		switch (id) {
		case CodeStrings::GENHELP_NPC:
			return "TALK|Press X to talk to characters with chat bubbles.";
		case CodeStrings::GENHELP_JOURNAL:
			return "NOTE|Press SELECT for Items/Info, then choose Journal.";
		case CodeStrings::GENHELP_DOORS:
			return "DOOR|Face a door and press X to open it. Some doors require a key.";
		case CodeStrings::GENHELP_WEAPONS:
			return "WEAPON SELECTION|Press L or R to cycle weapons. You can also choose a weapon from Inventory.";
		case CodeStrings::SYRINGE_MENU_HELP:
			return "Open the menu with START or CIRCLE, then choose Inventory and Syringes.";
		case CodeStrings::OTHER_MENU_HELP:
			return "Open the menu with START or CIRCLE, then choose Inventory and Other.";
		case CodeStrings::JOURNAL_MENU_HELP:
			return "Press SELECT for Items/Info, or open the menu and choose Journal.";
		case CodeStrings::HEALTH_MENU_HELP:
			return "Press SELECT for Items/Info, or open the menu and choose Inventory.";
		case CodeStrings::WEAPON_MENU_HELP:
			return "Press L or R to change weapons. Open the menu and choose Inventory to select one directly.";
		case CodeStrings::SKIP:
			return "X Skip";
		case CodeStrings::PRESS_OK:
			return "Press X to continue";
		case CodeStrings::HEALTH_RESTORED:
			return "Health restored.|To see your stats, open the menu with START or CIRCLE and choose Status.";
		case CodeStrings::PRESS_OK_TO_MIX:
			return "Press X to mix";
		case CodeStrings::PRESS_OK_TO_ADD:
			return "Press X to add";
		case CodeStrings::PRESS_OK_TO_EXIT:
			return "Press X to continue";
		case CodeStrings::GO_TO_WAR:
			return "Cards are equal.|Go to WAR!|Press X.";
		default:
			break;
		}
	}
	else if (file == Strings::FILE_FILESTRINGS) {
		switch (id) {
		case FileStrings::HELP_GENERAL:
			return "#HOW TO PLAY|Wolfenstein RPG is turn based: each movement or action uses one turn. Use the D-pad to move and turn, or the analog stick to move and strafe. Press X to attack, talk, use, or select.||#OTHER|SQUARE passes a turn.|TRIANGLE opens the automap.|START or CIRCLE opens the menu or goes back.|L and R change weapons.|SELECT opens Items/Info.";
		case FileStrings::HELP_MOVE:
			return "#HOW TO MOVE|D-pad Up/Down moves forward and backward.|D-pad Left/Right turns.|The analog stick moves forward/back and strafes left/right.||Movement uses one turn.";
		case FileStrings::HELP_ATTACK:
			return "#HOW TO ATTACK|Press X to attack, talk, or use.|Press L or R to change weapons.|Press SQUARE to pass a turn.||Attacking uses one turn.";
		case FileStrings::HELP_SNIPER:
			return "#SNIPER RIFLES|Press X with a sniper rifle equipped to enter sniper mode.|Use the D-pad or analog stick to aim.|Use L and R to zoom.|Press X to fire.|Press CIRCLE or START to exit sniper mode.";
		case FileStrings::HELP_KICKING:
			return "#CHICKEN KICKING|Press X to start. Press X again to set direction, then press X once more to set distance.";
		default:
			break;
		}
	}
	else if (file == Strings::FILE_MENUSTRINGS) {
		switch (id) {
		case MenuStrings::PRESS_OK_TO_LABEL:
			return "Press X to";
		case MenuStrings::BOOKS_ITEM_HELP:
			return "Press X to read this book.";
		case MenuStrings::TOUCH_CONTROLS_ITEM:
		case MenuStrings::TOUCH_CONTROLS_TITLE:
			return "PSP Controls";
		case MenuStrings::TOUCH_BUTTON_TRANSPARENCY_ITEM:
			return "Control Overlay";
		default:
			break;
		}
	}
	else if (file == Strings::FILE_M_01) {
		switch (id) {
		case 4:
			return "REMINDER|You can save at any time. Press START or CIRCLE, then choose Save Game.";
		case 5:
			return "HINT|Press SQUARE to pass a turn and stay in place.";
		case 9:
			return "HINT|Use the analog stick left or right to strafe.";
		case 10:
			return "HELP|PSP controls are listed under Help in the pause menu.";
		case 11:
			return "HELP|Press X to shoot, punch, kick, talk, or use. Fighting is turn based, so enemies act after your turn.";
		default:
			break;
		}
	}

	return nullptr;
}
#endif

Localization::Localization() {
	memset(this, 0, sizeof(Localization));
}

Localization::~Localization() {
}

bool Localization::startup() {
	//printf("Localization::startup\n");
	this->text = new char* [Strings::FILE_MAX]();
	this->textMap =  new uint16_t* [Strings::FILE_MAX]();

	/*for (int i = 0; i < (Strings::FILE_MAX); ++i) {
		this->text[i] = nullptr;
		this->textMap[i] = nullptr;
	}*/

	this->dynamicArgs = new Text(4096);
	this->scratchBuffers[0] = new Text(2048);
	this->scratchBuffers[1] = new Text(2048);
	this->scratchBuffers[2] = new Text(2048);
	this->scratchBuffers[3] = new Text(2048);
	this->scratchBuffers[4] = new Text(2048);
	this->scratchBuffers[5] = new Text(2048);
	this->scratchBuffers[6] = new Text(2048);


	this->textCount[Strings::FILE_BOOKSTRINGS] = (BookStrings::MAX + 1);
	this->textCount[Strings::FILE_CODESTRINGS] = (CodeStrings::MAX + 1);
	this->textCount[Strings::FILE_ENTITYSTRINGS] = (EntityStrings::MAX + 1);
	this->textCount[Strings::FILE_FILESTRINGS] = (FileStrings::MAX + 1);
	this->textCount[Strings::FILE_MEDALSTRINGS] = (MedalStrings::MAX + 1);
	this->textCount[Strings::FILE_MENUSTRINGS] = (MenuStrings::MAX + 1);
	this->textCount[Strings::FILE_M_01] = 125;
	this->textCount[Strings::FILE_M_02] = 79;
	this->textCount[Strings::FILE_M_03] = 65;
	this->textCount[Strings::FILE_M_04] = 102;
	this->textCount[Strings::FILE_M_05] = 27;
	this->textCount[Strings::FILE_M_06] = 66;
	this->textCount[Strings::FILE_M_07] = 75;
	this->textCount[Strings::FILE_M_08] = 63;
	this->textCount[Strings::FILE_M_09] = 63;
	this->textCount[Strings::FILE_M_TEST] = 8;
	this->textCount[Strings::FILE_TRANSLATIONS] = (Translations::MAX + 1);

	this->beginTextLoading();

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < Strings::FILE_MAX; ++j) {
			this->textSizes[j] = std::max(this->textSizes[j], this->textIndex[(j + i * Strings::FILE_MAX) * 3 + 2]);
		}
	}

	int totalStrings, totalBytes, totalLanguages = 0;
	for (const auto& lang : Localization::STRINGS_MENUS) { // Itera por cada idioma
		totalStrings = 0;
		totalBytes = 0;
		for (const char* str : lang) { // Itera por cada cadena
			totalStrings++;
			totalBytes += strlen(str) + 1; // Suma la longitud de cada cadena
		}

		this->textSizes[Strings::FILE_MENUSTRINGS] = std::max(this->textSizes[Strings::FILE_MENUSTRINGS], this->textIndex[(Strings::FILE_MENUSTRINGS + totalLanguages * Strings::FILE_MAX) * 3 + 2] + totalBytes);
		totalLanguages++;
	}

	this->textCount[Strings::FILE_MENUSTRINGS] += totalStrings;


	this->finishTextLoading();

	this->allocateText(Strings::FILE_BOOKSTRINGS);
	this->allocateText(Strings::FILE_CODESTRINGS);
	this->allocateText(Strings::FILE_ENTITYSTRINGS);
	this->allocateText(Strings::FILE_MEDALSTRINGS);
	this->allocateText(Strings::FILE_MENUSTRINGS);
	this->allocateText(Strings::FILE_TRANSLATIONS);
	this->defaultLanguage = 0;

	this->beginTextLoading();
	this->loadTextFromIndex(this->defaultLanguage, Strings::FILE_BOOKSTRINGS);
	this->loadTextFromIndex(this->defaultLanguage, Strings::FILE_CODESTRINGS);
	this->loadTextFromIndex(this->defaultLanguage, Strings::FILE_ENTITYSTRINGS);
	this->loadTextFromIndex(this->defaultLanguage, Strings::FILE_MEDALSTRINGS);
	this->loadTextFromIndex(this->defaultLanguage, Strings::FILE_MENUSTRINGS);
	this->loadTextFromIndex(this->defaultLanguage, Strings::FILE_TRANSLATIONS);
	this->finishTextLoading();

	this->resetTextArgs();
	return true;
}

bool Localization::isSpace(char c) {
	return c == ' ' || c == '\n' || c == '|' || c == '\r' || c == '\t';
}
bool Localization::isDigit(char c) {
	return c >= '0' && c <= '9';
}

char Localization::toLower(char c) {
	if (c >= 'A' && c <= 'Z') {
		return (char)(c - 'A' + 'a');
	}
	return c;
}

char Localization::toUpper(char c) {
	if (c >= 'a' && c <= 'z') {
		return (char)(c - 'a' + 'A');
	}
	return c;
}

Text* Localization::getSmallBuffer() {
	for (int i = (Localization::MAXBUFFERS-1); i >= 0; --i) {
		if ((this->bufferFlags & 1 << i) == 0x0) {
			this->bufferFlags |= 1 << i;
			return this->scratchBuffers[i];
		}
	}
	return nullptr;
}

Text* Localization::getFatalErrorBuffer() {
	this->scratchBuffers[0]->setLength(0);
	return this->scratchBuffers[0];
}

Text* Localization::getLargeBuffer() {
	for (int i = 0; i < (Localization::MAXBUFFERS); ++i) {
		if ((this->bufferFlags & 1 << i) == 0x0) {
			this->bufferFlags |= 1 << i;
			return this->scratchBuffers[i];
		}
	}
	return nullptr;
}

void Localization::freeAllBuffers() {
	this->bufferFlags = 0;
	for (int i = 0; i < (Localization::MAXBUFFERS); ++i) {
		this->scratchBuffers[i]->setLength(0);
	}
}

void Localization::allocateText(int index) {
	this->text[index] = new char[this->textSizes[index]];
	this->textMap[index] = new uint16_t[this->textCount[index]];
}

void Localization::unloadText(int index) {
	if (this->text[index]) {
		delete[] this->text[index];
		this->text[index] = nullptr;
	}

	if (this->textMap[index]) {
		delete[] this->textMap[index];
		this->textMap[index] = nullptr;
	}
}

void Localization::setLanguage(int language) {

	this->defaultLanguage = language;
	this->beginTextLoading();
	for (int i = 0; i < (Strings::FILE_MAX); ++i) {
		if (this->text[i] != nullptr) {
			this->loadTextFromIndex(this->defaultLanguage, i);
		}
	}
	this->finishTextLoading();
}

void Localization::beginTextLoading() {
	Applet* app = CAppContainer::getInstance()->app;

	this->textIndex = app->resource->loadFileIndex(Resources::RES_STRINGS_IDX_GZ);
	this->textLastType = -1;
	this->textCurChunk = -1;
	this->textChunkStream = new InputStream();
}

void Localization::finishTextLoading() {

	if (this->textIndex != nullptr) {
		delete[] this->textIndex;
	}
	this->textIndex = nullptr;

	this->textChunkStream->close();
	if (this->textChunkStream != nullptr) {
		delete this->textChunkStream;
	}
	this->textChunkStream = nullptr;
}

void Localization::loadTextFromIndex(int i, int textLastType) {
	Applet* app = CAppContainer::getInstance()->app;

	if (textLastType < this->textLastType) {
		app->Error(87); // ERR_STRINGTABLE
	}

	this->textLastType = textLastType;
	int n2 = this->textIndex[(textLastType + i * Strings::FILE_MAX) * 3 + 0];
	int n3 = this->textIndex[(textLastType + i * Strings::FILE_MAX) * 3 + 1];
	int n4 = this->textIndex[(textLastType + i * Strings::FILE_MAX) * 3 + 2];
	if (this->textCurChunk != n2) {
		this->textChunkStream->loadFile(Resources::RES_STRINGS_ARRAY[n2], InputStream::LOADTYPE_RESOURCE);
		this->textCurOffset = 0;
		this->textCurChunk = n2;
	}
	if (n3 != this->textCurOffset) {
		app->resource->bufSkip(this->textChunkStream, n3 - this->textCurOffset, false);
		this->textCurOffset += n3 - this->textCurOffset;
	}

	app->resource->readByteArray(this->textChunkStream, (uint8_t*)this->text[textLastType], 0, n4);
	this->textCurOffset += n4;

	int strSize, totalBytes = 0;
	if (textLastType == Strings::FILE_MENUSTRINGS) {
		char* strText = this->text[textLastType] + n4;
		for (const char* str : Localization::STRINGS_MENUS[i]) { // Itera por cada cadena
			strSize = strlen(str);
			totalBytes += strSize; // Suma la longitud de cada cadena
			strncpy(strText, str, strSize);
			strText += strSize;
			*strText++ = 0;
			totalBytes += 1;
		}
	}

	uint16_t* textMap = this->textMap[textLastType];
	char* text = this->text[textLastType];
	int j = 0;
	textMap[j++] = 0;
	for (int i = 0; i < n4 + totalBytes; ++i) {
		/*if (i == 0) {
			printf("[%d][%d][%d] %s\n", n2, textLastType, j-1, &text[i]);
		}*/
		if (text[i] == 0) {
			//printf("[%d][%d][%d] %s\n",n2, textLastType, j, &text[i + 1]);
			textMap[j++] = (short)(i + 1);
		}
	}
}

void Localization::loadText(int index)
{
	this->allocateText(index);
	this->beginTextLoading();
	this->loadTextFromIndex(this->defaultLanguage, index);
	this->finishTextLoading();
}

void Localization::resetTextArgs()
{
	this->numTextArgs = 0;
	this->dynamicArgs->setLength(0);
}

void Localization::addTextArg(char c) {
	Applet* app = CAppContainer::getInstance()->app;
	if (this->numTextArgs + 1 >= 50) {
		app->Error("Added too many String Args");
	}
	this->dynamicArgs->append(c);
	this->argIndex[this->numTextArgs++] = (int16_t)this->dynamicArgs->length();
}

void Localization::addTextArg(int i)
{
	Applet* app = CAppContainer::getInstance()->app;
	if (this->numTextArgs + 1 >= 50) {
		app->Error("Added too many String Args");
	}
	this->dynamicArgs->append(i);
	this->argIndex[this->numTextArgs++] = (int16_t)this->dynamicArgs->length();
}

void Localization::addTextArg(Text *text, int i, int i2) {
	Applet* app = CAppContainer::getInstance()->app;
	if (this->numTextArgs + 1 >= 50) {
		app->Error("Added too many String Args");
	}
	text->substring(this->dynamicArgs, i, i2 - i);
	this->argIndex[this->numTextArgs++] = (int16_t)this->dynamicArgs->length();
}

void Localization::addTextArg(Text* text) {
	Applet* app = CAppContainer::getInstance()->app;
	if (this->numTextArgs + 1 >= 50) {
		app->Error("Added too many String Args");
	}
	this->dynamicArgs->append(text);
	this->argIndex[this->numTextArgs++] = (int16_t)this->dynamicArgs->length();
}

void Localization::addTextIDArg(int16_t i) {
	this->addTextArg((int16_t)(i >> 10 & 0x1F), (int16_t)(i & 0x3FF));
}

void Localization::addTextArg(int16_t i, int16_t i2) {
	Text *text = Localization::getLargeBuffer();

	this->composeText(i, i2, text);
	this->addTextArg(text);
	text->dispose();
}

void Localization::composeText(int i, Text *text) {
	this->composeText((int16_t)(i >> 10 & 0x1F), (int16_t)(i & 0x3FF), text);
}

void Localization::composeText(int16_t n, int16_t n2, Text *text) {
#ifdef WOLFENRPG_PSP
	if (const char* replacement = getPSPTextOverride(n, n2)) {
		for (const char* c = replacement; *c != 0; ++c) {
			text->append(*c);
		}
		text->trim();
		return;
	}
#endif
	char *textBuff = this->text[n];
	Applet* app = CAppContainer::getInstance()->app;

	if (textBuff == nullptr) {
		app->Error(85); // ERR_LOCALIZE_UNLOADED_TYPE
	}
	if (n2 < 0 || n2 > this->textCount[n]) {

		app->Error(84); // ERR_LOCALIZE_INVALID_STRING
	}
	
	int16_t n3 = this->textMap[n][n2];
	for (int i = textBuff[n3] & 0xFF; i != 0; i = (textBuff[n3] & 0xFF)) {

		if (i == '\\') {
			++n3;

			char c = textBuff[n3];
			if (c == '%') {
				text->append(c);
			}
			else if (c == 'n') {
				text->append('\n');
			}
			else {
				text->append('\\');
				text->append(c);
			}
		}
		else if (i == '%') {
			int n6 = (((textBuff[n3 + 1] - '0') * 10) + (textBuff[n3 + 2] - '0')) - 1;
			n3 += 2;

			//text->appendTextArg(n6);
			int _n2 = 0;
			int _n3 = (int)this->argIndex[n6];
			if (n6 > 0) {
				_n2 = (int)this->argIndex[n6 - 1];
			}
			this->dynamicArgs->substring(text, _n2, _n3 - _n2);
		}
		else {
			text->append((char)i);
		}
		++n3;
	}

	text->trim(); // iOS only
}

void Localization::composeTextField(int i, Text *text) {
	this->composeText((int16_t)(i >> 10), (int16_t)(i & 0x3FF), text);
}

bool Localization::isEmptyString(int16_t i, int16_t i2) {
	return ((this->text[i][this->textMap[i][i2]] & 0xFF) == 0) ? true: false;
}

bool Localization::isEmptyString(int i) {
	return this->isEmptyString((int16_t)(i >> 10), (int16_t)(i & 0x3FF));
}

void Localization::getCharIndices(char c, int* i, int* i2)
{
	uint8_t chr = c & 0xff;

	int index1 = chr - '!';
	int index2 = 0;

	switch (chr) {
	case 0x85:
		index1 = 94;
		break;
	case 0x8b:
		index1 = 101;
		break;
	case 0x8c:
		index1 = 142;
		break;
	case 0x8d:
		index1 = 100;
		break;
	case 0x99:
		index1 = 107;
		break;
	case 0x9c:
		index1 = 143;
		break;
	case 0xa1:
		index1 = 120;
		break;
	case 0xa2:
		index1 = 127;
		break;
	case 0xa9:
		index1 = 106;
		break;
	case 0xbc:
	case 0xbd:
	case 0xbe:
		index1 = 108 + (chr - 0xbc);
		break;
	case 0xbf:
		index1 = 119;
		break;
	case 0xc0:
	case 0xc1:
	case 0xc2:
	case 0xc3:
	case 0xc4:
	case 0xc5:
		index2 = 121 + (chr - 0xc0);
		index1 = 128;
		break;
	case 0xc8:
	case 0xc9:
	case 0xca:
	case 0xcb:
		index2 = 121 + (chr - 0xc8);
		if (chr == 0xcb) {
			++index2;
		}
		index1 = 129;
		break;
	case 0xcc:
	case 0xcd:
	case 0xce:
	case 0xcf:
		index2 = 121 + (chr - 0xcc);
		if (chr == 0xcf) {
			++index2;
		}
		index1 = 130;
		break;
	case 0xd1:
		index1 = 134;
		index2 = 122;
		break;
	case 0xd2:
	case 0xd3:
	case 0xd4:
	case 0xd5:
	case 0xd6:
		index2 = 121 + (chr - 0xd2);
		index1 = 131;
		break;
	case 0xd8:
		index1 = 46;
		index2 = 14;
		break;
	case 0xd9:
	case 0xda:
	case 0xdb:
	case 0xdc:
		index2 = 121 + (chr - 0xd9);
		if (chr == 0xdc) {
			++index2;
		}
		index1 = 132;
		break;
	case 0xdd:
		index1 = 133;
		index2 = 122;
		break;
	case 0xdf:
		index1 = 117;
		break;
	case 0xe0:
	case 0xe1:
	case 0xe2:
	case 0xe3:
	case 0xe4:
	case 0xe5:
		index2 = 121 + (chr - 0xe0);
		index1 = 135;
		break;
	case 0xe7:
		index1 = 116;
		break;
	case 0xe8:
	case 0xe9:
	case 0xea:
	case 0xeb:
		index2 = 121 + (chr - 0xe8);
		if (chr == 0xeb) {
			++index2;
		}
		index1 = 136;
		break;
	case 0xec:
	case 0xed:
	case 0xee:
	case 0xef:
		index2 = 121 + (chr - 0xec);
		if (chr == 0xef) {
			++index2;
		}
		index1 = 137;
		break;
	case 0xf0:
		index1 = 118;
		break;
	case 0xf1:
		index1 = 140;
		index2 = 124;
		break;
	case 0xf2:
	case 0xf3:
	case 0xf4:
	case 0xf5:
	case 0xf6:
		index2 = 121 + (chr - 0xf2);
		index1 = 138;
		break;
	case 0xf9:
	case 0xfa:
	case 0xfb:
	case 0xfc:
		index2 = 121 + (chr - 0xf9);
		if (chr == 0xfc) {
			++index2;
		}
		index1 = 139;
		break;
	case 0xfd:
		index1 = 141;
		index2 = 122;
		break;
	case 0xff:
		index1 = 141;
		index2 = 125;
		break;
	}

	*i = index1;
	*i2 = index2;
}

// --------------------
// Text Class
// --------------------

Text::Text(int countChars) {
	//printf("Text::init\n");

	this->chars = new char[countChars];
	memset(this->chars, 0, countChars);
	this->_length = 0;
	this->chars[0] = '\0';
	this->stringWidth = -1;
}

Text::~Text() {
}

bool Text::startup() {
	//printf("Text::startup\n");

	return false;
}

int Text::length() {
	return this->_length;
}

void Text::setLength(int i) {
	if (i < 0) {
		i = 0;
	}
	this->_length = i;
	this->chars[i] = '\0';
}

Text* Text::deleteAt(int i, int i2) {
	memcpy(this->chars + i, this->chars + i + i2, this->_length - (i + i2));
	this->_length -= i2;
	this->chars[this->_length] = '\0';
	return this;
}

char Text::charAt(int i) {
	return this->chars[i];
}

void Text::setCharAt(char c, int i) {
	this->chars[i] = c;
}

Text* Text::append(char c) {
	this->chars[this->_length++] = c;
	this->chars[this->_length] = '\0';
	return this;
}

Text* Text::append(uint8_t c) {
	this->chars[this->_length++] = (char)c;
	this->chars[this->_length] = '\0';
	return this;
}

Text* Text::append(char* c) {
	int i, len;
	len = strlen(c);
	for (i = 0; i < len; i++) {
		this->chars[this->_length++] = c[i];
	}
	this->chars[this->_length] = '\0';
	return this;
}

Text* Text::append(int i) {
	return this->insert(i, this->_length);
}

Text* Text::append(Text* t) {
	return this->append(t, 0, t->_length);
}

Text* Text::append(Text* t, int i) {
	return this->append(t, i, t->_length - i);
}

Text* Text::append(Text* t, int i, int i2) {
	if (i2 > 0) {
		memcpy(this->chars + this->_length, t->chars + i, i2);
		this->_length += i2;
		this->chars[this->_length] = '\0';
	}
	return this;
}

Text* Text::insert(char c, int i) {
	memcpy(this->chars + i + 1, this->chars + i, this->_length - i);
	this->chars[i] = c;
	this->chars[++this->_length] = '\0';
	return this;
}

Text* Text::insert(uint8_t c, int i) {
	memcpy(this->chars + i + 1, this->chars + i, this->_length - i);
	this->chars[i] = c;
	this->chars[++this->_length] = '\0';
	return this;
}

Text* Text::insert(int i, int i2) {
	if (i < 0) {
		this->insert('-', i2);
		++i2;
		i = -i;
	}
	do {
		this->insert((char)('0' + i % 10), i2);
		i /= 10;
	} while (i != 0);
	return this;
}

Text* Text::insert(char* c, int i) {
	return this->insert(c, 0, strlen(c), i);
}

Text* Text::insert(char* c, int i, int i2, int i3) {
	memcpy(this->chars + i3 + i2, this->chars + i3, this->_length - i3);
	this->_length += i2;
	while (--i2 >= 0) {
		this->chars[i3++] = c[i++];
	}
	this->chars[this->_length] = '\0';
	return this;
}

int Text::findFirstOf(char c) {
	int i = 0;
	while (i < this->_length) {
		if (this->chars[i] == c) {
			return i;
		}
		++i;
	}
	return -1;
}

int Text::findFirstOf(char c, int i) {
	while (i < this->_length) {
		if (this->chars[i] == c) {
			return i;
		}
		++i;
	}
	return -1;
}

int Text::findAnyFirstOf(char* c, int i) {
	while (i < this->_length) {
		for (int j = 0; c[j] != '\0'; ++j) {
			if (this->chars[i] == c[j]) {
				return i;
			}
		}
		++i;
	}
	return -1;
}

int Text::findLastOf(char c) {
	int i = this->_length;
	while (--i >= 0) {
		if (this->chars[i] == c) {
			return i;
		}
	}
	return -1;
}

int Text::findLastOf(char c, int n) {
	while (--n >= 0) {
		if (this->chars[n] == c) {
			return n;
		}
	}
	return -1;
}

void Text::substring(Text* t, int i) {
	for (int j = i; j < this->_length; j++) {
		t->chars[t->_length++] = this->chars[j];
	}
}

void Text::substring(Text* t, int i, int i2) {
	for (int j = i; j < (i + i2); j++) {
		t->chars[t->_length++] = this->chars[j];
	}
}

void Text::dehyphenate() {
	this->dehyphenate(0, this->_length);
}

void Text::dehyphenate(int i, int i2) {
	int first;
	while ((first = this->findFirstOf('-', i)) != -1 && first < i + i2) {
		this->deleteAt(first, 1);
		i2 -= first - i + 2;
		i = ++first;
	}
}

void Text::trim() {
	this->trim(true, true);
}

void Text::trim(bool b, bool b2) {
	if (b) {
		while (this->_length > 0 && this->chars[0] == ' ') {
			this->deleteAt(0, 1);
		}
	}
	if (b2) {
		while (this->_length > 0 && this->chars[this->_length - 1] == ' ') {
			this->deleteAt(this->_length - 1, 1);
		}
	}
}

int Text::wrapText(int i) {
	return this->wrapText(0, i, -1, '|');
}

int Text::wrapText(int i, char c) {
	return this->wrapText(0, i, -1, c);
}

int Text::wrapText(int i, int i2, char c) {
	return this->wrapText(0, i, i2, c);
}

int Text::wrapText(int i, int i2, int i3, char c) {
	char wordBreaks[5];
	char* chars, n8;
	bool n9;
	int length, n4, n5, n6, n7, n10, n11, n12;

	memcpy(wordBreaks, "|\n- ", 5);

	length = this->_length;
	chars = this->chars;
	n4 = 0;
	n5 = 0;
	n6 = 0;
	n7 = i;
	n8 = '\0';
	n9 = false;
	while ((n12 = this->findAnyFirstOf(wordBreaks, i)) != -1) {
		n5 += n12 - i;
		if (n9 == false && n8 == '-') {
			--n5;
		}
		if (n5 + ((chars[n12] == '-') ? 1 : 0) > i2 || n8 == '|' || n8 == '\n') {
			n4 += i - n7;
			if (n9 != false) {
				--i;
			}
			n7 = this->insertLineBreak(n7, i - 1, c);
			i = n7 + 1;
			++n6;
			/*while (i < this->_length && n6 < i3 && (chars[i - 1] == '|' || chars[n - 1] == '\n')) {
				++n6;
				++n;
				++n7;
			}*/
			n5 = 1;
			n8 = 0;
			n9 = false;
			if (i3 > 0 && n6 == i3) {
				this->_length = n7;
				return n4;
			}
		}
		else {
			n9 = false;
			n8 = chars[n12];
			i = n12 + 1;
			++n5;
			if (n8 == '-' && chars[i] == '-') {
				n9 = true;
				++i;
			}
		}
	}
	n10 = n5 + (this->_length - i);
	if (n9 == false && n8 == '-') {
		--n10;
	}
	if (n10 > i2 || n8 == '|' || n8 == '\n') {
		n11 = n4 + (i - n7);
		if (n9 != false) {
			--i;
		}
		n7 = this->insertLineBreak(n7, i - 1, c);
		i = n7 + 1;
		++n6;
		if (i3 > 0 && n6 == i3) {
			this->_length = n7;
			return n11;
		}
	}
	this->stringWidth = this->_length - n7;
	this->dehyphenate(n7, this->_length - n7);
	return length;
}

int Text::insertLineBreak(int i, int i2, char c) {
	if (this->chars[i2] == '-') {
		++i2;
		if (this->chars[i2] == '-') {
			this->chars[i2] = c;
		}
		else {
			this->insert(c, i2);
		}
	}
	else {
		this->chars[i2] = c;
	}
	int oldlen = this->_length;
	this->dehyphenate(i, i2 - i - 1);
	return (i2 - (oldlen - this->_length)) + 1;
}

int Text::getStringWidth() {
	return this->getStringWidth(0, this->_length, true);
}

int Text::getStringWidth(bool b) {
	return this->getStringWidth(0, this->_length, b);
}

int Text::getStringWidth(int i, int i2, bool b) {
	Applet* app = CAppContainer::getInstance()->app;
	int n2 = 0;
	int n3 = 0;
	if (i2 == -1 || i2 >= this->_length) {
		i2 = this->_length;
	}
	if (i >= 0 && i < i2) {
		for (int j = i; j < i2; ++j) {
			char c = this->chars[j];
			if (c == '\n' || c == '|') {
				if (!b) {
					break;
				}
				if (n3 > n2) {
					n2 = n3;
					n3 = 0;
				}
			}
			else if (c == ' ') {
				n3 += Applet::CHAR_SPACING[app->fontType];
			}
			else if (c == '^' && j != i2 - 1) {
				int16_t n4 = (int16_t)(this->chars[++j] - '0');
				if (n4 < 0 || n4 > 9) {
					n3 += Applet::CHAR_SPACING[app->fontType];
					--j;
				}
			}
			else {
				n3 += Applet::CHAR_SPACING[app->fontType];
			}
		}
	}
	if (n3 > n2) {
		n2 = n3;
	}
	return n2;
}

int Text::getNumLines() {
	int numLines = 1;
	for (int i = 0; i < this->_length; ++i) {
		char c = this->chars[i];
		if (c == '\n' || c == '|') {
			++numLines;
		}
	}
	return numLines;
}

bool Text::compareTo(Text* t) {
	bool b = t->_length == this->_length;
	for (int i = 0; b && i < this->_length; i++) {
		b = (t->chars[i] == this->chars[i]) ? true : false;
	}
	return b;
}

bool Text::compareTo(char *str) {
	bool b = strlen(str) == this->_length;
	for (int i = 0; b && i < this->_length; i++) {
		b = (str[i] == this->chars[i]) ? true : false;
	}
	return b;
}

void Text::toLower() {
	for (int i = 0; i < this->_length; ++i) {
		this->chars[i] = Localization::toLower(this->chars[i]);
	}
}

void Text::toUpper() {
	for (int i = 0; i < this->_length; ++i) {
		this->chars[i] = Localization::toUpper(this->chars[i]);
	}
}

void Text::dispose() {
	Applet* app = CAppContainer::getInstance()->app;
	Localization* loc = app->localization;

	for (int i = 0; i < 7; ++i) {
		if (loc->scratchBuffers[i] == this) {
			loc->bufferFlags &= ~(1 << i);
			this->setLength(0);
			return;
		}
	}
}
