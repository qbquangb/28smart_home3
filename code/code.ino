/* Module SIM + Arduino UNO
   - Sensor cửa, chân số 2
   - SoftwareSerial: RX=10, TX=11 (to module)
*/

#include <SoftwareSerial.h>

SoftwareSerial simSerial(10, 11); // RX, TX (Arduino pins)
const int SENSOR_CUA = 2;
const int DEN = 7;
const int MAYTINH = 8;
const int COI = 4;
const int LED_STATUS = 5;
const int LOA = 6;

const char targetNumber[] = "0379019035"; // số gọi đến cần phát hiện
const unsigned long coiTimeOut = 60000; // thời gian còi hú (1 phút)

bool trangthaiden = false; // trạng thái đèn, false=tắt, true=bật
bool trangthailoa = false; // trạng thái loa, false=tắt, true=bật
bool trangthaibaove = false; // trạng thái bảo vệ, false=tắt, true=bật
bool isRunFirst = true; // Đoạn code chỉ chạy 1 lần

String Lenhbatden = "batden";
String Lenhtatden = "tatden";
String Lenhbatloa = "batloa";
String Lenhtatloa = "tatloa";
String Lenhbatmaytinh = "batmaytinh";
String Lenhkiemtra = "trangthai"; // kiểm tra trạng thái thiết bị và kiểm tra trạng thái bảo vệ
String simBuf = ""; // buffer để đọc dữ liệu từ module SIM

void sendSMS(String message);
void processURC(String &s);
String readLineFromSim(unsigned long timeout = 4000);

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
    simSerial.println("ATH");
    delay(9000);

    int firstQuote = s.indexOf('"');
    int secondQuote = s.indexOf('"', firstQuote + 1);
    if (firstQuote != -1 && secondQuote != -1) {
      String number = s.substring(firstQuote + 1, secondQuote);

      // Gọi điện đến số targetNumber để thay đổi biến trangthaibaove
      if (number == String(targetNumber)) {
        trangthaibaove = !trangthaibaove;
        String temp = trangthaibaove ? "BAT" : "TAT";
        String msg = "Trang thai bao ve dang: " + temp + ".";
        if (trangthaibaove == true) {digitalWrite(LED_STATUS, LOW);} 
        else {digitalWrite(LED_STATUS, HIGH);}
        sendSMS(msg);
      }
      else  {
        String msg = "Cuoc goi den tu so: " + number;
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
    // Kiểm tra số gửi SMS có trùng với targetNumber không
    int firstQuote = s.indexOf('"');
    int secondQuote = s.indexOf('"', firstQuote + 1);
    if (firstQuote == -1 || secondQuote == -1) return; // lỗi định dạng
    String number = s.substring(firstQuote + 1, secondQuote);
    // Chỉ xử lý với chuổi number có chiều dài lớn hơn 6
    if (number.length() <= 6) return;
    // Hiệu chỉnh cho phù hợp với định dạng số điện thoại từ +84379019035 sang 0379019035
    if (number.startsWith("+84")) {
      number = "0" + number.substring(3);
    }
    // Nếu số gửi SMS không phải là targetNumber thì bỏ qua
    if (number != String(targetNumber)) {
      String msg = "SMS da nhan tu so: " + number;
      sendSMS(msg);
      return;
    }
    // next line is the SMS body - read it
    String body = readLineFromSim();
    body.trim();
    body.toLowerCase();

    // Nhắn tin bật đèn
    if (body.indexOf(Lenhbatden) != -1) {
      String msg = "Den dang bat.";
      digitalWrite(DEN, HIGH);
      trangthaiden = true;
      sendSMS(msg);
    // Nhắn tin tắt đèn
    } else if (body.indexOf(Lenhtatden) != -1) {
      String msg = "Den da tat.";
      digitalWrite(DEN, LOW);
      trangthaiden = false;
      sendSMS(msg);
    // Nhắn tin bật máy tính
    } else if (body.indexOf(Lenhbatmaytinh) != -1) {
      String msg = "May tinh da bat";
      digitalWrite(MAYTINH, HIGH);
      delay(800);
      digitalWrite(MAYTINH, LOW);
      sendSMS(msg);
    // Nhắn tin bật loa
    } else if (body.indexOf(Lenhbatloa) != -1) {
      String msg = "Loa dang bat.";
      digitalWrite(LOA, HIGH);
      trangthailoa = true;
      sendSMS(msg);
    // Nhắn tin tắt loa
    } else if (body.indexOf(Lenhtatloa) != -1) {
      String msg = "Loa da tat.";
      digitalWrite(LOA, LOW);
      trangthailoa = false;
      sendSMS(msg);
    // Nhắn tin kiểm tra trạng thái thiết bị và kiểm tra trạng thái biến trangthaibaove
    } else if (body.indexOf(Lenhkiemtra) != -1) {
      String trangthaidenstr = trangthaiden ? "BAT" : "TAT";
      String trangthailoastr = trangthailoa ? "BAT" : "TAT";
      String trangthaibaovestr = trangthaibaove ? "BAT" : "TAT";
      String msg = "Trang thai hien tai:\r\nDen: " + trangthaidenstr + "\r\nLoa: " + trangthailoastr + "\r\nBao ve: " + trangthaibaovestr + ".";
      sendSMS(msg);
    } else {
      String msg = "Tin nhắn SMS đã nhận: lệnh không hợp lệ.\r\nKhông có hành động nào được thực hiện.";
      sendSMS(msg);
    }
    return;
  }
}

