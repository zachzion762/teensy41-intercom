#include "WebConfig.h"

#include <string.h>

#include "BoardConfig.h"

using qindesign::network::Ethernet;

namespace {

const uint32_t kRequestTimeoutMs = 4000;
const size_t   kMaxRequestBytes  = 4096;
const size_t   kReadChunk        = 512;

int hexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

String urlDecode(const String& in) {
  String out;
  out.reserve(in.length());
  for (unsigned int i = 0; i < in.length(); i++) {
    const char c = in[i];
    if (c == '+') {
      out += ' ';
    } else if (c == '%' && i + 2 < in.length()) {
      const int hi = hexVal(in[i + 1]);
      const int lo = hexVal(in[i + 2]);
      if (hi >= 0 && lo >= 0) {
        out += (char)((hi << 4) | lo);
        i += 2;
      } else {
        out += c;
      }
    } else {
      out += c;
    }
  }
  return out;
}

// Minimal HTML escaping for values echoed back into the form.
String htmlEscape(const char* s) {
  String out;
  for (const char* p = s; *p; p++) {
    switch (*p) {
      case '&':  out += F("&amp;");  break;
      case '<':  out += F("&lt;");   break;
      case '>':  out += F("&gt;");   break;
      case '"':  out += F("&quot;"); break;
      case '\'': out += F("&#39;");  break;
      default:   out += *p;          break;
    }
  }
  return out;
}

String base64Encode(const char* in) {
  static const char kTable[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String out;
  const size_t len = strlen(in);
  for (size_t i = 0; i < len; i += 3) {
    uint32_t v = (uint32_t)(uint8_t)in[i] << 16;
    if (i + 1 < len) v |= (uint32_t)(uint8_t)in[i + 1] << 8;
    if (i + 2 < len) v |= (uint32_t)(uint8_t)in[i + 2];
    out += kTable[(v >> 18) & 0x3F];
    out += kTable[(v >> 12) & 0x3F];
    out += (i + 1 < len) ? kTable[(v >> 6) & 0x3F] : '=';
    out += (i + 2 < len) ? kTable[v & 0x3F] : '=';
  }
  return out;
}

// Length-checked, non-short-circuiting compare so a wrong credential takes
// the same time to reject regardless of how much of it was correct.
bool secureEquals(const String& a, const String& b) {
  if (a.length() != b.length()) return false;
  uint8_t diff = 0;
  for (unsigned i = 0; i < a.length(); i++) diff |= (uint8_t)(a[i] ^ b[i]);
  return diff == 0;
}

// Returns the value of one header, or an empty string. Case-insensitive.
String headerValue(const String& req, int headerEnd, const char* nameLower) {
  const String head = req.substring(0, headerEnd < 0 ? (int)req.length() : headerEnd);
  String lower = head;
  lower.toLowerCase();
  const int at = lower.indexOf(nameLower);
  if (at < 0) return String();
  int lineEnd = head.indexOf('\r', at);
  if (lineEnd < 0) lineEnd = (int)head.length();
  String value = head.substring(at + (int)strlen(nameLower), lineEnd);
  // trim surrounding whitespace
  int b = 0;
  while (b < (int)value.length() && (value[b] == ' ' || value[b] == '\t')) b++;
  int e = (int)value.length();
  while (e > b && (value[e - 1] == ' ' || value[e - 1] == '\t')) e--;
  return value.substring(b, e);
}

long headerContentLength(const String& req, int headerEnd) {
  return headerValue(req, headerEnd, "content-length:").toInt();
}

}  // namespace

void WebConfig::begin(IntercomConfig& cfg, const AppHooks& hooks, uint16_t port) {
  cfg_   = &cfg;
  hooks_ = hooks;
  server_.begin(port);
  running_ = true;
}

void WebConfig::reset() {
  client_.stop();
  req_     = "";
  started_ = 0;
}

bool WebConfig::requestComplete() const {
  const int headerEnd = req_.indexOf("\r\n\r\n");
  if (headerEnd < 0) return false;
  const long contentLength = headerContentLength(req_, headerEnd);
  return (long)req_.length() >= headerEnd + 4 + contentLength;
}

void WebConfig::update(uint32_t now) {
  if (!running_ || !cfg_) return;

  if (!client_) {
    client_ = server_.accept();
    if (!client_) return;
    req_     = "";
    req_.reserve(512);
    started_ = now;
  }

  // Drop connections that stall so one bad client cannot hold the slot.
  if (now - started_ > kRequestTimeoutMs) {
    reset();
    return;
  }

  size_t budget = kReadChunk;
  while (client_.available() > 0 && budget-- > 0) {
    const int c = client_.read();
    if (c < 0) break;
    if (req_.length() < kMaxRequestBytes) req_ += (char)c;
  }

  if (req_.length() >= kMaxRequestBytes) {
    sendStatus(413, "Payload Too Large", "Request too large.");
    reset();
    return;
  }

  if (!requestComplete()) {
    if (!client_.connected() && client_.available() == 0) reset();
    return;
  }

  route();

  const bool reboot = rebootAfterResponse_;
  rebootAfterResponse_ = false;
  reset();

  if (reboot && hooks_.onReboot) hooks_.onReboot(hooks_.ctx);
}

bool WebConfig::authorized() const {
  // No password configured -- the page is open, as documented.
  if (cfg_->webPass[0] == '\0') return true;

  const int headerEnd = req_.indexOf("\r\n\r\n");
  const String value = headerValue(req_, headerEnd, "authorization:");
  if (value.length() == 0) return false;

  String scheme = value.substring(0, 6);
  scheme.toLowerCase();
  if (!(scheme == "basic ")) return false;

  char expectedRaw[72];
  snprintf(expectedRaw, sizeof(expectedRaw), "%s:%s", cfg_->webUser, cfg_->webPass);
  return secureEquals(value.substring(6), base64Encode(expectedRaw));
}

void WebConfig::sendAuthChallenge() {
  const char* body = "Authentication required.";
  client_.print(F("HTTP/1.1 401 Unauthorized\r\n"
                  "WWW-Authenticate: Basic realm=\"Teensy Intercom\", charset=\"UTF-8\"\r\n"
                  "Content-Type: text/plain\r\nConnection: close\r\n"));
  client_.printf("Content-Length: %u\r\n\r\n", (unsigned)strlen(body));
  client_.print(body);
  client_.flush();
}

void WebConfig::route() {
  // Every route is behind the credential check, including /reboot.
  if (!authorized()) {
    sendAuthChallenge();
    return;
  }

  const int sp1 = req_.indexOf(' ');
  const int sp2 = req_.indexOf(' ', sp1 + 1);
  if (sp1 < 0 || sp2 < 0) {
    sendStatus(400, "Bad Request", "Malformed request.");
    return;
  }

  const String method = req_.substring(0, sp1);
  const String path   = req_.substring(sp1 + 1, sp2);

  if (method == "GET" && (path == "/" || path.startsWith("/?"))) {
    sendFormPage(nullptr);
    return;
  }

  if (method == "POST" && path == "/save") {
    const int headerEnd = req_.indexOf("\r\n\r\n");
    const String body   = req_.substring(headerEnd + 4);
    char notice[96] = {0};
    applyForm(body, notice, sizeof(notice));
    sendFormPage(notice);
    return;
  }

  if (method == "POST" && path == "/reboot") {
    sendStatus(200, "OK", "Rebooting. This page will stop responding.");
    rebootAfterResponse_ = true;
    return;
  }

  sendStatus(404, "Not Found", "Not found.");
}

void WebConfig::applyForm(const String& body, char* notice, size_t noticeLen) {
  bool clearPass    = false;
  bool clearWebPass = false;
  bool anyError     = false;
  char lastErr[64] = {0};

  int start = 0;
  while (start <= (int)body.length()) {
    int amp = body.indexOf('&', start);
    if (amp < 0) amp = body.length();
    const String pair = body.substring(start, amp);
    start = amp + 1;
    if (pair.length() == 0) continue;

    const int eq = pair.indexOf('=');
    if (eq < 0) continue;

    const String key   = urlDecode(pair.substring(0, eq));
    const String value = urlDecode(pair.substring(eq + 1));

    if (key == "pass_clear") {
      clearPass = true;
      continue;
    }
    if (key == "web_pass_clear") {
      clearWebPass = true;
      continue;
    }
    // Checkboxes only post when ticked, so the booleans are handled below.
    if (key == "dhcp" || key == "web") continue;
    // An empty password box means "leave the stored password alone".
    if (key == "pass" && value.length() == 0) continue;
    if (key == "web_pass" && value.length() == 0) continue;

    char err[64] = {0};
    if (!config::setField(*cfg_, key.c_str(), value.c_str(), err, sizeof(err))) {
      anyError = true;
      snprintf(lastErr, sizeof(lastErr), "%s: %s", key.c_str(), err);
    }
  }

  // Unticked checkboxes are absent from the body entirely.
  cfg_->useDhcp    = body.indexOf("dhcp=on") >= 0 ? 1 : 0;
  cfg_->webEnabled = body.indexOf("web=on") >= 0 ? 1 : 0;
  if (clearPass) config::setField(*cfg_, "pass", "", nullptr, 0);
  if (clearWebPass) config::setField(*cfg_, "web_pass", "", nullptr, 0);

  if (anyError) {
    snprintf(notice, noticeLen, "Not saved -- %s", lastErr);
    return;
  }

  if (config::save(*cfg_)) {
    snprintf(notice, noticeLen, "Saved. Reboot to apply network changes.");
  } else {
    snprintf(notice, noticeLen, "Could not write to EEPROM.");
  }
}

void WebConfig::sendStatus(int code, const char* reason, const char* body) {
  client_.printf("HTTP/1.1 %d %s\r\n", code, reason);
  client_.print(F("Content-Type: text/plain\r\nConnection: close\r\n"));
  client_.printf("Content-Length: %u\r\n\r\n", (unsigned)strlen(body));
  client_.print(body);
  client_.flush();
}

void WebConfig::sendFormPage(const char* notice) {
  char buf[72];
  String page;
  page.reserve(4096);

  page += F("<!doctype html><html><head><meta charset=\"utf-8\">"
            "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
            "<title>Teensy Intercom</title><style>"
            "body{font-family:system-ui,sans-serif;max-width:34rem;margin:2rem auto;padding:0 1rem;"
            "background:#101418;color:#e8edf2}"
            "h1{font-size:1.3rem}fieldset{border:1px solid #2c343c;border-radius:8px;margin:0 0 1rem}"
            "legend{padding:0 .4rem;color:#9fb0c0;font-size:.85rem;text-transform:uppercase}"
            "label{display:block;margin:.6rem 0 .2rem;font-size:.9rem}"
            "input[type=text],input[type=password],input[type=number]{width:100%;box-sizing:border-box;"
            "padding:.45rem;border-radius:6px;border:1px solid #2c343c;background:#161c22;color:#e8edf2}"
            "button{padding:.55rem 1rem;border-radius:6px;border:0;background:#2f81f7;color:#fff;"
            "font-size:.95rem;cursor:pointer;margin-right:.5rem}"
            "button.secondary{background:#39434d}"
            ".notice{padding:.6rem .8rem;border-radius:6px;background:#1d2a1d;border:1px solid #2f5a2f;"
            "margin-bottom:1rem}.hint{color:#8b9aa8;font-size:.8rem;margin:.2rem 0 0}"
            ".warn{padding:.6rem .8rem;border-radius:6px;background:#2a231d;"
            "border:1px solid #5a482f;margin-bottom:1rem;font-size:.85rem}"
            "a{color:#6cb0ff}footer{margin-top:1.5rem;padding-top:.8rem;"
            "border-top:1px solid #2c343c;color:#8b9aa8;font-size:.78rem}"
            "</style></head><body><h1>Teensy Intercom</h1>");

  page += F("<p class=\"hint\">Firmware " INTERCOM_FW_VERSION " &middot; IP ");
  const IPAddress ip = Ethernet.localIP();
  snprintf(buf, sizeof(buf), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  page += buf;
  page += F("</p>");

  if (notice && notice[0]) {
    page += F("<div class=\"notice\">");
    page += htmlEscape(notice);
    page += F("</div>");
  }

  if (cfg_->webPass[0] == '\0') {
    page += F("<div class=\"warn\">This page has no password, so anyone on your "
              "network can change these settings. Set a web password below, or "
              "disable the page entirely.</div>");
  }

  page += F("<form method=\"post\" action=\"/save\">");

  page += F("<fieldset><legend>MQTT broker</legend>");
  config::getField(*cfg_, "host", buf, sizeof(buf), false);
  page += F("<label>Broker address</label><input type=\"text\" name=\"host\" value=\"");
  page += htmlEscape(buf);
  page += F("\" placeholder=\"192.168.1.10 or homeassistant.local\">");

  config::getField(*cfg_, "port", buf, sizeof(buf), false);
  page += F("<label>Port</label><input type=\"number\" name=\"port\" min=\"1\" max=\"65535\" value=\"");
  page += htmlEscape(buf);
  page += F("\">");

  config::getField(*cfg_, "user", buf, sizeof(buf), false);
  page += F("<label>Username</label><input type=\"text\" name=\"user\" value=\"");
  page += htmlEscape(buf);
  page += F("\">");

  page += F("<label>Password</label><input type=\"password\" name=\"pass\" placeholder=\"(unchanged)\">"
            "<p class=\"hint\"><label><input type=\"checkbox\" name=\"pass_clear\" value=\"1\"> "
            "clear the stored password</label></p></fieldset>");

  page += F("<fieldset><legend>Home Assistant</legend>");
  config::getField(*cfg_, "device_name", buf, sizeof(buf), false);
  page += F("<label>Device name</label><input type=\"text\" name=\"device_name\" value=\"");
  page += htmlEscape(buf);
  page += F("\">");

  config::getField(*cfg_, "device_id", buf, sizeof(buf), false);
  page += F("<label>Device ID</label><input type=\"text\" name=\"device_id\" value=\"");
  page += htmlEscape(buf);
  page += F("\"><p class=\"hint\">Changing this creates new entities in Home Assistant.</p>");

  config::getField(*cfg_, "base_topic", buf, sizeof(buf), false);
  page += F("<label>Base topic</label><input type=\"text\" name=\"base_topic\" value=\"");
  page += htmlEscape(buf);
  page += F("\">");

  config::getField(*cfg_, "discovery_prefix", buf, sizeof(buf), false);
  page += F("<label>Discovery prefix</label><input type=\"text\" name=\"discovery_prefix\" value=\"");
  page += htmlEscape(buf);
  page += F("\"></fieldset>");

  page += F("<fieldset><legend>Network</legend><label><input type=\"checkbox\" name=\"dhcp\" value=\"on\"");
  if (cfg_->useDhcp) page += F(" checked");
  page += F("> Use DHCP</label>");

  static const char* const kNetKeys[]   = {"ip", "mask", "gw", "dns"};
  static const char* const kNetLabels[] = {"Static IP", "Subnet mask", "Gateway", "DNS server"};
  for (uint8_t i = 0; i < 4; i++) {
    config::getField(*cfg_, kNetKeys[i], buf, sizeof(buf), false);
    page += F("<label>");
    page += kNetLabels[i];
    page += F("</label><input type=\"text\" name=\"");
    page += kNetKeys[i];
    page += F("\" value=\"");
    page += htmlEscape(buf);
    page += F("\">");
  }
  page += F("</fieldset>");

  page += F("<fieldset><legend>Services</legend><label><input type=\"checkbox\" name=\"web\" value=\"on\"");
  if (cfg_->webEnabled) page += F(" checked");
  page += F("> Keep this web page enabled</label>"
            "<p class=\"hint\">Turning this off means serial is the only way back in.</p>");

  config::getField(*cfg_, "web_user", buf, sizeof(buf), false);
  page += F("<label>Web username</label><input type=\"text\" name=\"web_user\" value=\"");
  page += htmlEscape(buf);
  page += F("\">");

  page += F("<label>Web password</label>"
            "<input type=\"password\" name=\"web_pass\" placeholder=\"(unchanged)\">"
            "<p class=\"hint\"><label><input type=\"checkbox\" name=\"web_pass_clear\" value=\"1\"> "
            "clear it (leaves this page open to anyone on your network)</label></p></fieldset>");

  page += F("<button type=\"submit\">Save</button></form>"
            "<form method=\"post\" action=\"/reboot\" style=\"display:inline\">"
            "<button class=\"secondary\" type=\"submit\">Reboot</button></form>");

  // Source-code offer. Required by AGPL-3.0 section 13 for the network-facing
  // interface of a work that links QNEthernet, and good manners regardless.
  page += F("<footer>Teensy Intercom " INTERCOM_FW_VERSION " &middot; "
            "<a href=\"" INTERCOM_SOURCE_URL "\">Source code</a><br>"
            "Provided with no warranty of any kind. This firmware links "
            "QNEthernet (AGPL-3.0-or-later); the complete corresponding source "
            "for this device is available at the link above.</footer>"
            "</body></html>");

  client_.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                  "Cache-Control: no-store\r\nConnection: close\r\n"));
  client_.printf("Content-Length: %u\r\n\r\n", (unsigned)page.length());
  client_.print(page);
  client_.flush();
}
