#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <MFRC522.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(SIDE_LEFT) == defined(SIDE_RIGHT)
#error "Build with exactly one of -DSIDE_LEFT / -DSIDE_RIGHT (use the PlatformIO envs 'left' and 'right')."
#endif

constexpr uint8_t NUM_ROWS = 4;
constexpr uint8_t NUM_COLS = 6;
constexpr uint8_t NUM_KEYS = NUM_ROWS * NUM_COLS;
constexpr uint8_t ROW_PINS[NUM_ROWS] = {4, 5, 6, 7};
constexpr uint8_t COL_PINS[NUM_COLS] = {15, 16, 17, 18, 21, 38};
constexpr bool keyExists(uint8_t row, uint8_t col) { return !(row == 3 && col < 3); }

constexpr uint8_t ENC_A_PIN = 10;
constexpr uint8_t ENC_B_PIN = 11;
constexpr uint8_t ENC_SW_PIN = 12;
constexpr int8_t ENC_DIRECTION = 1;

constexpr uint8_t BACKLIGHT_PIN = 13;
constexpr uint8_t BACKLIGHT_LEVELS[] = {0, 25, 64, 128, 192, 255};
constexpr uint8_t BACKLIGHT_NUM_LEVELS = sizeof(BACKLIGHT_LEVELS);
constexpr uint8_t BACKLIGHT_DEFAULT_LEVEL = 2;
constexpr uint32_t BACKLIGHT_IDLE_MS = 60UL * 1000UL;

.
constexpr bool HAVE_BATT_SENSE = false;
constexpr uint8_t BATT_ADC_PIN = 1;
constexpr float BATT_DIVIDER_RATIO = 2.0f;
constexpr uint8_t SPI_SCK_PIN  = 40;
constexpr uint8_t SPI_MOSI_PIN = 41;
constexpr uint8_t SPI_MISO_PIN = 42;

constexpr uint8_t TFT_CS_PIN  = 37;  
constexpr uint8_t TFT_DC_PIN  = 8;  
constexpr uint8_t TFT_RST_PIN = 9;    
constexpr uint8_t TFT_BL_PIN  = 36; 


constexpr uint8_t RFID_CS_PIN  = 39; 
constexpr uint8_t RFID_RST_PIN = 14; 


constexpr uint32_t DEBOUNCE_MS = 5;
constexpr uint32_t HEARTBEAT_MS = 50;
constexpr uint32_t REMOTE_TIMEOUT_MS = 250;
constexpr uint32_t BACKLIGHT_SYNC_MS = 500;
constexpr uint32_t DISPLAY_UPDATE_MS = 200;
constexpr uint32_t BATTERY_SAMPLE_MS = 5000;
constexpr uint32_t NFC_POLL_MS = 150;
constexpr uint8_t SPLIT_WIFI_CHANNEL = 1;

#ifdef SIDE_LEFT
#include <BleKeyboard.h>
#else
#define KEY_LEFT_CTRL   0x80
#define KEY_LEFT_SHIFT  0x81
#define KEY_LEFT_ALT    0x82
#define KEY_RIGHT_SHIFT 0x85
#define KEY_RIGHT_ALT   0x86
#define KEY_UP_ARROW    0xDA
#define KEY_DOWN_ARROW  0xD9
#define KEY_LEFT_ARROW  0xD8
#define KEY_RIGHT_ARROW 0xD7
#define KEY_BACKSPACE   0xB2
#define KEY_TAB         0xB3
#define KEY_RETURN      0xB0
#define KEY_DELETE      0xD4
#define KEY_F1          0xC2
#define KEY_F2          0xC3
#define KEY_F3          0xC4
#define KEY_F4          0xC5
#define KEY_F5          0xC6
#define KEY_F6          0xC7
#define KEY_F7          0xC8
#define KEY_F8          0xC9
#define KEY_F9          0xCA
#define KEY_F10         0xCB
#define KEY_F11         0xCC
#define KEY_F12         0xCD
#endif

constexpr uint16_t KC_TRNS = 0;
constexpr uint16_t KC_MO1 = 0x100;
constexpr uint8_t NUM_LAYERS = 2;