void sendSMS(String message) {
  String smsCmd = "AT+CMGS=\"" + String(targetNumber) + "\""; // gửi SMS đến số targetNumber
  simSerial.println(smsCmd);
  delay(4000);
  simSerial.print(message);
  delay(2000);
  simSerial.write(26); // Ctrl+Z to send
  delay(10000); // chờ gửi xong
}

void setup() {

  // Chờ module SIM khởi động
  delay(40000); // 40s, tùy module có thể cần lâu hơn

  pinMode(SENSOR_CUA, INPUT);
  pinMode(DEN, OUTPUT);
  pinMode(MAYTINH, OUTPUT);
  pinMode(COI, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LOA, OUTPUT);

  digitalWrite(DEN, LOW);
  digitalWrite(MAYTINH, LOW);
  digitalWrite(COI, LOW);
  digitalWrite(LED_STATUS, HIGH); // bật LED trạng thái
  digitalWrite(LOA, LOW);

  simSerial.begin(9600);
  delay(200);

  // Một vài lệnh khởi tạo - có thể cần delay lâu hơn tùy module
  simSerial.println("AT"); // Kiểm tra kết nối
  delay(13000);
  simSerial.println("ATE0"); // tắt echo
  delay(13000);
  simSerial.println("AT+CLIP=1");   // bật caller ID
  delay(13000);
  simSerial.println("AT+CVHU=0");
  delay(13000);
  simSerial.println("AT+CMGF=1");   // SMS text mode
  delay(13000);
  simSerial.println("AT+CNMI=2,2,0,0,0"); // yêu cầu module gửi +CMT khi có SMS mới đến
  delay(13000);

  // Nhắn tin thông báo khởi động thành công
  sendSMS("Thiet bi da khoi dong thanh cong.");
  delay(10000);
}

void loop() {

  if (digitalRead(SENSOR_CUA) == HIGH && trangthaibaove == true && isRunFirst == true) {
    delay(2000);
    if (digitalRead(SENSOR_CUA) == HIGH) {

    unsigned long alertStart = millis();
    digitalWrite(COI, HIGH);

    // Gọi điện đến số targetNumber
    String callCmd = "ATD" + String(targetNumber) + ";";
    simSerial.println(callCmd);
    delay(5000);
    simSerial.println("ATH"); // ngắt cuộc gọi
    delay(10000);

    // Gửi SMS cảnh báo
    sendSMS("Canh bao: Cua da mo khi bao ve dang bat!");
    // Bật còi trong 1 phút
    while (millis() - alertStart < coiTimeOut) {
      delay(100);
    }
    digitalWrite(COI, LOW);
    // Thay đổi biến isRunFirst
    isRunFirst = false;
  }
}

  // đọc dữ liệu từ simSerial (non-blocking)
  while (simSerial.available()) {
    char c = (char)simSerial.read();
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
      if (simBuf.length() > 300) simBuf = simBuf.substring(simBuf.length() - 300);
    }
  }

}
