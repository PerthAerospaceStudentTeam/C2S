/*
 * FISH_emulator.ino
 *
 * SPI master that stands in for FISH when talking to the C2Sv1.1 board
 * (comms.c / link.c, SLAVE_MODE). Requests heartbeats and images, and streams
 * received images to the PC over USB serial for receive_image.py to save.
 *
 * Link model (matches link.c):
 *   - One CS-low window carries exactly one frame. The C2S re-arms per window
 *     and restarts its TX buffer from byte 0 each time.
 *   - The C2S only queues its response after the request window closes, so the
 *     response is read back in a SEPARATE window by clocking dummy 0x00 bytes.
 *   - The C2S needs a few ms after CS rises to parse, respond and re-arm, so
 *     every window is followed by TURNAROUND_MS.
 *
 * Image transfer (matches serve_image() / on_ack()):
 *   REQUEST(IMAGE) -> RESPONSE(IMAGE, chunk at current offset)
 *   ACK            -> C2S advances the offset
 *   ... repeat until RESPONSE(IMAGE, 0 bytes) = end of image, or no image
 *   stored newer than the last one sent.
 *
 * Wiring (Nano -> C2S):  D13 SCK -> PB3,  D12 MISO <- PB4,  D11 MOSI -> PB5,
 *                        D10 CS  -> PC14, GND -> GND
 * The Nano is 5 V and the H7 is 3.3 V: level-shift, or at least check PC14
 * is 5 V tolerant before connecting.
 *
 * Serial (115200): 'h' heartbeat, 'i' fetch next image, 'p' toggle periodic
 * heartbeat. Image output, one line each:
 *   IMG BEGIN
 *   D <hex bytes of one chunk>
 *   IMG END <total bytes> <16-bit byte sum, hex>
 * or IMG NONE / IMG ABORT <bytes so far>.
 */

#include <SPI.h>
#include <stdint.h>
#include <string.h>

// ------------ ICD (mirror of icd.h) ------------
#define ICD_START_BYTE      0xFFu

#define ICD_ID_REQUEST      0xF0u
#define ICD_ID_RESPONSE     0xF1u
#define ICD_ID_ACK          0xF2u
#define ICD_ID_NAK          0xF3u

#define ICD_NODE_FISH       0x01u
#define ICD_NODE_C2S        0x03u

#define ICD_TYPE_IMAGE      0x00u
#define ICD_TYPE_HEARTBEAT  0x02u

#define ICD_FRAME_MAX       256u
#define ICD_HEADER_BYTES    3u
#define ICD_ADDR_BYTES      3u
#define ICD_CHECKSUM_BYTES  1u
#define ICD_DATA_MAX        (ICD_FRAME_MAX - ICD_HEADER_BYTES - ICD_ADDR_BYTES - ICD_CHECKSUM_BYTES)
#define ICD_LENGTH_MAX      (ICD_ADDR_BYTES + ICD_DATA_MAX)

// ------------ Configuration ------------
const int           CS_PIN           = 10;
const uint32_t      SERIAL_BAUD      = 115200;    // must match receive_image.py
const uint32_t      SPI_HZ           = 1000000;   // C2S services SPI3 by interrupt; keep it modest
const unsigned long TURNAROUND_MS    = 5;
const unsigned long HEARTBEAT_PERIOD = 2000;
const uint8_t       MAX_RETRIES      = 3;

typedef struct {
  uint8_t id;
  uint8_t has_addr;
  uint8_t source;
  uint8_t recipient;
  uint8_t type;
  uint8_t data_len;
  uint8_t data[ICD_DATA_MAX];
} icd_packet_t;

// Static buffers: the Nano only has 2 KB of RAM.
static uint8_t      s_frame[ICD_FRAME_MAX];
static icd_packet_t s_req;
static icd_packet_t s_res;
static bool         s_periodic = true;

