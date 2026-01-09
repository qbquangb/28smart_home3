/* Module SIM + Arduino UNO
   - relay1 (pin 7) sẽ bật khi có cuộc gọi từ số targetNumber và biến trạng thái relay1State = false
     relay1 (pin 7) sẽ tắt khi có cuộc gọi từ số targetNumber và biến trạng thái relay1State = true
   - relay2 (pin 8) sẽ bật/tắt theo SMS "3146onrelay2"/"3146offrelay2", chữ không phân biệt hoa thường.
   - Gửi thông báo phản hồi trạng thái qua SMS đến số targetNumber khi nhận đươc tin nhắn nội dung "3146status",
                                                                                  không phân biệt hoa thường.
   - SoftwareSerial: RX=10, TX=11 (to module)
*/

#include <SoftwareSerial.h>

SoftwareSerial simSerial(10, 11); // RX, TX (Arduino pins)
const int relay1Pin = 7;
const int relay2Pin = 8;

const unsigned int waitMsDefault = 13000; // thời gian chờ phản hồi mặc định
const char targetNumber[] = "0379019035"; // số gọi đến cần phát hiện
const unsigned long relay1Timeout = 10000UL; // 10 seconds

bool relay1State = false; // trạng thái relay1, false=off, true=on
bool relay2State = false; // trạng thái relay2, false=off, true=on

String CommandOnRelay2 = "3146onrelay2";
String CommandOffRelay2 = "3146offrelay2";
String CommandStatus = "3146status";
String simBuf = ""; // buffer để đọc dữ liệu từ module SIM

void sendSMS(String message);
void sendAT(const char *cmd, unsigned long waitMs = waitMsDefault);
String readLineFromSim(unsigned long timeout = 4000);
void processURC(String &s);

void sendAT(const char *cmd, unsigned long waitMs = waitMsDefault) { // gửi lệnh AT và chờ phản hồi
  simSerial.println(cmd);
  delay(50);
  unsigned long start = millis();
  while ((millis() - start) < waitMs) {
    if (simSerial.available()) {
      // đọc và forward ra Serial debug
      Serial.write(simSerial.read());
    }
  }
}

String readLineFromSim(unsigned long timeout = 4000) {
  String line = "";
  unsigned long start = millis();
  while ((millis() - start) < timeout) {
    while (simSerial.available()) {
      char c = (char)simSerial.read();
      if (c == '\r') continue;
      if (c == '\n') {
        if (line.length() > 0) return line;
      } else {
        line += c;
      }
    }
  }
  return line;
}

