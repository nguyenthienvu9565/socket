#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "ControlChannel.h"
#include "DataChannel.h"
#include <regex>

using namespace std;

// Hàm hỗ trợ tách chuỗi
vector<string> split(const string& str, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(str);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

int main() {
    // Khởi tạo Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed." << endl;
        return 1;
    }

    ControlChannel control;
    string serverIP;
    int controlPort;

    cout << "=== Hybrid FTP Client (TCP/UDP) ===" << endl;
    cout << "Nhập IP Server: ";
    getline(cin, serverIP);
    
    cout << "Nhập Port Control (Mặc định 21): ";
    string portStr;
    getline(cin, portStr);
    controlPort = portStr.empty() ? 21 : stoi(portStr);

    // 1. Kết nối đến Server
    if (!control.connectToServer(serverIP, controlPort)) {
        cout << "Không thể kết nối đến Server." << endl;
        WSACleanup();
        return 1;
    }

    // Nhận lời chào từ Server
    cout << control.receiveResponse();

    // 2. Vòng lặp lệnh (CLI Loop)
    string inputLine;
    while (true) {
        cout << "ftp> ";
        if (!getline(cin, inputLine)) break;
        if (inputLine.empty()) continue;

        // Trích xuất tên lệnh (ví dụ: USER, PASS, RETR, STOR)
        vector<string> tokens = split(inputLine, ' ');
        string cmd = tokens.empty() ? "" : tokens[0];
        for (char &c : cmd) c = toupper(c); // Chuyển thành in hoa để dễ so sánh

        // Xử lý các lệnh đặc biệt nếu cần truyền file bằng UDP Data Channel
        // Để sử dụng DataChannel đầy đủ, ta sẽ cần thông tin IP và Port của Data Channel từ Server
        // Tuy nhiên trong giới hạn CLI cơ bản, ta gửi lệnh qua Kênh Điều khiển trước.
        
        if (!control.sendCommand(inputLine)) {
            cout << "Gửi lệnh thất bại." << endl;
            break;
        }

        // Nhận mã phản hồi (Reply Code) và nội dung
        string response = control.receiveResponse();
        cout << response;

        if (cmd == "QUIT") {
            cout << "Đang ngắt kết nối..." << endl;
            break;
        }

        // Tích hợp Data Channel cho lệnh RETR và STOR
        if ((cmd == "RETR" || cmd == "STOR") && (response.find("150") == 0 || response.find("227") != string::npos || response.find("200") == 0)) {
            string dataIP = serverIP; // Mặc định dùng IP Server
            int dataPort = 0;

            // Cố gắng parse IP và Port từ phản hồi của Server
            // Hỗ trợ định dạng PASV (h1,h2,h3,h4,p1,p2)
            size_t start = response.find('(');
            size_t end = response.find(')');
            if (start != string::npos && end != string::npos && end > start) {
                string data = response.substr(start + 1, end - start - 1);
                vector<string> parts = split(data, ',');
                if (parts.size() == 6) {
                    dataIP = parts[0] + "." + parts[1] + "." + parts[2] + "." + parts[3];
                    dataPort = stoi(parts[4]) * 256 + stoi(parts[5]);
                }
            } 
            else {
                // Thử parse định dạng IP:Port hoặc IP Port thông thường bằng regex
                regex re("([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})[: ]([0-9]+)");
                smatch match;
                if (regex_search(response, match, re)) {
                    dataIP = match[1];
                    dataPort = stoi(match[2]);
                } else {
                    // Nếu không parse được, yêu cầu người dùng nhập (hoặc dùng cổng mặc định)
                    cout << "[Client] Khong tim thay Port trong phan hoi. Nhap Port Data Server: ";
                    string p;
                    getline(cin, p);
                    if (!p.empty()) dataPort = stoi(p);
                }
            }

            if (dataPort > 0) {
                DataChannel dataChannel;
                // Khởi tạo UDP trên Client (có thể dùng port cục bộ ngẫu nhiên bằng cách truyền 0)
                if (dataChannel.initUDP(0)) {
                    string fileName = (tokens.size() > 1) ? tokens[1] : "unknown.file";
                    if (cmd == "RETR") {
                        dataChannel.receiveFile(fileName, dataIP, dataPort);
                    } else if (cmd == "STOR") {
                        dataChannel.sendFile(fileName, dataIP, dataPort);
                    }
                }
            } else {
                cout << "[Client] Khong xac dinh duoc Data Port de truyen file!\n";
            }
        }

    // 3. Đóng kết nối
    control.disconnect();
    WSACleanup();

    return 0;
}