const uint16_t KEYMAP[NUM_LAYERS][2][NUM_ROWS][NUM_COLS] = {
    {
        {
            {KEY_TAB,        'q', 'w', 'e', 'r', 't'},
            {KEY_LEFT_CTRL,  'a', 's', 'd', 'f', 'g'},
            {KEY_LEFT_SHIFT, 'z', 'x', 'c', 'v', 'b'},
            {0, 0, 0,        KEY_LEFT_ALT, KC_MO1, ' '},
        },
        {
            {KEY_BACKSPACE,   'p', 'o', 'i', 'u', 'y'},
            {'\'',            ';', 'l', 'k', 'j', 'h'},
            {KEY_RIGHT_SHIFT, '/', '.', ',', 'm', 'n'},
            {0, 0, 0,         KEY_RIGHT_ALT, KC_MO1, KEY_RETURN},
        },
    },
    {
        {
            {'`',     '1',    '2',    '3',    '4',    '5'},
            {KC_TRNS, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5},
            {KC_TRNS, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10},
            {0, 0, 0, KC_TRNS, KC_TRNS, KC_TRNS},
        },
        {
            {KEY_DELETE, '0', '9',             '8',          '7',            '6'},
            {'=',        '-', KEY_RIGHT_ARROW, KEY_UP_ARROW, KEY_DOWN_ARROW, KEY_LEFT_ARROW},
            {KC_TRNS,    '\\', ']',            '[',          KEY_F12,        KEY_F11},
            {0, 0, 0,    KC_TRNS, KC_TRNS,    KC_TRNS},
        },
    },
};

namespace proto {
constexpr uint8_t MAGIC_STATE = 0xA5;
constexpr uint8_t MAGIC_BACKLIGHT = 0x5A;
constexpr uint8_t NET_KEY = 0x3C;
constexpr uint32_t VALID_KEY_MASK = 0x00FFFFFFu & ~(0x7u << 18);

inline uint8_t crc8(const uint8_t* d, size_t n) {
    uint8_t c = NET_KEY;
    for (size_t i = 0; i < n; i++) {
        c ^= d[i];
        for (int b = 0; b < 8; b++) c = (c & 0x80) ? (uint8_t)((c << 1) ^ 0x07) : (uint8_t)(c << 1);
    }
    return c;
}

struct __attribute__((packed)) StatePacket {
    uint8_t magic;
    uint8_t seq;
    uint8_t keys[4];
    uint8_t enc;
    uint8_t flags;
    uint8_t battPct;
    uint8_t crc;
};

struct __attribute__((packed)) BacklightPacket {
    uint8_t magic;
    uint8_t seq;
    uint8_t duty;
    uint8_t crc;
};

inline void encodeState(StatePacket& p, uint8_t seq, uint32_t keys, uint8_t enc, uint8_t flags, uint8_t battPct) {
    p.magic = MAGIC_STATE;
    p.seq = seq;
    for (int i = 0; i < 4; i++) p.keys[i] = (uint8_t)((keys >> (8 * i)) & 0xFF);
    p.enc = enc;
    p.flags = flags;
    p.battPct = battPct;
    p.crc = crc8(reinterpret_cast<const uint8_t*>(&p), sizeof(p) - 1);
}

inline bool decodeState(const uint8_t* data, int len, StatePacket& out) {
    if (len != (int)sizeof(StatePacket)) return false;
    memcpy(&out, data, sizeof(out));
    if (out.magic != MAGIC_STATE) return false;
    return out.crc == crc8(data, sizeof(out) - 1);
}

inline uint32_t keysOf(const StatePacket& p) {
    uint32_t k = 0;
    for (int i = 0; i < 4; i++) k |= (uint32_t)p.keys[i] << (8 * i);
    return k & VALID_KEY_MASK;
}

inline void encodeBacklight(BacklightPacket& p, uint8_t seq, uint8_t duty) {
    p.magic = MAGIC_BACKLIGHT;
    p.seq = seq;
    p.duty = duty;
    p.crc = crc8(reinterpret_cast<const uint8_t*>(&p), sizeof(p) - 1);
}

inline bool decodeBacklight(const uint8_t* data, int len, BacklightPacket& out) {
    if (len != (int)sizeof(BacklightPacket)) return false;
    memcpy(&out, data, sizeof(out));
    if (out.magic != MAGIC_BACKLIGHT) return false;
    return out.crc == crc8(data, sizeof(out) - 1);
}

inline bool seqNewer(uint8_t seq, uint8_t last) { return (int8_t)(seq - last) > 0; }
}  