// ------------ ICD packet (mirror of icd_packet.c) ------------
uint8_t icd_checksum(const uint8_t *frame, uint32_t len) {
  uint8_t sum = 0;
  for (uint32_t i = 0; i < len; i++) {
    sum = (uint8_t)(sum + frame[i]);
  }
  return sum;
}

uint32_t icd_build(const icd_packet_t *pkt, uint8_t out[ICD_FRAME_MAX]) {
  if (pkt->data_len > ICD_DATA_MAX) {
    return 0;
  }

  uint32_t length = pkt->has_addr ? ICD_ADDR_BYTES + pkt->data_len : 0u;

  out[0] = ICD_START_BYTE;
  out[1] = pkt->id;
  out[2] = (uint8_t)length;
  if (pkt->has_addr) {
    out[3] = pkt->source;
    out[4] = pkt->recipient;
    out[5] = pkt->type;
    memcpy(&out[6], pkt->data, pkt->data_len);
  }

  uint32_t end = ICD_HEADER_BYTES + length;
  out[end] = icd_checksum(out, end);
  return end + ICD_CHECKSUM_BYTES;
}

int icd_parse(const uint8_t *buf, uint32_t len, icd_packet_t *pkt) {
  uint32_t start = 0;
  while (start < len && buf[start] != ICD_START_BYTE) {
    start++;
  }

  const uint8_t *f = &buf[start];
  uint32_t avail = len - start;
  if (avail < ICD_HEADER_BYTES + ICD_CHECKSUM_BYTES) {
    return 0;
  }

  uint8_t length = f[2];
  if (length != 0u && (length < ICD_ADDR_BYTES || length > ICD_LENGTH_MAX)) {
    return 0;
  }

  uint32_t end = ICD_HEADER_BYTES + length;
  if (avail < end + ICD_CHECKSUM_BYTES || icd_checksum(f, end) != f[end]) {
    return 0;
  }

  memset(pkt, 0, sizeof(*pkt));
  pkt->id = f[1];
  pkt->has_addr = (length != 0u);
  if (pkt->has_addr) {
    pkt->source    = f[3];
    pkt->recipient = f[4];
    pkt->type      = f[5];
    pkt->data_len  = (uint8_t)(length - ICD_ADDR_BYTES);
    memcpy(pkt->data, &f[6], pkt->data_len);
  }
  return 1;
}

// ------------ Link (counterpart of link.c) ------------
// One CS window. tx may be NULL to clock dummy 0x00 bytes; rx may be NULL.
static void spi_window(const uint8_t *tx, uint8_t *rx, uint32_t len) {
  SPI.beginTransaction(SPISettings(SPI_HZ, MSBFIRST, SPI_MODE0));
  digitalWrite(CS_PIN, LOW);
  for (uint32_t i = 0; i < len; i++) {
    uint8_t b = SPI.transfer(tx ? tx[i] : 0x00);
    if (rx) {
      rx[i] = b;
    }
  }
  digitalWrite(CS_PIN, HIGH);
  SPI.endTransaction();
  delay(TURNAROUND_MS);
}

static void send_packet(const icd_packet_t *pkt) {
  uint32_t len = icd_build(pkt, s_frame);
  if (len > 0) {
    spi_window(s_frame, NULL, len);
  }
}

// Clocks a full frame window so any queued response (up to ICD_FRAME_MAX) is
// delivered in one go, which also lets the C2S clear its TX buffer.
static bool read_response(uint8_t type, icd_packet_t *res) {
  spi_window(NULL, s_frame, ICD_FRAME_MAX);
  return icd_parse(s_frame, ICD_FRAME_MAX, res)
      && res->id == ICD_ID_RESPONSE
      && res->has_addr
      && res->source == ICD_NODE_C2S
      && res->recipient == ICD_NODE_FISH
      && res->type == type;
}