void processURC(String &s) { // xử lý các URC (Unsolicited Result Codes) từ module SIM(các kết quả không mong đợi)
                             // &s: biến s truyền dưới dạng tham chiếu để tiết kiệm bộ nhớ
  s.trim(); // Loại bỏ khoảng trắng đầu và cuối chuỗi bao gồm ' ', '\r', '\n', '\t'
  if (s.length() == 0) return;

  // 1) Xử lý +CLIP (incoming caller id)
  if (s.startsWith("+CLIP:")) {
    delay(1000);
    // Kết thúc cuộc gọi
    // sendAT("AT+CHUP");
    sendAT("ATA", 100);
    delay(3000);
    sendAT("ATH",9000); // ATH cũng có thể dùng để ngắt cuộc gọi

    // Ví dụ: +CLIP: "0123456789",145
    int firstQuote = s.indexOf('"');
    int secondQuote = s.indexOf('"', firstQuote + 1);
    if (firstQuote != -1 && secondQuote != -1) {
      String number = s.substring(firstQuote + 1, secondQuote);
      Serial.print("Incoming caller: "); Serial.println(number);
      if ((number == String(targetNumber)) && (relay1State == false)) {
        String msg = "Incoming call from target number: " + number + "\r\nRelay1 turned ON.";
        digitalWrite(relay1Pin, HIGH);
        relay1State = true;
        delay(2000);
        Serial.println(msg);
        delay(1000);
        sendSMS(msg);
      }
      else if ((number == String(targetNumber)) && (relay1State == true)) {
        String msg = "Incoming call from target number: " + number + "\r\nRelay1 turned OFF.";
        digitalWrite(relay1Pin, LOW);
        relay1State = false;
        delay(2000);
        Serial.println(msg);
        delay(1000);
        sendSMS(msg);
      } else {
        Serial.println("Caller not match. Ignored.");
      }
    }
    return;
  }

  // 2) Xử lý +CMT (SMS delivered directly with text mode)
  // Format example:
  // +CMT: "+84123456789","","22/12/31,12:34:56+08"
  // On relay2
  if (s.startsWith("+CMT:")) {
    // next line is the SMS body - read it
    String body = readLineFromSim();
    body.trim();
    Serial.print("SMS body: '"); Serial.print(body); Serial.println("'");
    String CommandLower = body;
    CommandLower.toLowerCase();

    if (CommandLower.indexOf(CommandOnRelay2) != -1) {
      String msg = "SMS command received: ON relay2.\r\nRelay2 turned ON.";
      digitalWrite(relay2Pin, HIGH);
      relay2State = true;
      Serial.println(msg);
      sendSMS(msg);
    } else if (CommandLower.indexOf(CommandOffRelay2) != -1) {
      String msg = "SMS command received: OFF relay2.\r\nRelay2 turned OFF.";
      digitalWrite(relay2Pin, LOW);
      relay2State = false;
      Serial.println(msg);
      sendSMS(msg);
    } else if (CommandLower.indexOf(CommandStatus) != -1) {
      String relay1Status = relay1State ? "ON" : "OFF";
      String relay2Status = relay2State ? "ON" : "OFF";
      String msg = "SMS command received: STATUS.\r\n";
      msg += "Relay1 is " + relay1Status + ".\r\n";
      msg += "Relay2 is " + relay2Status + ".";
      Serial.println(msg);
      sendSMS(msg);
    } else {
      String msg = "SMS command received: unknown.\r\nNo action taken.";
      Serial.println(msg);
      sendSMS(msg);
    }
    return;
  }
}

void sendSMS(String message) {
  String smsCmd = "AT+CMGS=\"" + String(targetNumber) + "\""; // gửi SMS đến số targetNumber
  sendAT(smsCmd.c_str(), 2000);
  delay(2000);
  simSerial.print(message);
  simSerial.write(26); // Ctrl+Z to send
  delay(10000); // chờ gửi xong
}

void setup() {

  // Chờ module SIM khởi động
  Serial.println("Init: waiting for module to be ready...");
  delay(40000); // 40s, tùy module có thể cần lâu hơn

  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
  digitalWrite(relay1Pin, LOW);
  relay1State = false;
  digitalWrite(relay2Pin, LOW);
  relay2State = false;

  Serial.begin(9600);
  delay(200);
  simSerial.begin(9600);
  delay(200);

  Serial.println("Init: configuring module...");

  // Một vài lệnh khởi tạo - có thể cần delay lâu hơn tùy module
  sendAT("AT"); // Kiểm tra kết nối
  sendAT("ATE0"); // tắt echo
  sendAT("AT+CLIP=1");   // bật caller ID
  sendAT("AT+CVHU=0");
  sendAT("AT+CMGF=1");   // SMS text mode
  sendAT("AT+CNMI=2,2,0,0,0"); // yêu cầu module gửi +CMT khi có SMS mới đến

  Serial.println("Init done.");

  // Nhắn tin thông báo khởi động thành công
    String startupMsg = "Device started.";
    String smsCmd = "AT+CMGS=\"" + String(targetNumber) + "\""; // gửi SMS đến số targetNumber
    sendAT(smsCmd.c_str(), 2000);
    delay(2000);
    simSerial.print(startupMsg);
    simSerial.write(26); // Ctrl+Z to send
    delay(10000); // chờ gửi xong
}

void loop() {
  // đọc dữ liệu từ simSerial (non-blocking)
  while (simSerial.available()) {
    char c = (char)simSerial.read();
    // forward to debug
    Serial.write(c);
    // append để parse từng dòng
    if (c == '\n') {
      // process accumulated line
      String line = simBuf;
      simBuf = "";
      line.trim();
      if (line.length() > 0) {
        processURC(line);
      }
    } else if (c != '\r') {
      simBuf += c;
      // keep buffer limited
      if (simBuf.length() > 512) simBuf = simBuf.substring(simBuf.length() - 512);
    }
  }

}