namespace battery {
void begin() {
    if (!HAVE_BATT_SENSE) return;
    pinMode(BATT_ADC_PIN, INPUT);
    analogReadResolution(12);
}

uint8_t readPercent() {
    if (!HAVE_BATT_SENSE) return 0;
    uint32_t mv = analogReadMilliVolts(BATT_ADC_PIN);
    float vBat = (mv / 1000.0f) * BATT_DIVIDER_RATIO;
    if (vBat >= 4.15f) return 100;
    if (vBat <= 3.30f) return 0;
    return (uint8_t)((vBat - 3.30f) * 100.0f / (4.15f - 3.30f));
}
} 

namespace encoder_table {
constexpr uint8_t DIR_NONE = 0x0;
constexpr uint8_t DIR_CW = 0x10;
constexpr uint8_t DIR_CCW = 0x20;
constexpr uint8_t R_START = 0x0;
constexpr uint8_t R_CW_FINAL = 0x1;
constexpr uint8_t R_CW_BEGIN = 0x2;
constexpr uint8_t R_CW_NEXT = 0x3;
constexpr uint8_t R_CCW_BEGIN = 0x4;
constexpr uint8_t R_CCW_FINAL = 0x5;
constexpr uint8_t R_CCW_NEXT = 0x6;

constexpr uint8_t TABLE[7][4] = {
    {R_START, R_CW_BEGIN, R_CCW_BEGIN, R_START},
    {R_CW_NEXT, R_START, R_CW_FINAL, (uint8_t)(R_START | DIR_CW)},
    {R_CW_NEXT, R_CW_BEGIN, R_START, R_START},
    {R_CW_NEXT, R_CW_BEGIN, R_CW_FINAL, R_START},
    {R_CCW_NEXT, R_START, R_CCW_BEGIN, R_START},
    {R_CCW_NEXT, R_CCW_FINAL, R_START, (uint8_t)(R_START | DIR_CCW)},
    {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};
inline uint8_t step(uint8_t state, uint8_t pinstate) { return TABLE[state & 0xF][pinstate & 0x3]; }
} 

class Encoder {
public:
    void begin() {
        pinMode(ENC_A_PIN, INPUT_PULLUP);
        pinMode(ENC_B_PIN, INPUT_PULLUP);
        pinMode(ENC_SW_PIN, INPUT_PULLUP);
    }
    void update() {
        const uint8_t pins = (uint8_t)((digitalRead(ENC_B_PIN) << 1) | digitalRead(ENC_A_PIN));
        state_ = encoder_table::step(state_, pins);
        const uint8_t dir = state_ & 0x30;
        if (dir == encoder_table::DIR_CW) counter_ = (uint8_t)(counter_ + ENC_DIRECTION);
        else if (dir == encoder_table::DIR_CCW) counter_ = (uint8_t)(counter_ - ENC_DIRECTION);
        const uint32_t now = millis();
        const bool raw = (digitalRead(ENC_SW_PIN) == LOW);
        if (raw != button_ && (now - buttonChangeMs_) >= 15) {
            button_ = raw;
            buttonChangeMs_ = now;
        }
    }
    uint8_t counter() const { return counter_; }
    bool button() const { return button_; }
private:
    uint8_t state_ = 0;
    uint8_t counter_ = 0;
    bool button_ = false;
    uint32_t buttonChangeMs_ = 0;
};

class Matrix {
public:
    void begin() {
        for (uint8_t r = 0; r < NUM_ROWS; r++) {
            pinMode(ROW_PINS[r], OUTPUT);
            digitalWrite(ROW_PINS[r], HIGH);
        }
        for (uint8_t c = 0; c < NUM_COLS; c++) pinMode(COL_PINS[c], INPUT_PULLUP);
    }
    bool scan() {
        const uint32_t now = millis();
        bool changed = false;
        for (uint8_t r = 0; r < NUM_ROWS; r++) {
            digitalWrite(ROW_PINS[r], LOW);
            delayMicroseconds(20);
            for (uint8_t c = 0; c < NUM_COLS; c++) {
                if (!keyExists(r, c)) continue;
                const uint8_t idx = r * NUM_COLS + c;
                const bool raw = (digitalRead(COL_PINS[c]) == LOW);
                const bool cur = (state_ >> idx) & 1u;
                if (raw != cur && (now - lastChangeMs_[idx]) >= DEBOUNCE_MS) {
                    state_ ^= (1u << idx);
                    lastChangeMs_[idx] = now;
                    changed = true;
                }
            }
            digitalWrite(ROW_PINS[r], HIGH);
        }
        return changed;
    }
    uint32_t state() const { return state_; }
private:
    uint32_t state_ = 0;
    uint32_t lastChangeMs_[NUM_KEYS] = {0};
};


namespace backlight {
constexpr uint8_t LEDC_CHANNEL = 0;
constexpr uint32_t PWM_FREQ_HZ = 20000;
constexpr uint8_t PWM_BITS = 8;

void begin() {
    ledcSetup(LEDC_CHANNEL, PWM_FREQ_HZ, PWM_BITS);
    ledcAttachPin(BACKLIGHT_PIN, LEDC_CHANNEL);
    ledcWrite(LEDC_CHANNEL, 0);
}
void setDuty(uint8_t duty) { ledcWrite(LEDC_CHANNEL, duty); }
}  


struct RemoteState {
    uint32_t keys;
    uint8_t encCount;
    bool encButton;
    uint8_t battPct;
};

namespace {
const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;
RemoteState g_remote = {0, 0, false, 0};
uint32_t g_remoteMs = 0;
bool g_remoteValid = false;
uint8_t g_remoteSeq = 0;
uint8_t g_backlightDuty = 0;
bool g_backlightNew = false;
uint8_t g_backlightSeq = 0;
bool g_backlightSeqValid = false;
uint8_t g_txSeq = 0;

void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
    (void)mac;
#ifdef SIDE_LEFT
    proto::StatePacket p;
    if (!proto::decodeState(data, len, p)) return;
    const uint32_t now = millis();
    portENTER_CRITICAL(&g_mux);
    if (!g_remoteValid || proto::seqNewer(p.seq, g_remoteSeq) || (now - g_remoteMs) > 300) {
        g_remote.keys = proto::keysOf(p);
        g_remote.encCount = p.enc;
        g_remote.encButton = (p.flags & 1) != 0;
        g_remote.battPct = p.battPct;
        g_remoteSeq = p.seq;
        g_remoteMs = now;
        g_remoteValid = true;
    }
    portEXIT_CRITICAL(&g_mux);
#else
    proto::BacklightPacket p;
    if (!proto::decodeBacklight(data, len, p)) return;
    portENTER_CRITICAL(&g_mux);
    if (!g_backlightSeqValid || proto::seqNewer(p.seq, g_backlightSeq)) {
        g_backlightDuty = p.duty;
        g_backlightSeq = p.seq;
        g_backlightSeqValid = true;
        g_backlightNew = true;
    }
    portEXIT_CRITICAL(&g_mux);
#endif
}
} 

