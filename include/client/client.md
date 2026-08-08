# 🚀 KẾ HOẠCH NÂNG CẤP HYBRID FTP CLIENT ĐẠT CHUẨN EXCELLENT

**Người phụ trách:** Thành viên 1 (Quản lý Client)
**Mục tiêu:** Hoàn thiện 100% yêu cầu kỹ thuật (Mục 2.2, 2.3), tích hợp thành công UDP RDT, bổ sung tài liệu Báo cáo Kỹ thuật và chuẩn bị Vấn đáp.

---

## 📋 YÊU CẦU ĐẶC TẢ TỪ GIẢNG VIÊN (SECTION 2.2 & 2.3)

### 2.2 Approved FTP Commands
The following table defines the complete list of FTP commands accepted by this project. Each command must be transmitted over the TCP control channel. The Level column indicates the evaluation tier at which the command is expected to be functional.

| Command | Syntax | Description |
| :--- | :--- | :--- |
| **USER** | `USER <username>` | Send the client's username to initiate an authentication session. |
| **PASS** | `PASS <password>` | Send the client's password to complete authentication. |
| **QUIT** | `QUIT` | Gracefully terminate the control connection and end the session. |
| **NOOP** | `NOOP` | No-operation; used as a keep-alive ping to prevent session timeout. |
| **PWD** | `PWD` | Print the server's current working directory path. |
| **CWD** | `CWD <path>` | Change the server's current working directory to the specified path. |
| **CDUP** | `CDUP` | Change the server's working directory to its parent directory. |
| **MKD** | `MKD <dirname>` | Create a new directory on the server at the current path. |
| **RMD** | `RMD <dirname>` | Remove an empty directory from the server. |
| **LIST** | `LIST [path]` | Return a detailed listing (name, size, type, permissions) of files and directories in the current or specified path. |
| **NLST** | `NLST [path]` | Return a plain name-only listing of files in the current or specified path. |
| **STAT** | `STAT [path]` | Return server status or, if a path is given, file/directory metadata. |
| **SIZE** | `SIZE <filename>` | Return the exact byte size of the specified file on the server. |
| **MDTM** | `MDTM <filename>` | Return the last modification timestamp of the specified file (format: YYYYMMDDhhmmss). |
| **TYPE** | `TYPE {A \| I}` | Set the data transfer type: A = ASCII (text), I = Image/Binary. |
| **MODE** | `MODE {S \| B \| C}` | Set the transfer mode: S = Stream, B = Block, C = Compressed. |
| **PORT** | `PORT <h1,h2,h3,h4,p1,p2>` | Active Mode: Client specifies its IP and port for the server to open the data connection back to. |
| **PASV** | `PASV` | Passive Mode: Server opens a random port and returns its IP + port for the client to connect to. |
| **RETR** | `RETR <filename>` | Retrieve (download) the specified file from the server to the client via the data channel. |
| **STOR** | `STOR <filename>` | Store (upload) a file from the client to the server using the current filename. |
| **STOU** | `STOU` | Store a file with a guaranteed unique server-generated filename to prevent overwrites. |
| **APPE** | `APPE <filename>` | Append the uploaded data to an existing file on the server; create it if absent. |
| **DELE** | `DELE <filename>` | Delete the specified file from the server. |
| **RNFR** | `RNFR <oldname>` | Rename From: specify the file to be renamed (must be followed by RNTO). |
| **RNTO** | `RNTO <newname>` | Rename To: complete the rename operation initiated by RNFR. |
| **HASH** | `HASH <filename>` | Request a cryptographic hash (MD5 or SHA-256) of the specified file for post-transfer integrity verification. |
| **ABOR** | `ABOR` | Abort the current data transfer in progress; data channel is reset. |
| **HELP** | `HELP [command]` | Return help text for all supported commands, or detailed usage for a specific command. |

---

### 2.3 Standard Server Reply Codes
The server must respond to every client command using standard three-digit FTP reply codes over the TCP control channel:

