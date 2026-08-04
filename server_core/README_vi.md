# Server Core (TCP) — Phần của Member 2

Khung mã nguồn (skeleton) cho phần **Server Core & System (TCP) Specialist**.
Đã build và test bằng g++ (chạy thử toàn bộ luồng USER→PASS→PWD→MKD→CWD→
LIST→QUIT qua loopback) — biên dịch sạch, không lỗi không warning.

## Đã hoàn thiện (bạn cần hiểu để bảo vệ, không chỉ chạy được)

- **TCP server core** (`src/main.cpp`): `socket()/bind()/listen()/accept()`,
  mỗi client được xử lý trên 1 thread riêng (`std::thread`), danh sách
  client kết nối được bảo vệ bằng `std::mutex` + `std::lock_guard`.
- **Session xác thực** (`handlers/auth_handler.cpp`): USER, PASS, QUIT, NOOP.
- **Lệnh hệ thống file** (`handlers/fs_handler.cpp`): PWD, CWD, CDUP, MKD,
  RMD, LIST, NLST, SIZE — dùng `std::filesystem`, có hàm `resolveSafePath()`
  chống việc client đi ra ngoài thư mục gốc (directory traversal).
- **Khung xử lý lệnh** (`command_dispatcher.cpp`): parse dòng lệnh, phân
  loại lệnh cần/không cần xác thực, route tới handler đúng.
- **Mã phản hồi 3 chữ số** (`reply_codes.h`): đầy đủ theo bảng ở mục 2.3
  của đề.
- **Khung binary I/O + gọi sang RDT** (`handlers/transfer_handler.cpp`):
  RETR đọc file nhị phân theo từng chunk và gọi `rdt.sendChunk()`; STOR
  gọi `rdt.receiveChunk()` và ghi ra file — đây chính là phần
  "chunk dữ liệu rồi bơm vào lớp RDT của Member 1".
- **`MockRDTChannel`**: một bản UDP thô, KHÔNG tin cậy, chỉ để bạn tự
  build/test phần TCP của mình trước khi Member 1 xong lớp RDT thật. Nhớ
  thay bằng lớp thật của Member 1 trước khi nộp bài.

## Còn phải tự làm (đã đánh dấu `TODO(you)` trong code)

Đây là những phần **cố tình để trống** — bạn cần tự thiết kế và code, vì
đây chính là những chỗ giám khảo sẽ hỏi trong buổi vấn đáp:

1. **Buffer hoá dữ liệu TCP theo dòng lệnh thật sự.** `main.cpp` hiện
   đang giả định 1 lần `recv()` = 1 lệnh — điều này KHÔNG đúng trên mạng
   thật (TCP có thể gộp/chia lệnh bất kỳ lúc nào). Bạn cần buffer và tách
   theo `\r\n`.
2. **PORT / PASV / TYPE / MODE** — chưa có handler nào, `command_dispatcher.cpp`
   có để sẵn comment chỗ cần thêm.
3. **Nối RETR/STOR vào dispatcher** — hiện 2 hàm này cần một `IRDTChannel&`
   mà `handleCommand()` chưa có cách truyền vào; bạn cần quyết định kiến
   trúc (mở kênh RDT ở đâu, truyền qua đâu) — trao đổi với Member 1 vì nó
   ảnh hưởng tới interface chung.
4. **Kiểm tra xác thực thật** (`handlePASS`) — hiện chấp nhận mọi mật khẩu
   không rỗng, chỉ để test tạm.
5. **MDTM** — cần convert `std::filesystem::file_time_type` sang định
   dạng `YYYYMMDDhhmmss`, phần này C++17 khá lắt léo, cần tự tra cứu.
6. **STOU, APPE, DELE, RNFR/RNTO, HASH, ABOR, HELP** — chưa viết, đã có
   gợi ý pattern trong `transfer_handler.h`.
7. **MKD chưa đi qua `resolveSafePath()`** — hiện có thể bị lợi dụng
   traversal giống như CWD từng bị trước khi có hàm đó; nên sửa cho nhất
   quán.

## Build & chạy thử

```bash
g++ -std=c++17 -Wall -Wextra -Iinclude \
  src/main.cpp src/command_dispatcher.cpp src/reply_codes.cpp src/mock_rdt.cpp \
  src/handlers/auth_handler.cpp src/handlers/fs_handler.cpp src/handlers/transfer_handler.cpp \
  -pthread -o server_core

mkdir -p ftp_root
./server_core
```

Server lắng nghe ở cổng `2121` (đổi ở `CONTROL_PORT` trong `main.cpp` nếu
cần). Test nhanh bằng `nc 127.0.0.1 2121` hoặc script Python/telnet, gõ
từng lệnh (`USER alice`, `PASS 1234`, `PWD`, `LIST`...).

(`CMakeLists.txt` cũng có sẵn nếu máy bạn có `cmake` cài sẵn — môi trường
build test ở đây không có cmake nên bản build thử ở trên dùng thẳng
`g++`.)