static void send_request(uint8_t type) {
  memset(&s_req, 0, sizeof(s_req));
  s_req.id        = ICD_ID_REQUEST;
  s_req.has_addr  = 1;
  s_req.source    = ICD_NODE_FISH;
  s_req.recipient = ICD_NODE_C2S;
  s_req.type      = type;
  send_packet(&s_req);
}

static void send_ack(void) {
  memset(&s_req, 0, sizeof(s_req));
  s_req.id = ICD_ID_ACK;
  send_packet(&s_req);
}

// Request + read back, retried. A lost or corrupt response is safe to retry:
// the C2S only advances on ACK, so it re-serves the same data.
static bool transact(uint8_t type, icd_packet_t *res) {
  for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++) {
    send_request(type);
    if (read_response(type, res)) {
      return true;
    }
  }
  return false;
}

// ------------ Operations ------------
static void do_heartbeat(void) {
  if (!transact(ICD_TYPE_HEARTBEAT, &s_res) || s_res.data_len < 6) {
    Serial.println(F("HB NONE"));
    return;
  }

  uint32_t uptime = (uint32_t)s_res.data[0]
                  | ((uint32_t)s_res.data[1] << 8)
                  | ((uint32_t)s_res.data[2] << 16)
                  | ((uint32_t)s_res.data[3] << 24);

  Serial.print(F("HB uptime="));
  Serial.print(uptime);
  Serial.print(F("s ready="));
  Serial.print(s_res.data[4]);
  Serial.print(F(" images="));
  Serial.println(s_res.data[5]);
}

static void print_hex_line(const uint8_t *data, uint8_t len) {
  static const char HEX_DIGITS[] = "0123456789ABCDEF";
  Serial.print(F("D "));
  for (uint8_t i = 0; i < len; i++) {
    Serial.write(HEX_DIGITS[data[i] >> 4]);
    Serial.write(HEX_DIGITS[data[i] & 0x0F]);
  }
  Serial.println();
}

// Fetches the next stored image (the C2S serves the oldest one newer than the
// last image it completed) and streams it to the PC chunk by chunk.
static void do_image(void) {
  uint32_t total  = 0;
  uint16_t sum    = 0;
  unsigned long t0 = millis();
  bool begun = false;

  for (;;) {
    if (!transact(ICD_TYPE_IMAGE, &s_res)) {
      Serial.print(F("IMG ABORT "));
      Serial.println(total);
      return;
    }

    if (s_res.data_len == 0) {
      break;
    }

    if (!begun) {
      Serial.println(F("IMG BEGIN"));
      begun = true;
    }
    print_hex_line(s_res.data, s_res.data_len);
    for (uint8_t i = 0; i < s_res.data_len; i++) {
      sum = (uint16_t)(sum + s_res.data[i]);
    }
    total += s_res.data_len;
    send_ack();
  }

  if (!begun) {
    Serial.println(F("IMG NONE"));
    return;
  }

  Serial.print(F("IMG END "));
  Serial.print(total);
  Serial.print(' ');
  Serial.println(sum, HEX);

  Serial.print(F("# "));
  Serial.print(total);
  Serial.print(F(" bytes in "));
  Serial.print(millis() - t0);
  Serial.println(F(" ms"));
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  SPI.begin();

  Serial.println(F("READY FISH emulator: 'h' heartbeat, 'i' image, 'p' toggle periodic heartbeat"));
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 'h' || cmd == 'H') {
      do_heartbeat();
    } else if (cmd == 'i' || cmd == 'I') {
      do_image();
    } else if (cmd == 'p' || cmd == 'P') {
      s_periodic = !s_periodic;
      Serial.println(s_periodic ? F("# periodic heartbeat on") : F("# periodic heartbeat off"));
    }
  }

  static unsigned long last_hb = 0;
  if (s_periodic && millis() - last_hb >= HEARTBEAT_PERIOD) {
    last_hb = millis();
    do_heartbeat();
  }
}