bool splitBegin() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_channel(SPLIT_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
    if (esp_now_init() != ESP_OK) return false;
    esp_now_register_recv_cb(onRecv);
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, BROADCAST_MAC, 6);
    peer.channel = SPLIT_WIFI_CHANNEL;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    return esp_now_add_peer(&peer) == ESP_OK;
}

void splitSendState(uint32_t keys, uint8_t encCount, bool encButton, uint8_t battPct) {
    proto::StatePacket p;
    proto::encodeState(p, g_txSeq++, keys, encCount, encButton ? 1 : 0, battPct);
    esp_now_send(BROADCAST_MAC, reinterpret_cast<const uint8_t *>(&p), sizeof(p));
}

bool splitTakeBacklight(uint8_t &duty) {
    bool fresh;
    portENTER_CRITICAL(&g_mux);
    fresh = g_backlightNew;
    duty = g_backlightDuty;
    g_backlightNew = false;
    portEXIT_CRITICAL(&g_mux);
    return fresh;
}

bool splitGetRemote(RemoteState &out, uint32_t &ageMs) {
    bool valid;
    uint32_t stamp;
    portENTER_CRITICAL(&g_mux);
    valid = g_remoteValid;
    out = g_remote;
    stamp = g_remoteMs;
    portEXIT_CRITICAL(&g_mux);
    ageMs = valid ? (millis() - stamp) : 0xFFFFFFFFu;
    return valid;
}