| Code | Category | Common Examples |
| :--- | :--- | :--- |
| **1xx** | Positive Preliminary Reply | `125 Data connection already open`; `150 File status okay, opening data connection`. |
| **2xx** | Positive Completion Reply | `200 Command OK`; `220 Service ready`; `221 Goodbye`; `226 Transfer complete`; `230 Login successful`; `250 Requested file action OK`. |
| **3xx** | Positive Intermediate Reply | `331 Username OK, need password`; `350 Requested file action pending RNTO`. |
| **4xx** | Transient Negative Reply | `421 Service unavailable`; `425 Can't open data connection`; `426 Connection closed; transfer aborted`; `450 File unavailable`. |
| **5xx** | Permanent Negative Reply | `500 Syntax error`; `501 Syntax error in parameters`; `502 Command not implemented`; `530 Not logged in`; `550 File unavailable`. |

---

## 🏆 TIÊU CHÍ ĐÁNH GIÁ MỨC EXCELLENT (SECTION 3 - GRADING RUBRIC)

Để đạt điểm tối đa ở mức **Excellent**, dự án và phần làm việc cá nhân của Client cần đáp ứng 4 cột tiêu chí sau:

1. **Code Quality & Application Demo (40% Weight - Excellent):**
   > *"Optimised reliable UDP (ACK + timeout recovery); functional congestion control or end-to-end hash verification."*
2. **Theoretical Understanding / Oral Viva (30% Weight - Excellent):**
   > *"Flawless mastery of RDT states (Stop-and-Wait, GBN, SR); defends bandwidth optimisation strategies with mathematical precision."*
3. **Live Coding & On-the-Spot Debugging (20% Weight - Excellent):**
   > *"Rewrites code segments live to satisfy arbitrary network constraints or edge-case scenarios set by the examiner."*
4. **Technical Documentation & GenAI Provenance (10% Weight - Excellent):**
   > *"Industry-grade documentation; packet headers charted to individual bit/byte fields; GenAI appendix shows deep critical auditing of AI output."*

---

## 🛠️ GIAI ĐOẠN 1: TỐI ƯU CÚ PHÁP LỆNH & GIAO DIỆN CLI (File `client_cli.h` / `client_cli.cpp`)
*Mục tiêu: Đảm bảo Client gửi lệnh chuẩn xác theo Mục 2.2 và giao diện trực quan.*

- [ ] **Sửa lỗi "nuốt" khoảng trắng trong tên file:**
  - Thay thế `ss >> argOut` bằng `std::getline(ss, remaining)` trong hàm `parseAndValidate()`.
  - Thêm logic xóa khoảng trắng thừa ở đầu chuỗi (trim) để bắt chính xác tham số chứa dấu cách (VD: `RETR tai lieu cua toi.pdf`).
- [ ] **Bổ sung bộ lọc toàn bộ lệnh FTP (Mục 2.2):**
  - Nhóm lệnh không tham số: Cập nhật kiểm tra hợp lệ cho `PWD`, `CDUP`, `PASV`, `NOOP`, `QUIT`, `STOU`, `ABOR`.
  - Nhóm lệnh có tham số: Cập nhật kiểm tra hợp lệ cho `USER`, `PASS`, `CWD`, `MKD`, `RMD`, `LIST`, `NLST`, `SIZE`, `MDTM`, `TYPE`, `MODE`, `PORT`, `RETR`, `STOR`, `APPE`, `DELE`, `RNFR`, `RNTO`, `HASH`, `HELP`.
- [ ] **Nâng cấp UI (Giao diện):**
  - Viết thêm hàm `printProgressBar(uint64_t current, uint64_t total)` hiển thị thanh tiến độ khi đang tải/up file.
  - Hiển thị rõ trạng thái kết nối mạng trên console (IP, Port đang dùng).

---

## 🛡️ GIAI ĐOẠN 2: BẢO MẬT BỘ NHỚ & XỬ LÝ CHẾ ĐỘ MẠNG (File `client_ftp.h` / `client_ftp.cpp`)
*Mục tiêu: Nâng cấp Socket API và xử lý động IP/Port (Active/Passive Mode).*

- [ ] **Bảo mật và chống lỗi tràn bộ nhớ (Foundation):**
  - Xóa bỏ hàm cũ `inet_addr`.
  - Thêm `ZeroMemory(&serverAddr, sizeof(serverAddr));` vào trước khi gán địa chỉ.
  - Sử dụng `inet_pton(AF_INET, ip, &serverAddr.sin_addr)` và viết lệnh `if` kiểm tra kết quả trả về (`0` hoặc `< 0`) để bắt lỗi định dạng IP.
