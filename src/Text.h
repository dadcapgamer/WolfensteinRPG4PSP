#ifndef __TEXT_H__
#define __TEXT_H__

#include "MenuStrings.h"

class Text;
class InputStream;

// -------------------
// Localization Class
// -------------------

class Localization
{
private:

public:
	static constexpr int MAXBUFFERS = 7;
	static constexpr int MAX_STRING_ARGS = 50;
	static constexpr uint8_t C_LINE = '\x80';
	static constexpr uint8_t C_ELLIPSES = '\x85';
	static constexpr uint8_t C_RED_ING = '¼';
	static constexpr uint8_t C_BLUE_ING = '½';
	static constexpr uint8_t C_GREEN_ING = '¾';
	static constexpr uint8_t C_CHECK = '\x87';
	static constexpr uint8_t C_MINIDASH = '\x88';
	static constexpr uint8_t C_CURSOR2 = '\x84';
	static constexpr uint8_t C_CURSOR = '\x8a';
	static constexpr uint8_t C_POINTER = '\x90';
	static constexpr uint8_t C_GREYLINE = '\x89';
	static constexpr uint8_t C_HEART = '\x8d';
	static constexpr uint8_t C_SHIELD = '\x8b';
	static constexpr uint8_t HYPHEN = '-';
	static constexpr uint8_t NEWLINE = '|';
	static constexpr uint8_t HARD_SPACE = ' ';

	static constexpr char* STRINGS_MENUS[4][49] = {
		{ // INGLES
		"Options", "Language", "Sound", "Video", "Input",
		"Sound", "SFX Volume:", "Music Volume:",
		"Video", "Window Mode:", "Windowed", "Borderless", "FullScreen", "VSync:", "Resolution:", "TinyGL:", "Apply Changes",
		"Input", "Touch Controls", "Bindings", "Controller",
		"Touch Controls", "Button Transparency:",
		"Bindings", "Unbound",
		"MOVEMENT",
		"Move Forward", "Move Backward", "Move Left", "Move Right", "Turn Left", "Turn Right",
		"OTHER",
		"Atk/Talk/Use", "Next Weapon", "Prev Weapon", "Pass Turn", "Automap", "Menu", "Items/Info", "Syringes", "Journal", "Reset Binds", " ",
		"Controller", "Vibration", "Vibration Intensity", "Deadzone", "Control layout:"
		},
		{ // FRANCES Appuyez sur une nouvelle touche pour
		"Options", "Langue", "Son", "Vidéo", "Commandes",
		"Son", "Volume effets :", "Volume musique :",
		"Vidéo", "Mod.Affichage :", "Fenêtré", "Sans Bords", "Plein Écran", "VSync :", "Résolution :", "TinyGL :", "Valider Les Modifications",
		"Commandes", "Contrôles Tactiles", "Assignation Commandes", "Manette",
		"Contrôles Tactiles", "Transparence touches",
		"Assignation Commandes", "Non Assgnée",
		"MOUVEMENT",
		"Avant", "Arrière", "Dépl.Gauche", "Dépl.Droit", "Tourner Gauche", "Tourner Droit",
		"AUTRE",
		"Att/Parl/Util", "Arme Suivante", "Arme Précédente", "Pass.Tour", "Carte", "Menu", "Articles/Info", "Seringues", "Journal", "Réinitialiser Les Commandes", " ",
		"Manette", "Vibration", "Intensité de Vibration", "Zone Morte", "Config. Contrôles"
		},
		{ // ITALIANO Premi un nuovo tasto per
		"Opzioni", "Lingua", "Audio", "Video", "Comandi",
		"Audio", "Volume Effettii:", "Volume Musica:",
		"Video", "Mod.Finestra:", "Finestra", "Senza Bordi", "Sch. Intero", "VSync:", "Risoluzione:", "TinyGL:", "Applica Modifiche",
		"Comandi", "Controlli Touch", "Assegnazione Comandi", "Controller",
		"Controlli Touch", "Trasparenza Tasti",
		"Assegnazione Comandi", "Non Assegnato",
		"MOVIMENTO",
		"Avanti", "Indietro", "Sposta Sinistra", "Sposta Destra", "Ruota Sinistra", "Ruota Destra",
		"ALTRO",
		"Att/Parl/Usa", "Arma Successiva", "Arma Precedente", "Pass.Turno", "Automappa", "Menu", "Oggetti/Info", "Siringhe", "Diario", "Ripristina Assegnazioni", " ",
		"Controller", "Vibrazione", "Intensità Vibrazione", "Zona Morta", "Schema controlli"
		},
		{ // ESPAÑOL Presiona una nueva tecla para
		"Opciones", "Idioma", "Sonido", "Vídeo", "Entrada",
		"Sonido", "Vol.Efectos:", "Vol.Música:",
		"Vídeo", "Modo Ventana:", "En Ventana", "Sin Bordes", "Pant.Completa", "VSync:", "Resolución:", "TinyGL:", "Aplicar Cambios",
		"Entrada", "Controles Táctiles", "Asignaciones", "Mando",
		"Controles Táctiles", "Transparencia de Botones:",
		"Asignaciones", "Sin vincular",
		"MOVIMIENTO",
		"Adelante", "Atras", "Mover Izquierda", "Mover Derecha", "Girar Izquierda", "Girar Derecha",
		"OTROS",
		"Atq/Habl/Usar", "Arma Siguiente", "Arma Anterior", "Pasar Turno", "Automapa", "Menú", "Artículos/Info", "Jeringas", "Diario", "Restablecer Asignaciones", " ",
		"Mando", "Vibración", "Intensidad de Vibración", "Zona Muerta", "Diseño de Control:"
		}
	};