void splitSendBacklight(uint8_t duty) {
    proto::BacklightPacket p;
    proto::encodeBacklight(p, g_txSeq++, duty);
    esp_now_send(BROADCAST_MAC, reinterpret_cast<const uint8_t *>(&p), sizeof(p));
}

static Matrix matrix;
static Encoder encoder;

#ifdef SIDE_LEFT
static BleKeyboard ble("GarvitaSplit", "Garvita", 100);
static Preferences prefs;
static Adafruit_ST7735 tft(&SPI, TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);
static MFRC522 rfid(RFID_CS_PIN, RFID_RST_PIN);

static uint16_t pressedCode[2][NUM_KEYS];
static uint32_t prevKeys[2] = {0, 0};
static uint8_t layerHolds = 0;
static uint8_t brightnessLevel = BACKLIGHT_DEFAULT_LEVEL;
static uint8_t currentDuty = 0xFF;
static uint8_t lastSentDuty = 0xFF;
static uint32_t lastBacklightSendMs = 0;
static uint32_t lastActivityMs = 0;

static uint8_t lastLocalEnc = 0;
static uint8_t lastRemoteEnc = 0;
static bool remoteEncInit = false;
static bool lastLocalBtn = false;
static bool lastRemoteBtn = false;
static bool wasConnected = false;
static bool wasRemoteUp = false;

static bool leftKnobVolumeMode = false;

static uint8_t localBattPct = 100;
static uint8_t remoteBattPct = 0;
static uint32_t lastBattReadMs = 0;
static uint32_t lastDisplayUpdateMs = 0;
static uint32_t lastNfcPollMs = 0;
static bool tftReady = false;
static char lastNfcUid[16] = "None";

static void markActive() { lastActivityMs = millis(); }

static uint8_t getBacklightPercent() {
    uint16_t duty = BACKLIGHT_LEVELS[brightnessLevel];
    return (uint8_t)((duty * 100) / 255);
}

static void adjustBrightnessSteps(int8_t steps) {
    if (!steps) return;
    markActive();
    int16_t next = (int16_t)brightnessLevel + (steps > 0 ? 1 : -1);
    if (next < 0) next = 0;
    if (next >= (int16_t)BACKLIGHT_NUM_LEVELS) next = BACKLIGHT_NUM_LEVELS - 1;
    if ((uint8_t)next != brightnessLevel) {
        brightnessLevel = (uint8_t)next;
        prefs.putUChar("bl", brightnessLevel);
    }
}

static void applyVolumeSteps(int8_t steps) {
    if (!steps) return;
    markActive();
    if (steps > 8) steps = 8;
    if (steps < -8) steps = -8;
    if (!ble.isConnected()) return;
    for (int8_t i = 0; i < abs(steps); i++) {
        ble.write(steps > 0 ? KEY_MEDIA_VOLUME_UP : KEY_MEDIA_VOLUME_DOWN);
    }
}

static uint16_t lookup(uint8_t side, uint8_t r, uint8_t c) {
    const uint8_t layer = layerHolds > 0 ? 1 : 0;
    uint16_t code = KEYMAP[layer][side][r][c];
    if (code == KC_TRNS && layer > 0) code = KEYMAP[0][side][r][c];
    return code;
}

static void keyEvent(uint8_t side, uint8_t idx, bool pressed) {
    const uint8_t r = idx / NUM_COLS, c = idx % NUM_COLS;
    markActive();
    if (pressed) {
        const uint16_t code = lookup(side, r, c);
        pressedCode[side][idx] = code;
        if (code == 0) return;
        if (code == KC_MO1) { layerHolds++; return; }
        if (ble.isConnected()) ble.press((uint8_t)code);
    } else {
        const uint16_t code = pressedCode[side][idx];
        pressedCode[side][idx] = 0;
        if (code == 0) return;
        if (code == KC_MO1) { if (layerHolds) layerHolds--; return; }
        if (ble.isConnected()) ble.release((uint8_t)code);
    }
}

