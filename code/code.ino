/* Module SIM + Arduino UNO
   
   - Sensor cửa, chân số 2

   - DEN (pin 7) sẽ bật khi có cuộc gọi từ số targetNumber và biến trạng thái trangthaiden = false
     DEN (pin 7) sẽ tắt khi có cuộc gọi từ số targetNumber và biến trạng thái trangthaiden = true
     DEN (pin 7) sẽ bật/tắt theo SMS "3146batden"/"3146tatden", chữ không phân biệt hoa thường.

   - MAYTINH (pin 8) sẽ bật trong 800ms theo SMS "3146batmaytinh", chữ không phân biệt hoa thường.

   - LOA (pin 3) sẽ bật/tắt theo SMS "3146batloa"/"3146tatloa", chữ không phân biệt hoa thường.

   - COI (pin 4) sẽ bật/tắt theo SMS "3146batcoi"/"3146tatcoi", chữ không phân biệt hoa thường.
   - BUZZER (pin 5) báo trạng thái

   - Gửi thông báo phản hồi trạng thái qua SMS đến số targetNumber khi nhận đươc tin nhắn nội dung "3146trangthai",
                                                                                        không phân biệt hoa thường.

   - SoftwareSerial: RX=10, TX=11 (to module)
*/

#include <SoftwareSerial.h>

SoftwareSerial simSerial(10, 11); // RX, TX (Arduino pins)
const int SENSOR_CUA = 2;
const int DEN = 7;
const int MAYTINH = 8;
const int LOA = 3;
const int COI = 4;
const int BUZZER = 5;

#define BIP_1      1
#define BIP_2      2
#define BIP_3      3
#define BIP_4      4 // Do dac diem cua mach, BIP_4 thay the cho BIP_LONG

const unsigned int waitMsDefault = 13000; // thời gian chờ phản hồi mặc định
const char targetNumber[] = "0379019035"; // số gọi đến cần phát hiện
const unsigned long coiTimeOut = 60000; // thời gian còi hú (1 phút)

bool trangthaiden = false; // trạng thái đèn, false=off, true=on
bool trangthailoa = false; // trạng thái loa, false=off, true=on
bool trangthaibaove = true; // trạng thái bảo vệ, false=off, true=on
bool isRunFirst = true; // Đoạn code chỉ chạy 1 lần

String Lenhbatden = "3146batden";
String Lenhtatden = "3146tatden";
String Lenhbatmaytinh = "3146batmaytinh";
String Lenhbatloa = "3146batloa";
String Lenhtatloa = "3146tatloa";
String Lenhkiemtra = "3146trangthai"; // kiểm tra trạng thái thiết bị
String LenhthaydoibienRun = "3146thaydoibienrun"; // thay đổi biến isRunFirst
String simBuf = ""; // buffer để đọc dữ liệu từ module SIM

void sendSMS(String message);
void sendAT(const char *cmd, unsigned long waitMs = waitMsDefault);
String readLineFromSim(unsigned long timeout = 4000);
void processURC(String &s);
void control_buzzer(uint8_t para);