	Text* scratchBuffers[Localization::MAXBUFFERS];
	int bufferFlags;
	Text* dynamicArgs;
	int16_t argIndex[Localization::MAX_STRING_ARGS];
	int numTextArgs;
	bool selectLanguage;
	int defaultLanguage;
	int textSizes[Strings::FILE_MAX];
	int textCount[Strings::FILE_MAX];
	char** text;
	uint16_t** textMap;
	int* textIndex;
	int textLastType;
	int textCurChunk;
	int textCurOffset;
	InputStream* textChunkStream;

	// Constructor
	Localization();
	// Destructor
	~Localization();

	bool startup();
	static bool isSpace(char c);
	static bool isDigit(char c);
	static char toLower(char c);
	static char toUpper(char c);
	Text* getSmallBuffer();
	Text* getFatalErrorBuffer();
	Text* getLargeBuffer();
	void freeAllBuffers();
	void allocateText(int index);
	void unloadText(int index);
	void setLanguage(int language);
	void beginTextLoading();
	void finishTextLoading();
	void loadTextFromIndex(int i, int textLastType);
	void loadText(int index);
	void resetTextArgs();
	void addTextArg(char c);
	void addTextArg(int i);
	void addTextArg(Text* text, int i, int i2);
	void addTextArg(Text* text);
	void addTextIDArg(int16_t i);
	void addTextArg(int16_t i, int16_t i2);
	static constexpr int STRINGID(int16_t i, int16_t i2) {
		return i << 10 | i2;
	};
	void composeText(int i, Text* text);
	void composeText(int16_t n, int16_t n2, Text* text);
	void composeTextField(int i, Text* text);
	bool isEmptyString(int16_t i, int16_t i2);
	bool isEmptyString(int i);
	void getCharIndices(char c, int* i, int* i2);
};

// -----------
// Text Class
// -----------

class Text
{
private:

public:
	char* chars;
	int _length;
	int stringWidth;

	// Constructor
	Text(int countChars);
	// Destructor
	~Text();

	bool startup();
	int length();
	void setLength(int i);
	Text* deleteAt(int i, int i2);
	char charAt(int i);
	void setCharAt(char c, int i);
	Text* append(char c);
	Text* append(uint8_t c);
	Text* append(char* c);
	Text* append(int i);
	Text* append(Text* t);
	Text* append(Text* t, int i);
	Text* append(Text* t, int i, int i2);
	Text* insert(char c, int i);
	Text* insert(uint8_t c, int i);
	Text* insert(int i, int i2);
	Text* insert(char* c, int i);
	Text* insert(char* c, int i, int i2, int i3);
	int findFirstOf(char c);
	int findFirstOf(char c, int i);
	int findLastOf(char c);
	int findLastOf(char c, int n);
	int findAnyFirstOf(char* c, int i);
	void substring(Text* t, int i);
	void substring(Text* t, int i, int i2);
	void dehyphenate();
	void dehyphenate(int i, int i2);
	void trim();
	void trim(bool b, bool b2);
	int wrapText(int i);
	int wrapText(int i, char c);
	int wrapText(int i, int i2, char c);
	int wrapText(int i, int i2, int i3, char c);
	int insertLineBreak(int i, int i2, char c);
	int getStringWidth();
	int getStringWidth(bool b);
	int getStringWidth(int i, int i2, bool b);
	int getNumLines();
	bool compareTo(Text* t);
	bool compareTo(char* str);
	void toLower();
	void toUpper();
	void dispose();
};

#endif