static void processKeys(uint8_t side, uint32_t now) {
    uint32_t diff = now ^ prevKeys[side];
    if (!diff) return;
    for (uint8_t i = 0; i < NUM_KEYS; i++)
        if ((diff >> i) & 1u && !((now >> i) & 1u)) keyEvent(side, i, false);
    for (uint8_t i = 0; i < NUM_KEYS; i++)
        if ((diff >> i) & 1u && ((now >> i) & 1u)) keyEvent(side, i, true);
    prevKeys[side] = now;
}

static void pollNfc(uint32_t now) {
    if (now - lastNfcPollMs < NFC_POLL_MS) return;  
    lastNfcPollMs = now;
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;
    markActive();
    snprintf(lastNfcUid, sizeof(lastNfcUid), "%02X:%02X:%02X:%02X",
             rfid.uid.uidByte[0], rfid.uid.uidByte[1],
             rfid.uid.uidByte[2], rfid.uid.uidByte[3]);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
}

static void tftRow(int16_t y, uint16_t color, const char* s) {
    char buf[27];
    snprintf(buf, sizeof(buf), "%-26s", s);  
    tft.setTextColor(color, ST77XX_BLACK);
    tft.setCursor(0, y);
    tft.print(buf);
}

static void updateTft(bool remoteUp) {
    if (!tftReady) return;
    char b[32];

    snprintf(b, sizeof(b), "BLE:%s  Layer:%u", ble.isConnected() ? "ON" : "OFF", layerHolds > 0 ? 1 : 0);
    tftRow(2, ST77XX_CYAN, b);

    if (!HAVE_BATT_SENSE)  snprintf(b, sizeof(b), "Battery: n/a");
    else if (remoteUp)     snprintf(b, sizeof(b), "Bat L:%3u%%  R:%3u%%", localBattPct, remoteBattPct);
    else                   snprintf(b, sizeof(b), "Bat L:%3u%%  R:---%%", localBattPct);
    tftRow(18, ST77XX_WHITE, b);

    const uint8_t blPct = (millis() - lastActivityMs > BACKLIGHT_IDLE_MS) ? 0 : getBacklightPercent();
    snprintf(b, sizeof(b), "Backlight: %3u%%", blPct);
    tftRow(34, ST77XX_YELLOW, b);

    const int16_t barW = tft.width() - 4;
    const int16_t fillW = (blPct * barW) / 100;
    tft.fillRect(2, 48, fillW, 4, ST77XX_GREEN);
    tft.fillRect(2 + fillW, 48, barW - fillW, 4, ST77XX_BLACK);

    snprintf(b, sizeof(b), "L-Knob:%s  R:VOL", leftKnobVolumeMode ? "VOL" : "BRT");
    tftRow(62, ST77XX_WHITE, b);

    snprintf(b, sizeof(b), "NFC: %s", lastNfcUid);
    tftRow(78, ST77XX_MAGENTA, b);
}

static void leftSetup() {
    prefs.begin("split", false);
    brightnessLevel = prefs.getUChar("bl", BACKLIGHT_DEFAULT_LEVEL) % BACKLIGHT_NUM_LEVELS;
    battery::begin();
    localBattPct = battery::readPercent();

   
    pinMode(TFT_CS_PIN, OUTPUT);   digitalWrite(TFT_CS_PIN, HIGH);
    pinMode(RFID_CS_PIN, OUTPUT);  digitalWrite(RFID_CS_PIN, HIGH);

    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, -1);

    pinMode(TFT_BL_PIN, OUTPUT);
    digitalWrite(TFT_BL_PIN, HIGH);         
    tft.initR(INITR_BLACKTAB);              
    tft.setRotation(1);                     
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setTextWrap(false);
    tft.drawRect(0, 46, tft.width(), 8, ST77XX_WHITE); 
    tftReady = true;

    rfid.PCD_Init();

    ble.begin();
    if (!splitBegin()) Serial.println("ESP-NOW init failed");
    markActive();
}