void control_buzzer(uint8_t para) {
    if (para == BIP_2) {
        digitalWrite(Buzzer, HIGH);
        delay(500);
        digitalWrite(Buzzer, LOW);
        delay(2000);
        digitalWrite(Buzzer, HIGH);
        delay(500);
        digitalWrite(Buzzer, LOW);
        delay(2000);
}
    if (para == BIP_1) {
        digitalWrite(Buzzer, HIGH);
        delay(500);
        digitalWrite(Buzzer, LOW);
        delay(2000);
    }
    if (para == BIP_3)
    {
        digitalWrite(Buzzer, HIGH);
        delay(500);
        digitalWrite(Buzzer, LOW);
        delay(2000);
        digitalWrite(Buzzer, HIGH);
        delay(500);
        digitalWrite(Buzzer, LOW);
        delay(2000);
        digitalWrite(Buzzer, HIGH);
        delay(500);
        digitalWrite(Buzzer, LOW);
        delay(2000);
    }
    if (para == BIP_4)
    {
        digitalWrite(Buzzer, HIGH);
        delay(500);
        digitalWrite(Buzzer, LOW);
        delay(2000);
        digitalWrite(Buzzer, HIGH);
        delay(500);
        digitalWrite(Buzzer, LOW);
        delay(2000);
        digitalWrite(Buzzer, HIGH);
        delay(500);
        digitalWrite(Buzzer, LOW);
        delay(2000);
        digitalWrite(Buzzer, HIGH);
        delay(500);
        digitalWrite(Buzzer, LOW);
        delay(2000);
    }
}

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
    //sendAT("ATA", 100);
    //delay(3000);
    sendAT("ATH",9000); // ATH cũng có thể dùng để ngắt cuộc gọi

    // Ví dụ: +CLIP: "0123456789",145
    int firstQuote = s.indexOf('"');
    int secondQuote = s.indexOf('"', firstQuote + 1);
    if (firstQuote != -1 && secondQuote != -1) {
      String number = s.substring(firstQuote + 1, secondQuote);
      Serial.print("Incoming caller: "); Serial.println(number);

      // Gọi điện đến số targetNumber để thay đổi biến trangthaibaove
      if (number == String(targetNumber)) {
        trangthaibaove = !trangthaibaove;
        String msg = "Cuộc gọi đến từ số: " + number + "\r\nTrạng thái bảo vệ đã được thay đổi thành: " + (trangthaibaove ? "BẬT" : "TẮT") + ".";
        Serial.println(msg);
        trangthaibaove ? control_buzzer(BIP_2) : control_buzzer(BIP_4);
      }
      else  {
        String msg = "Cuộc gọi đến từ số: " + number + "\r\nKhông có hành động nào được thực hiện.";
        Serial.println(msg);
        sendSMS(msg);
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

    // Nhắn tin bật đèn
    if (CommandLower.indexOf(Lenhbatden) != -1) {
      String msg = "Tin nhắn SMS đã nhận: bật đèn.\r\nĐèn đang bật.";
      digitalWrite(DEN, HIGH);
      trangthaiden = true;
      Serial.println(msg);
      sendSMS(msg);
    // Nhắn tin tắt đèn
    } else if (CommandLower.indexOf(Lenhtatden) != -1) {
      String msg = "Tin nhắn SMS đã nhận: tắt đèn.\r\nĐèn đang tắt.";
      digitalWrite(DEN, LOW);
      trangthaiden = false;
      Serial.println(msg);
      sendSMS(msg);
    // Nhắn tin bật máy tính
    } else if (CommandLower.indexOf(Lenhbatmaytinh) != -1) {
      String msg = "Tin nhắn SMS đã nhận: bật máy tính.\r\nMáy tính đã bật.";
      digitalWrite(MAYTINH, HIGH);
      delay(800);
      digitalWrite(MAYTINH, LOW);
      Serial.println(msg);
      sendSMS(msg);
    // Nhắn tin bật loa
    } else if (CommandLower.indexOf(Lenhbatloa) != -1) {
      String msg = "Tin nhắn SMS đã nhận: bật loa.\r\nLoa đang bật.";
      digitalWrite(LOA, HIGH);
      trangthailoa = true;
      Serial.println(msg);
      sendSMS(msg);
    // Nhắn tin tắt loa
    } else if (CommandLower.indexOf(Lenhtatloa) != -1) {
      String msg = "Tin nhắn SMS đã nhận: tắt loa.\r\nLoa đã tắt.";
      digitalWrite(LOA, LOW);
      trangthailoa = false;
      Serial.println(msg);
      sendSMS(msg);
    // Lệnh thay đổi biến isRunFirst
    } else if (CommandLower.indexOf(LenhthaydoibienRun) != -1) {
      isRunFirst = !isRunFirst;
      String msg = "Tin nhắn SMS đã nhận: thay đổi biến isRunFirst.\r\nBiến isRunFirst hiện đang là: ";
      msg += isRunFirst ? "TRUE." : "FALSE.";
      Serial.println(msg);
      sendSMS(msg);
    // Nhắn tin kiểm tra trạng thái thiết bị và kiểm tra trạng thái biến isRunFirst
    } else if (CommandLower.indexOf(Lenhkiemtra) != -1) {
      String denStatus = trangthaiden ? "BẬT" : "TẮT";
      String loaStatus = trangthailoa ? "BẬT" : "TẮT";
      String runFirstStatus = isRunFirst ? "TRUE" : "FALSE";
      String msg = "Tin nhắn SMS đã nhận: kiểm tra trạng thái thiết bị và kiểm tra trạng thái biến isRunFirst.\r\n";
      msg += "Đèn hiện đang: " + denStatus + ".\r\n";
      msg += "Loa hiện đang: " + loaStatus + ".";
      msg += "\r\nBiến isRunFirst hiện đang là: " + runFirstStatus + ".";
      Serial.println(msg);
      sendSMS(msg);
    } else {
      String msg = "Tin nhắn SMS đã nhận: lệnh không hợp lệ.\r\nKhông có hành động nào được thực hiện.";
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

  pinMode(SENSOR_CUA, INPUT);
  pinMode(DEN, OUTPUT);
  pinMode(MAYTINH, OUTPUT);
  pinMode(LOA, OUTPUT);
  pinMode(COI, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(DEN, LOW);
  digitalWrite(MAYTINH, LOW);
  digitalWrite(LOA, LOW);
  digitalWrite(COI, LOW);
  digitalWrite(BUZZER, LOW);
  trangthaiden = false;
  trangthailoa = false;
  trangthaicoi = false;
  trangthaibaove = true;
  isRunFirst = true;

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
    String startupMsg = "Thiết bị đã khởi động.";
    String smsCmd = "AT+CMGS=\"" + String(targetNumber) + "\""; // gửi SMS đến số targetNumber
    sendAT(smsCmd.c_str(), 2000);
    delay(2000);
    simSerial.print(startupMsg);
    simSerial.write(26); // Ctrl+Z to send
    delay(10000); // chờ gửi xong
}

void loop() {

  /*
  Khi cửa mở, trangthaibaove = true và isRunFirst = true thì gọi điện đến số targetNumber, gửi SMS cảnh báo,
  bật còi trong 1 phút, sau đó tắt còi, isRunFirst = false.
  */
  if (digitalRead(SENSOR_CUA) == HIGH && trangthaibaove == true && isRunFirst == true) {
    delay(2000); // chờ 2s để tránh báo động giả
    if (digitalRead(SENSOR_CUA) == HIGH) {

    unsigned long alertStart = millis();
    digitalWrite(COI, HIGH);

    // Gọi điện đến số targetNumber
    String callCmd = "ATD" + String(targetNumber) + ";";
    sendAT(callCmd.c_str(), 10000); // chờ 10s
    delay(5000);
    sendAT("ATH", 5000); // ngắt cuộc gọi sau 5s

    // Gửi SMS cảnh báo
    String alertMsg = "Cảnh báo: Cửa đã mở khi bảo vệ đang bật!";
    Serial.println(alertMsg);
    sendSMS(alertMsg);
    // Bật còi trong 1 phút
    while (millis() - alertStart < coiTimeOut) {
      delay(100);
    }
    // Thay đổi biến isRunFirst
    isRunFirst = false;
  }
}

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