- [ ] **Xử lý Passive Mode (Lệnh PASV):**
  - Viết hàm `parsePassiveResponse()` dùng xử lý chuỗi (hoặc Regex) tách 6 số từ mã trả về `227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)`.
  - Tính toán Port theo công thức: $\text{Port} = p1 \times 256 + p2$.
- [ ] **Xử lý Active Mode (Lệnh PORT):**
  - Viết hàm `buildPortCommand()` lấy IP cục bộ của Client và Port ngẫu nhiên để tạo thành chuỗi `PORT h1,h2,h3,h4,p1,p2` gửi cho Server.

---

## ⚙️ GIAI ĐOẠN 3: ĐIỀU PHỐI MÁY TRẠNG THÁI & KÍCH HOẠT UDP (File `main.cpp`)
*Mục tiêu: Xử lý Mã phản hồi (Mục 2.3), tích hợp module RDT và đọc/ghi file nhị phân.*

- [ ] **Bỏ Hardcode IP/Port:**
  - Viết code cho phép người dùng nhập IP và Port của Server trực tiếp từ màn hình Console khi khởi động ứng dụng.
- [ ] **Xây dựng Máy Trạng Thái cho Reply Code (Mục 2.3):**
  - Thêm cấu trúc `if-else` xử lý dựa trên hàm `getReplyCode(response)`.
  - **Nhóm 1xx:** Bắt mã `150 Opening Data Connection`.
  - **Nhóm 2xx:** Bắt các mã thành công (`220`, `230`, `226`, `227`) để in log màu xanh lá hoặc lấy dữ liệu.
  - **Nhóm 3xx:** Bắt mã `331` để yêu cầu nhập Password.
  - **Nhóm 4xx/5xx:** In chi tiết lỗi và hủy thao tác hiện tại để tránh treo Client.
- [ ] **Kích hoạt luồng UDP RDT (Kênh dữ liệu):**
  - Khi bắt được mã `150` và lệnh là `RETR`: Mở luồng nhận và gọi hàm `rdt_receive_file()`.
  - Khi bắt được mã `150` và lệnh là `STOR`: Mở luồng gửi và gọi hàm `rdt_send_file()`.
  - **ĐẶC BIỆT (Mục 1.3):** Khi bắt được mã `150` và lệnh là `LIST` hoặc `NLST`: Gọi hàm nhận UDP để hứng chuỗi text chứa danh sách thư mục, sau đó in ra màn hình.
- [ ] **Đảm bảo tính toàn vẹn Dữ liệu Nhị phân:**
  - Cập nhật luồng I/O file dùng cờ `std::ios::binary` để không làm hỏng file ảnh, video, nén.
  - Bổ sung logic tự động gọi lệnh `HASH <filename>` sau khi UDP truyền xong để so sánh chuỗi mã băm từ Server với mã băm local.

---

## 📑 GIAI ĐOẠN 4: HOÀN THIỆN TÀI LIỆU VÀ CHUẨN BỊ VẤN ĐÁP
*Mục tiêu: Hoàn tất phụ lục Báo cáo (Mục 2.4) và ôn tập cho buổi Vấn đáp (Oral Viva - Mục 3).*

- [ ] **Hoàn thiện tài liệu Kỹ thuật (Góp phần vào Báo cáo nhóm):**
  - Vẽ Sequence Diagram cho luồng hoạt động của lệnh `RETR` (từ lúc gửi TCP -> nhận mã 150 -> mở luồng UDP -> truyền file -> nhận EOF -> nhận TCP 226).
  - Chụp ảnh minh chứng ứng dụng (Application Demo Evidence): CLI hoạt động mượt mà, tải thành công file ảnh lớn.
- [ ] **Nhật ký GenAI (`docs/gen_ai_log.md`):**
  - Ghi chép lại quá trình từ lúc dùng AI tạo code Raw ban đầu, đến quá trình tự refactor module (`client_ftp`, `client_cli`) và nâng cấp API (`inet_pton`).
- [ ] **Chuẩn bị Vấn đáp (Oral Defense & Live Code):**
  - Hiểu rõ sự khác biệt giữa `SOCK_STREAM` (TCP) và `SOCK_DGRAM` (UDP).
  - Nắm vững công thức toán học tính Port trong Active/Passive Mode.
  - Sẵn sàng thao tác trực tiếp (Live code): Sửa nhanh cú pháp chuỗi, ngắt luồng TCP khẩn cấp, hoặc thay đổi port tĩnh thành port động nếu thầy yêu cầu.