static void leftTick() {
    const uint32_t now = millis();
    matrix.scan();
    encoder.update();
    pollNfc(now);

    if (now - lastBattReadMs >= BATTERY_SAMPLE_MS) {
        localBattPct = battery::readPercent();
        lastBattReadMs = now;
    }

    const bool connected = ble.isConnected();
    if (connected != wasConnected) {
        wasConnected = connected;
        ble.releaseAll();
    }

    processKeys(0, matrix.state());

    int8_t leftSteps = (int8_t)(encoder.counter() - lastLocalEnc);
    lastLocalEnc = encoder.counter();
    if (leftSteps != 0) {
        if (leftKnobVolumeMode) applyVolumeSteps(leftSteps);
        else adjustBrightnessSteps(leftSteps);
    }

    if (encoder.button() && !lastLocalBtn) {
        leftKnobVolumeMode = !leftKnobVolumeMode;
        markActive();
    }
    lastLocalBtn = encoder.button();

    RemoteState rs = {0, 0, false, 0};
    uint32_t age = 0;
    const bool remoteUp = splitGetRemote(rs, age) && age < REMOTE_TIMEOUT_MS;
    if (remoteUp != wasRemoteUp) wasRemoteUp = remoteUp;

    if (remoteUp) {
        remoteBattPct = rs.battPct;
        if (!remoteEncInit) {
            lastRemoteEnc = rs.encCount;
            lastRemoteBtn = rs.encButton;
            remoteEncInit = true;
        }
        processKeys(1, rs.keys);

        int8_t rightSteps = (int8_t)(rs.encCount - lastRemoteEnc);
        lastRemoteEnc = rs.encCount;
        applyVolumeSteps(rightSteps);

        if (rs.encButton && !lastRemoteBtn && ble.isConnected()) {
            ble.write(KEY_MEDIA_MUTE);
            markActive();
        }
        lastRemoteBtn = rs.encButton;
    } else {
        remoteEncInit = false;
        processKeys(1, 0);
    }

    const uint8_t duty = (now - lastActivityMs > BACKLIGHT_IDLE_MS) ? 0 : BACKLIGHT_LEVELS[brightnessLevel];
    if (duty != currentDuty) {
        currentDuty = duty;
        backlight::setDuty(duty);
        if (HAVE_BATT_SENSE) ble.setBatteryLevel(localBattPct);
    }
    if (duty != lastSentDuty || now - lastBacklightSendMs >= BACKLIGHT_SYNC_MS) {
        splitSendBacklight(duty);
        lastSentDuty = duty;
        lastBacklightSendMs = now;
    }

    if (now - lastDisplayUpdateMs >= DISPLAY_UPDATE_MS) {
        updateTft(remoteUp);
        lastDisplayUpdateMs = now;
    }
}
#endif  

#ifdef SIDE_RIGHT
static uint32_t lastKeys = 0xFFFFFFFFu;
static uint8_t lastEnc = 0;
static bool lastBtn = false;
static uint32_t lastSendMs = 0;
static uint32_t lastBattMs = 0;
static uint8_t localBattPct = 100;
static uint8_t currentDuty = 0;

static void rightSetup() {
    battery::begin();
    localBattPct = battery::readPercent();
    if (!splitBegin()) Serial.println("ESP-NOW init failed");
}

static void rightTick() {
    matrix.scan();
    encoder.update();
    const uint32_t now = millis();

    if (now - lastBattMs >= BATTERY_SAMPLE_MS) {
        localBattPct = battery::readPercent();
        lastBattMs = now;
    }

    const uint32_t keys = matrix.state();
    const uint8_t enc = encoder.counter();
    const bool btn = encoder.button();

    if (keys != lastKeys || enc != lastEnc || btn != lastBtn || now - lastSendMs >= HEARTBEAT_MS) {
        splitSendState(keys, enc, btn, localBattPct);
        lastKeys = keys;
        lastEnc = enc;
        lastBtn = btn;
        lastSendMs = now;
    }

    uint8_t duty;
    if (splitTakeBacklight(duty) && duty != currentDuty) {
        currentDuty = duty;
        backlight::setDuty(duty);
    }
}
#endif 

void setup() {
    Serial.begin(115200);
    matrix.begin();
    encoder.begin();
    backlight::begin();
#ifdef SIDE_LEFT
    leftSetup();
#else
    rightSetup();
#endif
}

void loop() {
#ifdef SIDE_LEFT
    leftTick();
#else
    rightTick();
#endif
    delay(1);
}